#define _BOOTLOADER_C
/****************************************************************************
* Filename          : bootloader.c
* Module            : bootloader flash
* Author	    : Shilesh Babu
*-------------------------------------------------------------------------------
* Description       : This module used to received firmware from soc over uart and write it in  flash memory.
*-------------------------------------------------------------------------------
* Processor(s)      : MSP430FR68xx
* Compiler(s)       : CCE Version: V6.1.1.00022(GCC)
*-------------------------------------------------------------------------------
$Header$
*******************************************************************************/

/*******************************************************************************
* Project Includes
*******************************************************************************/
//if (signature == 0xFFFF)
	if ((*(unsigned int *)0x1800 == 0xFFFF) || *(unsigned int *)0x1800 == 0x5A5A)
	{
		__disable_interrupt();
		//signature = 0x5A5A;
		setup();
		// Erase flash memory for new FW.
		for (temp_addr = (unsigned char *)0x6400; temp_addr < (unsigned char *)0xFF01; temp_addr++)
			*(unsigned int *)(temp_addr+0x0) = 0xFFFF;
		for (temp_addr = (unsigned char *)0xFF90; temp_addr < (unsigned char *)0xFFFE; temp_addr++)
			*(unsigned int *)(temp_addr+0x0) = 0xFFFF;
		//write_flash_data(1);
		//__enable_interrupt();
		if (*(unsigned int *)0x1800 == 0x5A5A)
			send_command(CMD_REQUEST_FW);
		else
			send_command(REQUEST_FW_EVENT);
		while (done_flag)
		{
			// Only if buffer is full of data then only enter into the flashing mechanism
			if (buffer_full)
			{
				sum = 0;
				//This is the start of frame
				if (buffer[0] == ':')
				{
					// Get the data byte count. Starting frm: 1
					DataCount = parse_data(&buffer[1], 2);
					// Get the start address of the frame. Starting frm: 3
					temp_addr = parse_data(&buffer[3], 4);
					// Get the Data type. Starting frm: 7
					//if (parse_data(&buffer[7], 2))
					if (parse_data(&buffer[7], 2))
					{
						// This is the end of the file frame.
						//send_command(3);
						__disable_interrupt();
						done_flag = 0;
						// Need to replace the ISR teble and then Jump Main FW.
						//jumpMainFW();
					}	buffer_full = 0;
						}
						else
						{
							for (tempCnt = 0; tempCnt < DataCount; tempCnt++)
								tempCharArray[tempCnt] = (unsigned char)parse_data(&buffer[9+tempCnt*2], 2);
							//jumpAddress = *(unsigned int*)tempCharArray;
							*(unsigned int*)0x1802 = *(unsigned int*)tempCharArray;
//							JumpAddr = *(unsigned int*)0x1802;
							buffer_full = 0;
							send_command(CMD_PACKET_ACK);
						}
					}
				}
				else
				{
					if (reTxCounts++ < MAX_COUNTS)
					send_command(CMD_RE_TX);
//					send_command(CMD_RE_TX);
				}
			}
		}
	
		*(unsigned int *)0x1800 = 0xA5A5;
		__delay_cycles(800000);
		send_command(CMD_UPGRADE_DONE);
		__delay_cycles(800000);
		WDTCTL = 0x5600 | WDTHOLD;
	//	asm("	MOV.W jumpAddress, pc");
	}
	else if(*(unsigned int *)0x1800 == 0xA5A5)
        {
		jumpAddress  = *(unsigned int *)0x1802;
		//asm("	MOV.W JumpAddr, pc");
		asm("	MOV.W jumpAddress, pc");
	}
}

void jumpMainFW()
{
	WDTCTL = WDTPW | WDTHOLD;						// Stop watchdog timer
 }
unsigned char calc_crc(unsigned char * data, int count)
{
	unsigned char i = 0;
	unsigned char checksum = 0;
	for (i = 0; i < count; i++)
		sum += (unsigned char)parse_data(&data[i*2], 2);
	return ((checksum == sum) ? 1:0);
}

