/*********************************************************************************************************************/	
/*      file: data_transfer_app.c
 *
 *	brief: Read live plus backup data from storage device, files, convert into json & send to mqtt, ftp, http server
 *
 * 	author: Shilesh Babu
 */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/*				Header Files															*/
/**********************************************************************************************************************/
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <complex.h>
#include <ctype.h>
#include <errno.h>
#include <fenv.h>
#include <float.h>
#include <inttypes.h>
#include <iso646.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <tgmath.h>
#include <pthread.h>
#include <time.h>
#include <uchar.h>
#include <wchar.h>
#include <wctype.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/ioctl.h>
#include <sys/stat.h>


#include "data_transfer_app.h"
#include "../../includes/config.h"
#include "../../includes/common.h"
#include "../../includes/version_control.h"

/****************************************************************************************/
/*					Macros																*/
/****************************************************************************************/
/*	def: MV_BACKUP_DATA_TIME Variable
 *	brief: Move and Send backup data on Mqtt server every 240 sec.
 */
#define MV_BACKUP_DATA_TIME 240


/*******************************************************************************/
/*		fn:	void *Monitor_usb_status_thread(void *thread_arg)
 *
 *		brief: Thread is used to monitor  usb status
 *
 *		param: thread_arg - thread args
 *
 *		return: NULL
 */
/*******************************************************************************/
void *Monitor_usb_status_thread(void *thread_arg);

/********************************************************************************/
/*		fn: void signal_handler(int sig)
 *
 *		brief: Signal Handler function
 *
 *		param: sig - signal captured during running process
 *
 *		return: void
 */
/********************************************************************************/
void signal_handler(int sig)
{
	switch (sig)
	{
	/* catch terminate signal */
	case SIGTERM:
		DTRNSF_DEBUG(DATA_TRNSF_APP, "Terminate signal received");
		data_transfer_app_exit_flag = 1;
		break;
		/* catch interrupt signal */
	case SIGINT:
		DTRNSF_DEBUG(DATA_TRNSF_APP, "Interrupt signal received");
		data_transfer_app_exit_flag = 1;
		break;
	case SIGSEGV:
		DTRNSF_ERROR(DATA_TRNSF_APP, "*************************************************************");
		DTRNSF_ERROR(DATA_TRNSF_APP, "****** Segmentation Fault, PID: %d, TID: %ld ******", getpid(), pthread_self());
		DTRNSF_ERROR(DATA_TRNSF_APP, "*************************************************************");
		exit(-1);
		break;
	}
}

/**********************************************************************************/
/*		fn: int main (void)
 *
 *		brief: Main function
 *
 *		param:void
 *
 *		return: Success or Failure return code
 */
/**********************************************************************************/
int main(void)
{
	/* Function  return status */
	int  ret_status = ND_SUCCESS;
	/* Data transfer app. structure */
	data_app_transfer_ctx_t data_transfer_ctx;

	DTRNSF_INFO(DATA_TRNSF_APP, "DATA_TRANSFER_APPLICATION, FW_VERSION_NUMBER:%s, APP_VERSION_NUMBER:%s", FIRMWARE_VERSION, DATA_TRANSFER_APP_VERSION);
	DTRNSF_INFO(DATA_TRNSF_APP, "Data transfer application app started");

	do{
		/* catch terminate signal */
		signal(SIGTERM, signal_handler);
		/* catch Interrupt signal */
		signal(SIGINT, signal_handler);
		/* catch Interrupt signal */
		signal(SIGSEGV, signal_handler);

		/* Memset data transfer app structure */
		memset(&data_transfer_ctx ,0x00, sizeof(data_app_transfer_ctx_t));
		/* Initialize data transfer application */
		if (ND_SUCCESS != (ret_status = Initialise_data_transfer_app(&data_transfer_ctx))){
#ifdef ENABLE_DEBUG_LOG
			DTRNSF_ERROR(DATA_TRNSF_APP, "Fail to Initialize data transfer application");
#endif
			ret_status = -1;
			break;
		}
		/* Wait till application got exit status */
		while(0 == data_transfer_app_exit_flag ){
			SLEEP_P(SLEEP_TIME_2, data_transfer_app_exit_flag , 0);
		}
	}while(0);
#ifdef ENABLE_DEBUG_LOG
	DTRNSF_INFO(DATA_TRNSF_APP, "Data monitor application is now exiting");
#endif
	/*De_initialize data transfer application */
	if (ND_SUCCESS != Deinitialise_data_transfer_app(&data_transfer_ctx)){
#ifdef ENABLE_DEBUG_LOG
		DTRNSF_ERROR(DATA_TRNSF_APP, "Fail to De_initialize data transfer application");
#endif
		ret_status = -1;
	}
#ifdef ENABLE_DEBUG_LOG
	DTRNSF_INFO(DATA_TRNSF_APP, "Data transfer application is exited");
#endif
	return ret_status;
}

