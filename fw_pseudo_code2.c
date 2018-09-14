#define _LED_CONF_C
/****************************************************************************
* Filename          : led_conf.c
* Module            : LED  (TLC59116F)
* Author	    : Shilesh Babu
*-------------------------------------------------------------------------------
* Description       : This module enables the use of LED Module
*-------------------------------------------------------------------------------
* Processor(s)      : MSP430FR68xx
* Compiler(s)       : CCE Version: V6.1.1.00022(GCC)
*-------------------------------------------------------------------------------
$Header$
*******************************************************************************/

/*******************************************************************************
* Project Includes
*******************************************************************************/

#include "project.h"
#include "tsk_prot.h"
#include "tmr_prot.h"
#include "i2cm_prot.h"


static ubyte dbuff[4];
/*Function to Calculate the Brightness for LED*/

ubyte LED_GetBrightCount(ulword ulbright)
{
	ubyte ultemp;
	ultemp = ((ulbright*256)/100);
	return ultemp;
};
/*Initialization for LED*/
void Led_Init()
{
	dbuff[0]=TLC59116_MODE1;
	dbuff[1]=0x00;
	if(!(I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT)))
			I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT);			// Default Configuration. OSci ON and LED all call

	dbuff[0]=TLC59116_MODE2;
	dbuff[1]=0x20;//0x20;//0x28;
	if(!(I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT)))
			I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT);			// Group Control Period
#if 0 //TODO - RV - Not needed
	dbuff[0]=TLC59116_LEDOUT0;
	dbuff[1]=0xff;
	if(!(I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT)))
			I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT);

	dbuff[0]=TLC59116_LEDOUT1;
	dbuff[1]=0xff;
	if(!(I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT)))
			I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT);

	dbuff[0]=TLC59116_LEDOUT2;
	dbuff[1]=0xff;
	if(!(I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT)))
			I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT);

	dbuff[0]=TLC59116_LEDOUT3;
	dbuff[1]=0xff;
	if(!(I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT)))
			I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT);
#endif

}
/*LED all stop and Reset*/
void Led_All_Stop()
{
	dbuff[0]=TLC59116_LEDOUT0;							// Turning OFF all LED On This command
	dbuff[1]=0x00;
	if(!(I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT)))
			I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT);

	dbuff[0]=TLC59116_LEDOUT1;							// Turning OFF all LED On This command
	dbuff[1]=0x00;
	if(!(I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT)))
			I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT);


	dbuff[0]=TLC59116_LEDOUT2;							// Turning OFF all LED On This command
	dbuff[1]=0x00;
	if(!(I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT)))
			I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT);

	dbuff[0]=TLC59116_LEDOUT3;							// Turning OFF all LED On This command
	dbuff[1]=0x00;
	if(!(I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT)))
			I2CM_vI2CWriteTimeout(I2CM2_CHANNEL,TLC59116_ADDR,2,dbuff,I2CTIMEOUT);

}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/*
 *
 * main.c
 */
int main(void)
{

	WDTCTL = WDTPW | WDTHOLD; 			 /* Stop watchdog timer*/
	PM5CTL0 &= ~LOCKLPM5; 				/* low power mode disable  */
    	CSCTL0_H = CSKEY >> 8;               /* Unlock CS registers */
	CSCTL1 = DCOFSEL_6;                  /*  Set DCO to 8MHz */
	CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;  /* Set SMCLK = MCLK = DCO */
	CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     /* Set all dividers to 1 */
	CSCTL0_H = 0;                             /* Lock CS registers */
	MPUCTL0_H = MPUPW_H;
	MPUSEGB1 = 0X1800;//0840;
	MPUSEGB2 = 0x180A;
	MPUSAM = ;
	MPUCTL0_L = MPULOCK|MPUENA;
	MPUCTL0_H = 0;
	INT_DefaultIntClear();
	TSK_vRAMInit();
	SCI_vUARTInit();
	TMR_vTMRInit();
	INT_DefaultInit();

	 __bis_SR_register(GIE); 				 /* Enabling Global Interruot */
	 SCI_DefInit();

	 I2CM_I2CReset(I2CM1_CHANNEL,15);
	 I2CM_I2CReset(I2CM2_CHANNEL,15);
	 //SCI_vGetTempConfig();

	while(1)
	{
		if (TSK_uwTskStatus != CLEAR)
			TSK_vTskHandler(TSK_uwTskStatus);
		__no_operation();
	}
}