/*******************************************************************************************/
/*					Function Declaration												   */
/*******************************************************************************************/

/*******************************************************************************************/
/*		fn: Initialise_data_transfer_app(data_app_transfer_ctx_t *data_transfer_ctx)
 *
 *		brief: initialize and send data to cloud
 *
 *		param: data_app_transfer_ctx_t *data_transfer_ctx
 *
 *		return: NDCommonReturnCodes
 */
/*******************************************************************************************/
NDCommonReturnCodes Initialise_data_transfer_app(data_app_transfer_ctx_t *data_transfer_ctx)
{
	/* Func. return status */
	NDCommonReturnCodes ret_status = 0;
	/* semaphore return status */
	char semaphore_ret_status =	0;
	/* Tmp counter */
	char tmpCount = 0;
	/* Str. parsing ptr */
	char *pch = NULL;
	/* Usb node str buffer */
	char usb_node[BUF_LEN_30] = {0};
	/* Shared memory read time out */
	char shm_read_tmout = 0;
	/* time structure */
	struct timespec wait_time;


	/* Memset NodeX shm structure */
	memset(&data_transfer_ctx->nodex_info, 0x00, sizeof(nodex_config_info_t));
	/* Memset Mqtt shm structure */
	memset(&data_transfer_ctx->mqtt_config, 0x00, sizeof(mqtt_config_info_t));
#ifdef ENABLE_DEBUG_LOG
	DTRNSF_INFO(DATA_TRNSF_APP,"Initialize data transfer application");
#endif

	do{
		/* Initialize shared memory */
		if ((Shared_memory_initialization(&(data_transfer_ctx->shm_data_trans_ctx.shm_data))) == -1){
#ifdef ENABLE_DEBUG_LOG
			DTRNSF_ERROR(DATA_TRNSF_APP, "Fail to initializing shared memory %s", __func__);
#endif
			ret_status = -1;
			break;
		}
		/* Initialize  semaphore  */
		if (0 != init_sem(&(data_transfer_ctx->data_transfer_sem), 1)){
#ifdef ENABLE_DEBUG_LOG
			DTRNSF_ERROR(DATA_TRNSF_APP, "Fail to initialize  semaphore in %s", __func__);
#endif
			ret_status = -1;
			break;
		}

		/* Sleep to sync process and wait for read */
		sync();
		sleep(30);
		/* If fail first time to read data, Try up to three times to read data from shared memory */
		/* Read device(NodeX) configuration data from shared memory */
		do{
			memset(&wait_time, 0x00, sizeof(wait_time));
			snprintf(data_transfer_ctx->data_transfer_sem.sem_name, 256, "%s", NODEX_CONFIG_INFO);
			/* Initialize semaphore */
			if (-1 == open_sem(&(data_transfer_ctx->data_transfer_sem))){
#ifdef ENABLE_DEBUG_LOG
				DTRNSF_ERROR(DATA_TRNSF_APP, "Fail to open  semaphore in %s", __func__);
#endif
				semaphore_ret_status = -1;
				break;
			}

			/* Get current epoch time */
			if(-1 == clock_gettime(CLOCK_REALTIME, &wait_time)){
				printf("Fail to get time at fun:%s line:%d\n", __func__, __LINE__);
				ret_status = -1;
				break;
			}
			else{
				/* setting timer in seconds. */
				wait_time.tv_sec += SEMAPHORE_TIMEOUT;
			}

			/* Acquire semaphore */
			if (-1 == waittimeout_sem(&(data_transfer_ctx->data_transfer_sem), &(wait_time))){
				printf("%s semaphore timeout occured, fun:%s line:%d\n", NODEX_CONFIG_INFO, __func__, __LINE__);
#ifdef ENABLE_DEBUG_LOG
				DTRNSF_ERROR(DATA_TRNSF_APP, "%s semaphore timeout occured, fun:%s line:%d", NODEX_CONFIG_INFO, __func__, __LINE__);
#endif
				ret_status = -1;
				break;
			}

			if((Shared_memory_Read(&(data_transfer_ctx->shm_data_trans_ctx.shm_data), NODEX_CONFIG_DATA , &data_transfer_ctx->nodex_info, sizeof(nodex_config_info_t))) == -1){
#ifdef ENABLE_DEBUG_LOG
				DTRNSF_ERROR(DATA_TRNSF_APP, "Failed to read device(NodeX) configuration data from shared memory");
#endif
				//ret_status = -1;
				//break;
				/* Reset shared memory read  data flag */
				shm_read_tmout++;
			}
			else{
#ifdef ENABLE_DEBUG_LOG
				DTRNSF_ERROR(DATA_TRNSF_APP, "Read device(NodeX) configuration from shared memory successfully ");
#endif
				/* Reset shared memory read  data flag */
				shm_read_tmout = 0;
			}

			/* Read Mqtt configuration from shared memory */
			if((Shared_memory_Read(&(data_transfer_ctx->shm_data_trans_ctx.shm_data), MQTT_CONFIG_DATA , &data_transfer_ctx->mqtt_config, sizeof(mqtt_config_info_t))) == -1){
#ifdef ENABLE_DEBUG_LOG
				DTRNSF_ERROR(DATA_TRNSF_APP, "Failed to read Mqtt configuration data from shared memory");
#endif
				/* Reset shared memory read  data flag */
				shm_read_tmout++;
				//ret_status = -1;
				//break;
			}
			else{
#ifdef ENABLE_DEBUG_LOG
				DTRNSF_ERROR(DATA_TRNSF_APP, "Read Mqtt configuration data from shared memory successfully");
#endif
				/* Reset shared memory read  data flag */
				shm_read_tmout = 0;
			}
			/* Check read time out status */
			if ((shm_read_tmout > 0) && (shm_read_tmout < 3)){
				//;; /* Do nothing just wait & sync the process */
				sync();
				sleep(10);
			}
			else{
				shm_read_tmout = 0;
			}

			/* Close & free acquire semaphore */
			if (-1 != semaphore_ret_status){
				/* Release semaphore */
				post_sem(&(data_transfer_ctx->data_transfer_sem));
			}
			/* Close semaphore */
			close_sem(&(data_transfer_ctx->data_transfer_sem));
		}while(shm_read_tmout);

		/* Detect connected Usb & mount */
		ret_status =  detect_and_mount_connect_usb_device(data_transfer_ctx ,usb_node);
		if (ret_status == -1){
#ifdef ENABLE_DEBUG_LOG
			DTRNSF_DEBUG(DATA_TRNSF_APP, "Fail to detect Usb & create data backup directory in %s ", __func__);
#endif
			ret_status = -1;
			break;
		}
#if 0 /* just for debuging */
		//mqtt host name
		printf("mqtt host name  = %s\n",data_transfer_ctx->mqtt_config.host_name);
		//mqtt user name
		printf("mqtt host user  = %s\n", data_transfer_ctx->mqtt_config.host_user);
		//mqtt password
		printf("mqtt host password  = %s\n",data_transfer_ctx->mqtt_config.password);
		//mqtt port
		printf("mqtt port  = %d\n", data_transfer_ctx->mqtt_config.port);
		//mqtt qos
		printf("mqtt qos  = %d\n",data_transfer_ctx->mqtt_config.qos);
		//mqtt port
		printf("mqtt period  = %d\n", data_transfer_ctx->mqtt_config.period);
		//device name
		printf("recv device name = %s\n",data_transfer_ctx->nodex_info.dev_name);
		//device id
		printf("recv device id  = %s\n",data_transfer_ctx->nodex_info.dev_id);
		//customer
		printf("customer id  = %s\n",data_transfer_ctx->nodex_info.customer_id);
		//no of devices connected
		printf("device cont  name = %d\n",data_transfer_ctx->nodex_info.conct_dev_count);
		//connected usb path
		printf("usb path = %s\n",data_transfer_ctx->nodex_info.usb_path);

#endif
		/* Copy no of devices connect with NodeX device */
		data_transfer_ctx->no_of_concte_dev =  data_transfer_ctx->nodex_info.conct_dev_count;
		/* Check connected devices, it should greater then zero */
		if (data_transfer_ctx->no_of_concte_dev > 0){
#if 1
			//allocate memory to read all devices dir name
			data_transfer_ctx->concte_dev_dir_name = (char **)malloc(data_transfer_ctx->no_of_concte_dev * sizeof(char*));
			if (data_transfer_ctx->concte_dev_dir_name == NULL)
			{
				DTRNSF_ERROR(DATA_TRNSF_APP, " Error while  allocating memory in func %s ", __func__);
				ret_status = -1;
				break;
			}
			//to allocate size of directory name
			for(lp = 0; lp < data_transfer_ctx->no_of_concte_dev; lp++)
			{
				//allocate buffer to copy dir path name
				data_transfer_ctx->concte_dev_dir_name[lp] = (char *)malloc(10 * sizeof(char));
			}
#endif

			/* wait till all connected devices name find */
			/* Check available dir in temp */
			ret_status =  find_connect_dev_dir(data_transfer_ctx, TEMP_PATH);
			if (ret_status != 0){
#ifdef ENABLE_DEBUG_LOG
				DTRNSF_ERROR(DATA_TRNSF_APP, "Connected devices dir folder not find in %s", __func__);
#endif
				ret_status = -1;
				break;
			}
		}
		else /* Devices are not connected with NodeX */{
#ifdef ENABLE_DEBUG_LOG
			DTRNSF_INFO(DATA_TRNSF_APP, " Fail to find no of devices connected = %d ,  %s \n", data_transfer_ctx->no_of_concte_dev, __func__);
#endif
			ret_status = -1;
			break;
		}
		/* Initialize Mqtt */
		ret_status = mqtt_initialization(data_transfer_ctx);
		if (ret_status != 0){
#ifdef ENABLE_DEBUG_LOG
			DTRNSF_INFO(DATA_TRNSF_APP, "Failed to initialise mqtt client in func = %s", __func__);
#endif
		}

		//BUG0001
		//memset usb monitor struct
		memset (&(data_transfer_ctx->usb_monitor_state), 0x00 , sizeof(data_transfer_ctx->usb_monitor_state));
		//Created thread to monitor usb status
		if (0 != pthread_create(&(data_transfer_ctx->usb_monitor_state.usb_monitor_thread_id), NULL, &Monitor_usb_status_thread, data_transfer_ctx))
		{
			DTRNSF_INFO(DATA_TRNSF_APP, " Failed to create usb monitor thread\n");
			data_transfer_ctx->usb_monitor_state.usb_monitor_thread_init = 0;
			ret_status = -1;
			break;
		}
		data_transfer_ctx->usb_monitor_state.usb_monitor_thread_init = 1;
}

