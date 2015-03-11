/*******************************************************************************

   Filename:    M29W_device_driver.c
   Description: Library routines for the M29W
                128Mb 256Mb 512Mb (Uniform Block) Flash Memory drivers
                in different configurations.

   Version:     1.0
   Author:      Micron

   Copyright (c) 2014 Micron Technology Incorporated

   THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
   EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTY
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK
   AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE
   PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,
   REPAIR OR CORRECTION.
********************************************************************************

   Version History.

   Ver.   Date        Comments

   1.0    2014/02/24  Qualified Version
********************************************************************************

   This source file provides library C code for using the M29W flash devices.
   The following device is supported in the code:
      M29W128, M29W256, M29W512

   This file can be used to access the devices in 8bit and 16bit mode.

   The following functions are available in this library:
      FlashInit() to initialize the driver

   Standard functions:
      Flash(BlockErase, ParameterType)            to erase one block
      Flash(CheckBlockProtection, ParameterType)  to check whether a given block is protected
      Flash(CheckCompatibility, ParameterType)    to check the compatibility of the flash
      Flash(ChipErase, ParameterType)             to erase the whole chip
      Flash(ChipUnprotect, ParameterType)         to unprotect the whole chip
      Flash(GroupProtect, ParameterType)          to unprotect a blocks group
      Flash(Program, ParameterType)               to program an array of elements
      Flash(Read, ParameterType)                  to read from the flash device
      Flash(ReadCfi, ParameterType)               to read CFI information from the flash
      Flash(ReadDeviceId, ParameterType)          to get the Device ID from the device
      Flash(ReadManufacturerCode, ParameterType)  to get the Manufacture Code from the device
      Flash(Reset, ParameterType)                 to reset the flash for normal memory access
      Flash(Resume, ParameterType)                to resume a suspended erase
      Flash(SingleProgram, ParameterType)         to program a single element
      Flash(Suspend, ParameterType)               to suspend an erase
      Flash(Write, ParameterType)                 to write a value to the flash device

   Use Flash_2Gb() for multi-die parts instead of Flash() described above.


  Fast program functions:
      FlashBufferProgram()
      FlashBufferProgramAbort()
      FlashBufferProgramConfirm()
      FlashUnlockBypass( void )
      FlashUnlockBypassProgram ()
      FlashUnlockBypassReset()
      FlashUnlockBypassBlockErase()
      FlashUnlockBypassBufferProgram()
      FlashUnlockBypassChipErase()

  Protection functions:
      FlashCheckBlockProtection()
      FlashEnterExtendedBlock()
      FlashExitExtendedBlock()
      FlashReadExtendedBlockVerifyCode()
      FlashCheckProtectionMode()
      FlashSetNVProtectionMode()
      FlashSetPasswordProtectionMode()
      FlashSetExtendedRomProtection()
      FlashSetNVPBLockBit()
      FlashCheckNVPBLockBit()
      FlashClearBlockNVPB()
      FlashSetBlockNVPB( )
      FlashCheckBlockVPB( )
      FlashClearBlockVPB()
      FlashSetBlockVPB()
      FlashPasswordProgram()
      FlashVerifyPassword()
      FlashPasswordProtectionUnlock()
      FlashExitProtection()

   For further information consult the Data Sheet and the Application Note.
   The Application Note gives information about how to modify this code for
   a specific application.

   The hardware specific functions which may need to be modified by the user are:

      FlashWrite() for writing an element (uCPUBusType) to the flash
      FlashRead()  for reading an element (uCPUBusType) from the flash

   A list of the error conditions can be found at the end of the header file.

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef PLATFORM_ORION
#include "rodan_mmrb.h"
#else
#define UPDIE_ACTIVATE
#define UPDIE_ENABLE
#define UPDIE_DISABLE
#endif

#include "M29W_device_driver.h"

#ifdef TIME_H_EXISTS
   #include <time.h>
#endif

/******************************************************************************
    Global variables
*******************************************************************************/
ErrorInfoType eiErrorInfo;

/************************* Static Global variables ****************************/

static NMX_uint32  udCounter; /* Nr of elapsed cycles from the last call
                                  of FLASH_START_TIMER() */

NMX_uint32  udDeviceSize      = 0;  /* Flash size in uCPUBusType elems */
uBlockType ublNumBlocks       = 0;  /* Number of Blocks in the device  */
uBlockType ublBlocksPerDie    = 0;  /* used for multiple-die devices   */
NMX_uint32  udBlockSize       = 0;  /* Block size of the device        */
NMX_uint32  udDieSize         = 0;	/* used for multiple-die devices   */
NMX_uint32  udBufferSize	  = 0;  /* size of buffer for programming  */

volatile uCPUBusType *BASE_ADDR  = FLASH_BASE_ADDRESS;
/* BASE_ADDR is the base or start address of the flash, see the functions
   FlashRead and FlashWrite(). Some applications which require a more
   complicated FlashRead() or FlashWrite() may not use BASE_ADDR */


/* This section is obsolete.  Buffer size is now read from the CFI. */

// 8-bit mode
// The maximum number of cycles in the buffer program command sequence is 261. 256 is the maximum
// number of bytes to be programmed during the Write to Buffer Program operation.
// Datasheet Version 10 - May 2010
//#ifdef USE_8BIT_FLASH
//	NMX_uint32  udBufferSize       = 0x100;  /* Block size of the device M29EW x8 */
//#endif
// 16-bit mode
// Addresses must lie within the range from the (start address+1) to the (start address + N-1).
// Optimum programming performance and lower power usage are obtained
// by aligning the starting address at the beginning of a 512-word boundary
// Note: All the addresses used in the Write to Buffer Program operation must lie
// within the block boundary.
// Datasheet Version 10 - May 2010
//#ifdef USE_16BIT_FLASH
//	NMX_uint32  udBufferSize       = 0x200;  /* Block size of the device M29EW x16*/
//#endif

/******************* Functions Internally used ********************************/


/*******************************************************************************
Use this for multi-die parts only.

Function:     ReturnType Flash_2Gb( CommandType cmdCommand, ParameterType *fp )
Arguments:    cmdCommand is an enum which contains all the available function
   commands of the SW driver.
              fp is a (union) parameter struct for all flash command parameters
Return Value: The function returns the following conditions:

   Flash_AddressInvalid
   Flash_BlockEraseFailed
   Flash_BlockNrInvalid
   Flash_BlockProtected
   Flash_BlockProtectFailed
   Flash_BlockProtectionUnclear
   Flash_BlockUnprotected
   Flash_CfiFailed
   Flash_ChipEraseFailed
   Flash_ChipUnprotectFailed
   Flash_FunctionNotSupported
   Flash_GroupProtectFailed
   Flash_NoInformationAvailable
   Flash_OperationOngoing
   Flash_OperationTimeOut
   Flash_ProgramFailed
   Flash_ResponseUnclear
   Flash_SpecificError
   Flash_Success
   Flash_WrongType

Description:  This function is used to access all functions provided with the
   current flash chip.

Pseudo Code:
   Step 1: Select the right action using the cmdCommand parameter
   Step 2: Execute the Flash Function
   Step 3: Return the Error Code
*******************************************************************************/
ReturnType Flash_2Gb( CommandType cmdCommand, ParameterType *fp ) {
   ReturnType  rRetVal;
   uCPUBusType  ucDeviceId, ucManufacturerCode;
	volatile uCPUBusType  *oldBaseAddress = BASE_ADDR;
	NMX_uint32  oldNumBlocks = ublNumBlocks;
	ReturnType *rpResults;
	udword offset;
	int i;

   switch (cmdCommand) {
      case BlockErase:
	  		if ((*fp).BlockErase.ublBlockNr >= ublNumBlocks)
				return Flash_BlockNrInvalid;
			if ((*fp).BlockErase.ublBlockNr >= ublBlocksPerDie)
			{
				BASE_ADDR += udDieSize;
			}
         rRetVal = FlashBlockErase( (*fp).BlockErase.ublBlockNr % ublBlocksPerDie);
         break;

      case BufferProgram:
			/* if the data is all in one die we can call FlashProgram(). */
			if ( ((*fp).Program.udAddrOff+(*fp).Program.udNrOfElementsInArray
				<= (udDieSize<<1)) ||
				((*fp).Program.udAddrOff >= (udDieSize<<1)) )
			{
				if ((*fp).Program.udAddrOff >= udDieSize)
					BASE_ADDR += udDieSize;
				rRetVal = FlashBufferProgram( (*fp).Program.udMode,
												(*fp).Program.udAddrOff,
												(*fp).Program.udNrOfElementsInArray,
												(*fp).Program.pArray );
			}
			else
			{
				/* we need to break the data up into lower die and upper
				 * die pieces and program each piece individually */
				NMX_uint32 size = (udDieSize<<1) - (*fp).Program.udAddrOff - 1;
				rRetVal = FlashBufferProgram( (*fp).Program.udMode,
												(*fp).Program.udAddrOff,
												size,
												(*fp).Program.pArray );
				if (rRetVal == Flash_Success)
				{
					BASE_ADDR += udDieSize;
					rRetVal = FlashBufferProgram( (*fp).Program.udMode,
													0,
													(*fp).Program.udNrOfElementsInArray-size,
													(char*)((*fp).Program.pArray)+size );
				}

			}
         break;
      case CheckBlockProtection:
	  		if ((*fp).BlockErase.ublBlockNr >= ublNumBlocks)
				return Flash_BlockNrInvalid;
			if ((*fp).BlockErase.ublBlockNr >= ublBlocksPerDie)
			{
				BASE_ADDR += udDieSize;
			}
         rRetVal = FlashCheckBlockProtection( (*fp).CheckBlockProtection.ublBlockNr
				% ublBlocksPerDie);
         break;

      case CheckCompatibility:
         rRetVal = FlashCheckCompatibility();
         break;

      case ChipErase:
		 ublNumBlocks = ublBlocksPerDie;
         rRetVal = FlashChipErase( (*fp).ChipErase.rpResults );
			if (rRetVal == Flash_Success)
			{
				BASE_ADDR += udDieSize;
				rpResults = ((*fp).ChipErase.rpResults != NULL) ?
					NULL :
					(*fp).ChipErase.rpResults + sizeof(ReturnType)*ublBlocksPerDie;
				rRetVal = FlashChipErase((*fp).ChipErase.rpResults );
			}
			ublNumBlocks = oldNumBlocks;
         break;

      case Program:
			/* if the data is all in one die we can call FlashProgram(). */
			if ( ((*fp).Program.udAddrOff+(*fp).Program.udNrOfElementsInArray
				<= (udDieSize<<1)) ||
				((*fp).Program.udAddrOff >= (udDieSize<<1)) )
			{
				if ((*fp).Program.udAddrOff >= udDieSize)
					BASE_ADDR += udDieSize;
         	rRetVal = FlashProgram( (*fp).Program.udMode,
                                 (*fp).Program.udAddrOff,
                                 (*fp).Program.udNrOfElementsInArray,
                                 (*fp).Program.pArray );
			}
			else
			{
				/* we need to break the data up into lower die and upper
				 * die pieces and program each piece individually */
				NMX_uint32 size = (udDieSize<<1) - (*fp).Program.udAddrOff - 1;
				rRetVal = FlashProgram( (*fp).Program.udMode,
												(*fp).Program.udAddrOff,
												size,
												(*fp).Program.pArray );
				if (rRetVal == Flash_Success)
				{
					BASE_ADDR += udDieSize;
					rRetVal = FlashProgram( (*fp).Program.udMode,
													0,
													(*fp).Program.udNrOfElementsInArray-size,
													(char*)((*fp).Program.pArray)+size );
				}
			}
         break;

      case Read:
         (*fp).Read.ucValue = FlashRead( (*fp).Read.udAddrOff );
         rRetVal = Flash_Success;
         break;

      case ReadCfi:
         rRetVal = FlashReadCfi( (*fp).ReadCfi.uwCfiFunc, &((*fp).ReadCfi.ucCfiValue) );
         break;

      case ReadDeviceId:
         rRetVal = FlashReadDeviceId(&ucDeviceId);
         (*fp).ReadDeviceId.ucDeviceId = ucDeviceId;
         break;

      case ReadManufacturerCode:
         rRetVal = FlashReadManufacturerCode(&ucManufacturerCode);
         (*fp).ReadManufacturerCode.ucManufacturerCode = ucManufacturerCode;
         break;

      case Reset:
   		for (i=0; i<NUM_DEVICES; ++i)
   		{
				rRetVal = FlashReset();
				BASE_ADDR += udDieSize;
			}
			break;

      case Resume:
   		for (i=0; i<NUM_DEVICES; ++i)
   		{
				rRetVal = FlashResume();
				BASE_ADDR += udDieSize;
			}
			break;

      case SingleProgram:
	  		offset = (*fp).SingleProgram.udAddrOff;
			if ((*fp).SingleProgram.udAddrOff >= udDieSize)
			{
				BASE_ADDR += udDieSize;
				offset -= udDieSize;
			}
         rRetVal = FlashSingleProgram( offset, (*fp).SingleProgram.ucValue );
         break;

      case Suspend:
   		for (i=0; i<NUM_DEVICES; ++i)
   		{
				rRetVal = FlashSuspend();
				BASE_ADDR += udDieSize;
			}
			break;

      case Write:
         FlashWrite( (*fp).Write.udAddrOff, (*fp).Write.ucValue );
         rRetVal = Flash_Success;
         break;

      default:
         rRetVal = Flash_FunctionNotSupported;
         break;

   } /* EndSwitch */
	UPDIE_DISABLE;
	BASE_ADDR = oldBaseAddress;
   return rRetVal;
} /* EndFunction Flash */

/*******************************************************************************
Function:     ReturnType Flash( CommandType cmdCommand, ParameterType *fp )
Arguments:    cmdCommand is an enum which contains all the available function
   commands of the SW driver.
              fp is a (union) parameter struct for all flash command parameters
Return Value: The function returns the following conditions:

   Flash_AddressInvalid
   Flash_BlockEraseFailed
   Flash_BlockNrInvalid
   Flash_BlockProtected
   Flash_BlockProtectFailed
   Flash_BlockProtectionUnclear
   Flash_BlockUnprotected
   Flash_CfiFailed
   Flash_ChipEraseFailed
   Flash_ChipUnprotectFailed
   Flash_FunctionNotSupported
   Flash_GroupProtectFailed
   Flash_NoInformationAvailable
   Flash_OperationOngoing
   Flash_OperationTimeOut
   Flash_ProgramFailed
   Flash_ResponseUnclear
   Flash_SpecificError
   Flash_Success
   Flash_WrongType

Description:  This function is used to access all functions provided with the
   current flash chip.

Pseudo Code:
   Step 1: Select the right action using the cmdCommand parameter
   Step 2: Execute the Flash Function
   Step 3: Return the Error Code
*******************************************************************************/
ReturnType Flash( CommandType cmdCommand, ParameterType *fp ) {
   ReturnType  rRetVal;
   uCPUBusType  ucDeviceId, ucManufacturerCode;

   switch (cmdCommand) {
      case BlockErase:
         rRetVal = FlashBlockErase( (*fp).BlockErase.ublBlockNr );
         break;

      case BufferProgram:
           rRetVal = FlashBufferProgram( (*fp).Program.udMode,
                                   (*fp).Program.udAddrOff,
                                   (*fp).Program.udNrOfElementsInArray,
                                   (*fp).Program.pArray );
           break;
      case CheckBlockProtection:
         rRetVal = FlashCheckBlockProtection( (*fp).CheckBlockProtection.ublBlockNr );
         break;

      case CheckCompatibility:
         rRetVal = FlashCheckCompatibility();
         break;

      case ChipErase:
         rRetVal = FlashChipErase( (*fp).ChipErase.rpResults );
         break;

      case Program:
         rRetVal = FlashProgram( (*fp).Program.udMode,
                                 (*fp).Program.udAddrOff,
                                 (*fp).Program.udNrOfElementsInArray,
                                 (*fp).Program.pArray );

         break;

      case Read:
         (*fp).Read.ucValue = FlashRead( (*fp).Read.udAddrOff );
         rRetVal = Flash_Success;
         break;

      case ReadCfi:
         rRetVal = FlashReadCfi( (*fp).ReadCfi.uwCfiFunc, &((*fp).ReadCfi.ucCfiValue) );
         break;

      case ReadDeviceId:
         rRetVal = FlashReadDeviceId(&ucDeviceId);
         (*fp).ReadDeviceId.ucDeviceId = ucDeviceId;
         break;

      case ReadManufacturerCode:
         rRetVal = FlashReadManufacturerCode(&ucManufacturerCode);
         (*fp).ReadManufacturerCode.ucManufacturerCode = ucManufacturerCode;
         break;

      case Reset:
         rRetVal = FlashReset();
         break;

      case Resume:
         rRetVal = FlashResume();
         break;

      case SingleProgram:
         rRetVal = FlashSingleProgram( (*fp).SingleProgram.udAddrOff, (*fp).SingleProgram.ucValue );
         break;

      case Suspend:
         rRetVal = FlashSuspend();
         break;

      case Write:
         FlashWrite( (*fp).Write.udAddrOff, (*fp).Write.ucValue );
         rRetVal = Flash_Success;
         break;

      default:
         rRetVal = Flash_FunctionNotSupported;
         break;

   } /* EndSwitch */
   return rRetVal;
} /* EndFunction Flash */


/*******************************************************************************
Function: ReturnType FlashInit(uBlockType ublFirstBlock,uBlockType ublLastBlock)

Parameters:
   ublFirstBlock the number of the first block of the range to be unprotected;
   ublLastBlock  the number of the last block of the range to be unprotected.

Return Values:
   Flash_Success
   Flash_BlockNrInvalid
   Flash_BlockUnprotectFailed
   Flash_InvalidParameter
   Flash_UnknownDevice

Description:
   This function has to be used in order to initialize the driver.

Pseudo Code:
   Step 1: Send the Read CFI Command
   Step 2: Check that the CFI interface is operable
   Step 3: Initialize all global variables
   Step 4: Unprotect the given flash area
   Step 5: Return error condition
*******************************************************************************/
ReturnType  FlashInit ( void ){

   ReturnType  rRetVal = Flash_Success;
   NMX_uint32   udPrimaryAlgOffset=0;
   uCPUBusType ucPrimaryCommandSetLow, ucPrimaryCommandSetHigh;

   /* Step 1: Send the Read CFI Command */
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */

   FlashWrite(0x0055, CMD(0x0098));

   /* Step 2: Check that the CFI interface is operable */
   if( ((FlashRead( 0x0010 ) & CMD(0x00FF)) != CMD(0x0051)) ||
       ((FlashRead( 0x0011 ) & CMD(0x00FF)) != CMD(0x0052)) ||
       ((FlashRead( 0x0012 ) & CMD(0x00FF)) != CMD(0x0059)) )
      return Flash_CfiFailed;

   /* Check the device family */
   ucPrimaryCommandSetLow  = FlashRead( 0x013 ) & CMD(0x00FF);
   ucPrimaryCommandSetHigh = FlashRead( 0x014 ) & CMD(0x00FF);

   /* Primary Algorithm-specific Extended Query Table offset */
   udPrimaryAlgOffset = ( (FlashRead(0x0015) & CMD(0x00FF) ) +
              ( (FlashRead(0x0016) & CMD(0x00FF) ) << 8 ) );

   /* Step 3: Initialize all global variables */

   /*******************************************/
   /*           NUMBER OF BLOCKS              */
   /*******************************************/
   /* Retrieve the number of blocks */
   ublNumBlocks = ( ( FlashRead(0x002D) & CMD(0x00FF) ) +
                  ( ( FlashRead(0x002E) & CMD(0x00FF) ) << 8 ) ) + 1;


   /*******************************************/
   /*           BLOCK SIZE                    */
   /*******************************************/
   /* Retrieve the block size in bytes... */
   udBlockSize = ( ( ( FlashRead(0x002F) & CMD(0x00FF) ) +
                   ( ( FlashRead(0x0030) & CMD(0x00FF) ) << 8 ) ) << 8 );

   /* ... and convert it in uCPUBusType */
   udBlockSize = (udBlockSize * NUM_DEVICES) / sizeof(uCPUBusType);


   /*******************************************/
   /*            DEVICE SIZE                  */
   /*******************************************/
   udDeviceSize = ublNumBlocks * udBlockSize;

   /*******************************************/
   /*        DEVICE DIE INFORMATION           */
   /*******************************************/
	udDieSize = udDeviceSize / NUM_DEVICES;
	ublBlocksPerDie = ublNumBlocks / NUM_DEVICES;

   /*******************************************/
   /*        PROGRAM BUFFER SIZE              */
   /*******************************************/
	udBufferSize = FlashRead(0x2A) + (FlashRead(0x2B)<<8);
	udBufferSize = (1<<udBufferSize) / sizeof(uCPUBusType);	// WORDS or DWORDS!
   /* Return the flash to Read Array mode */
   FlashReset();

   /* Step 5: Return error condition */
   return rRetVal;

} /* EndFunction FlashInit */


/*******************************************************************************
Function:     ReturnType FlashBlockErase( uBlockType ublBlockNr )
Arguments:    ublBlockNr is the number of the Block to be erased.
Return Value: The function returns the following conditions:
   Flash_Success
   Flash_BlockEraseFailed
   Flash_BlockNrInvalid
   Flash_BlockProtected
   Flash_OperationTimeOut

Description:  This function can be used to erase the Block specified in ublBlockNr.
   The function checks that the block nr is within the valid range and not protected
   before issuing the erase command, otherwise the block will not be erased and an
   error code will be returned.
   The function returns only when the block is erased. During the Erase Cycle the
   Data Toggle Flow Chart of the Datasheet is followed. The polling bit, DQ7, is not
   used.

Pseudo Code:
   Step 1:  Check that the block number exists
   Step 2:  Check if the block is protected
   Step 3:  Write Block Erase command
   Step 4:  Wait for the timer bit to be set
   Step 5:  Follow Data Toggle Flow Chart until the Program/Erase Controller is finished
   Step 6:  Return to Read mode (if an error occurred)
*******************************************************************************/
ReturnType FlashBlockErase( uBlockType ublBlockNr) {

   ReturnType rRetVal = Flash_Success; /* Holds return value: optimistic initially! */

   /* Step 1: Check for invalid block. */
   if( ublBlockNr >= ublNumBlocks ) /* Check specified blocks <= ublNumBlocks */
      return Flash_BlockNrInvalid;

   /* Step 2: Check if the block is protected */
   if ( FlashCheckBlockProtection(ublBlockNr) == Flash_BlockProtected)
      return Flash_BlockProtected;

   /* Step 3: Write Block Erase command */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) );
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) );
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0080) );
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) );
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) );
   FlashWrite( BlockOffset(ublBlockNr), (uCPUBusType)CMD(0x0030) );

   /* Step 4: Wait for the Erase Timer Bit (DQ3) to be set */
   FlashTimeOut(0); /* Initialize TimeOut Counter */
   while( !(FlashRead( BlockOffset(ublBlockNr) ) & CMD(0x0008) ) ){
      if (FlashTimeOut(5) == Flash_OperationTimeOut) {
         FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
         return Flash_OperationTimeOut;
      } /* EndIf */
   } /* EndWhile */

   /* Step 5: Follow Data Toggle Flow Chart until Program/Erase Controller completes */
   if( FlashDataToggle(10) !=  Flash_Success ) {
      /* Step 6: Return to Read mode (if an error occurred) */
      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
      rRetVal=Flash_BlockEraseFailed;
   } /* EndIf */
   return rRetVal;
} /* EndFunction FlashBlockErase */





/*******************************************************************************
Function:     ReturnType FlashChipErase( ReturnType *rpResults )
Arguments:    rpResults is a pointer to an array where the results will be
   stored. If rpResults == NULL then no results have been stored.
Return Value: The function returns the following conditions:
   Flash_Success
   Flash_ChipEraseFailed

Description: The function can be used to erase the whole flash chip. Each Block
   is erased in turn. The function only returns when all of the Blocks have
   been erased. If rpResults is not NULL, it will be filled with the error
   conditions for each block.

Pseudo Code:
   Step 1: Check if some blocks are protected
   Step 2: Send Chip Erase Command
   Step 3: Check for blocks erased correctly
   Step 4: Return to Read mode (if an error occurred)
*******************************************************************************/
ReturnType FlashChipErase( ReturnType *rpResults ) {

   ReturnType rRetVal = Flash_Success, /* Holds return value: optimistic initially! */
              rProtStatus; /* Holds the protection status of each block */
   uBlockType ublCurBlock; /* Used to tack current block in a range */

   /* Step 1: Check if some blocks are protected */
   for (ublCurBlock=0; ublCurBlock < ublNumBlocks;ublCurBlock++) {
      if (FlashCheckBlockProtection(ublCurBlock)==Flash_BlockProtected) {
         rProtStatus = Flash_BlockProtected;
	 rRetVal = Flash_ChipEraseFailed;
      } else
         rProtStatus =Flash_Success;
      if (rpResults != NULL)
         rpResults[ublCurBlock] = rProtStatus;
   } /* Next ublCurBlock */

   /* Step 2: Send Chip Erase Command */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) );
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) );
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0080) );
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) );
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) );
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0010) );

   /* Step 3: Check for blocks erased correctly */
   if( FlashDataToggle(120)!=Flash_Success){
      rRetVal= Flash_ChipEraseFailed;
      if (rpResults != NULL){
         for (ublCurBlock=0;ublCurBlock < ublNumBlocks;ublCurBlock++)
            if (rpResults[ublCurBlock]==Flash_Success)
               rpResults[ublCurBlock] = FlashCheckBlockEraseError(ublCurBlock);
      } /* EndIf */
         /* Step 4: Return to Read mode (if an error occurred) */
         FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
   } /* EndIf */
   return rRetVal;

} /* EndFunction FlashChipErase */






/*******************************************************************************
Function:      ReturnType FlashCheckCompatibility( void )
Arguments:     None
Return Values: The function returns the following conditions:
   Flash_Success
   Flash_WrongType
Description:   This function checks the compatibility of the device with
   the SW driver.

Pseudo Code:
   Step 1:  Read the Device Id
   Step 2:  Read the Manufacturer Code
   Step 3:  Check the results
*******************************************************************************/
ReturnType FlashCheckCompatibility( void ) {
   ReturnType  rRetVal, rCheck1, rCheck2; /* Holds the results of the Read operations */
   uCPUBusType ucDeviceId, ucManufacturerCode; /* Holds the values read */

   rRetVal =  Flash_WrongType;

   /* Step 1:  Read the Device Id */
   rCheck1 =  FlashReadDeviceId( &ucDeviceId );

   /* Step 2:  Read the ManufactureCode */
   rCheck2 =  FlashReadManufacturerCode( &ucManufacturerCode );

   /* Step 3:  Check the results */
   if (    (rCheck1 == Flash_Success) && (rCheck2 == Flash_Success)
      && (ucDeviceId == EXPECTED_DEVICE)  && (ucManufacturerCode == MANUFACTURER)  )
      rRetVal = Flash_Success;
   return rRetVal;
} /* EndFunction FlashCheckCompatibility */





/*******************************************************************************
Function:     ReturnType FlashCheckBlockEraseError( uBlockType  ublBlock )
Arguments:    ublBlock specifies the block to be checked
Return Value:
         Flash_Success
         FlashBlockEraseFailed

Description:  This function can only be called after an erase operation which
   has failed the FlashDataPoll() function. It must be called before the reset
   is made. The function reads bit 2 of the Status Register to check if the block
   has erased successfully or not. Successfully erased blocks should have DQ2
   set to 1 following the erase. Failed blocks will have DQ2 toggle.

Pseudo Code:
   Step 1: Read DQ2 in the block twice
   Step 2: If they are both the same then return Flash_Success
   Step 3: Else return Flash_BlockEraseFailed
*******************************************************************************/
static ReturnType FlashCheckBlockEraseError( uBlockType ublBlock ){

   uCPUBusType ucFirstRead, ucSecondRead; /* Two variables used for clarity*/

   /* Step 1: Read DQ2 in the block twice */
   ucFirstRead  = FlashRead( BlockOffset(ublBlock) ) & CMD(0x0004);
   ucSecondRead = FlashRead( BlockOffset(ublBlock) ) & CMD(0x0004);
   /* Step 2: If they are the same return Flash_Success */
   if( ucFirstRead == ucSecondRead )
      return Flash_Success;
   /* Step 3: Else return Flash_BlockEraseFailed */
   return Flash_BlockEraseFailed;
} /*EndFunction FlashCheckBlockEraseError*/



/*******************************************************************************
Function:ReturnType FlashMultipleBlockErase(uBlockType ublNumBlocks,uBlockType
                    *ublpBlock,ReturnType *rpResults)
Arguments:   ublNumBlocks holds the number of blocks in the array ubBlock
   ublpBlocks is an array containing the blocks to be erased.
   rpResults is an array that it holds the results of every single block
   erase.
   Every elements of rpResults will be filled with below values:
      Flash_Success
      Flash_BlockEraseFailed
      Flash_BlockProtected
   If a time-out occurs because the MPU is too slow then the function returns
   Flash_MpuTooSlow

Return Value: The function returns the following conditions:
   Flash_Success
   Flash_BlockEraseFailed
   Flash_BlockNrInvalid
   Flash_OperationTimeOut
   Flash_SpecificError      : if a no standard error occour.In this case the
      field sprRetVal of the global variable eiErrorInfo will be filled
      with Flash_MpuTooSlow when any blocks are not erased because DQ3
      the MPU is too slow.


Description: This function erases up to ublNumBlocks in the flash. The blocks
   can be listed in any order. The function does not return until the blocks are
   erased. If any blocks are protected or invalid none of the blocks are erased,
   in this casse the function return Flash_BlockEraseFailed.
   During the Erase Cycle the Data Toggle Flow Chart of the Data Sheet is
   followed. The polling bit, DQ7, is not used.

Pseudo Code:
   Step 1:  Check for invalid block
   Step 2:  Check if some blocks are protected
   Step 3:  Write Block Erase command
   Step 4:  Check for time-out blocks
   Step 5:  Wait for the timer bit to be set.
   Step 6:  Follow Data Toggle Flow Chart until Program/Erase Controller has
            completed
   Step 7:  Return to Read mode (if an error occurred)

*******************************************************************************/
ReturnType FlashMultipleBlockErase(uBlockType ublNumBlocks,uBlockType *ublpBlock,ReturnType *rpResults) {

   ReturnType rRetVal = Flash_Success, /* Holds return value: optimistic initially! */
              rProtStatus; /* Holds the protection status of each block */
   uBlockType ublCurBlock; /* Range Variable to track current block */
   uCPUBusType ucFirstRead, ucSecondRead; /* used to check toggle bit DQ2 */

   /* Step 1: Check for invalid block. */
   if( ublNumBlocks > ublNumBlocks ){ /* Check specified blocks <= ublNumBlocks */
      eiErrorInfo.sprRetVal = FlashSpec_TooManyBlocks;
      return Flash_SpecificError;
   } /* EndIf */

   /* Step 2: Check if some blocks are protected */
   for (ublCurBlock=0; ublCurBlock < ublNumBlocks;ublCurBlock++) {
      if (FlashCheckBlockProtection(ublCurBlock)==Flash_BlockProtected) {
         rProtStatus = Flash_BlockProtected;
	 rRetVal = Flash_BlockEraseFailed;
      } else
         rProtStatus =Flash_Success;
         if (rpResults != NULL)
            rpResults[ublCurBlock] = rProtStatus;
   } /* Next ublCurBlock */

   /* Step 3: Write Block Erase command */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) );
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) );
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0080) );
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) );
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) );

   /* DSI!: Disable Interrupt, Time critical section. Additional blocks must be added
            every 50us */

   for( ublCurBlock = 0; ublCurBlock < ublNumBlocks; ublCurBlock++ ) {
      FlashWrite( BlockOffset(ublpBlock[ublCurBlock]), (uCPUBusType)CMD(0x0030) );
      /* Check for Erase Timeout Period (is bit DQ3 set?)*/
      if( (FlashRead( BlockOffset(ublpBlock[0]) ) & CMD(0x0008)) != 0 )
         break; /* Cannot set any more blocks due to timeout */
   } /* Next ublCurBlock */

   /* ENI!: Enable Interrupt */

   /* Step 4: Check for time-out blocks */
   /* if timeout occurred then check if current block is erasing or not */
   /* Use DQ2 of status register, toggle implies block is erasing */
   if ( ublCurBlock < ublNumBlocks ) {
      ucFirstRead = FlashRead( BlockOffset(ublpBlock[ublCurBlock]) ) & CMD(0x0004);
      ucSecondRead = FlashRead( BlockOffset(ublpBlock[ublCurBlock]) ) & CMD(0x0004);
      if( ucFirstRead != ucSecondRead )
         ublCurBlock++; /* Point to the next block */

      if( ublCurBlock < ublNumBlocks ){
         /* Indicate that some blocks have been timed out of the erase list */
         rRetVal = Flash_SpecificError;
         eiErrorInfo.sprRetVal = FlashSpec_MpuTooSlow;
      } /* EndIf */

      /* Now specify all other blocks as not being erased */
      while( ublCurBlock < ublNumBlocks ) {
         rpResults[ublCurBlock++] = Flash_BlockEraseFailed;
      } /* EndWhile */
   } /* EndIf ( ublCurBlock < ublNumBlocks ) */


   /* Step 5: Wait for the Erase Timer Bit (DQ3) to be set */
   FlashTimeOut(0); /* Initialize TimeOut Counter */
   while( !(FlashRead( BlockOffset(ublpBlock[0]) ) & CMD(0x0008) ) ){
      if (FlashTimeOut(5) == Flash_OperationTimeOut) {
         FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction
                                                              cycle method */
         return Flash_OperationTimeOut;
      } /* EndIf */
   } /* EndWhile */

   /* Step 6: Follow Data Toggle Flow Chart until Program/Erase Controlle completes */
   if( FlashDataToggle(ublNumBlocks*10) !=  Flash_Success ) {
      if (rpResults != NULL) {
         for (ublCurBlock=0;ublCurBlock < ublNumBlocks;ublCurBlock++)
            if (rpResults[ublCurBlock]==Flash_Success)
               rpResults[ublCurBlock] = FlashCheckBlockEraseError(ublCurBlock);
      } /* EndIf */

      /* Step 7: Return to Read mode (if an error occurred) */
      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
      rRetVal=Flash_BlockEraseFailed;
   } /* EndIf */
   return rRetVal;

} /* EndFunction FlashMultipleBlockErase */




/*******************************************************************************
Function:     ReturnType FlashProgram( udword udMode, udword udAddrOff,
                                       udword udNrOfElementsInArray, void *pArray )
Arguments:    udMode changes between programming modes
   udAddrOff is the address offset into the flash to be programmed
   udNrOfElementsInArray holds the number of elements (uCPUBusType) in the array.
   pArray is a void pointer to the array with the contents to be programmed.

Return Value: The function returns the following conditions:
   Flash_Success
   Flash_AddressInvalid
   Flash_ProgramFailed

Description: This function is used to program an array into the flash. It does
   not erase the flash first and will not produce proper results, if the block(s)
   are not erased first.
   Any errors are returned without any further attempts to program other addresses
   of the device. The function returns Flash_Success when all addresses have
   successfully been programmed.

   Note: Two program modes are available:
   - udMode = 0, Normal Program Mode
   The number of elements (udNumberOfElementsInArray) contained in pArray
   are programmed directly to the flash starting with udAddrOff.
   - udMode = 1, Single Value Program Mode
? Only the first value of the pArray will be programmed to the flash
   starting from udAddrOff.
   .
Pseudo Code:
   Step  1:  Check whether the data to be programmed are are within the
             Flash memory
   Step  2:  Determine first and last block to program
   Step  3:  Check protection status for the blocks to be programmed
   Step  4:  Issue the Unlock Bypass command
   Step  5:  Unlock Bypass Program command
   Step  6:  Wait until Program/Erase Controller has completed
   Step  7:  Return to Read Mode
   Step  8:  Decision between direct and single value programming
   Step  9:  Unlock Bypass Reset
*******************************************************************************/
ReturnType FlashProgram(udword udMode, udword udAddrOff, udword udNrOfElementsInArray, void *pArray ) {
   ReturnType rRetVal = Flash_Success; /* Return Value: Initially optimistic */
   ReturnType rProtStatus; /* Protection Status of a block */
   uCPUBusType *ucpArrayPointer; /* Use an uCPUBusType to access the array */
   udword udLastOff; /* Holds the last offset to be programmed */
   uBlockType ublFirstBlock; /* The block where start to program */
   uBlockType ublLastBlock; /* The last block to be programmed */
   uBlockType ublCurBlock; /* Current block */

   if (udMode > 1)
      return Flash_FunctionNotSupported;

   /* Step 1: Check if the data to be programmed are within the Flash memory space */
   udLastOff = udAddrOff + udNrOfElementsInArray - 1;
   if( udLastOff >= udDeviceSize )
      return Flash_AddressInvalid;

   /* Step 2: Determine first and last block to program */
   for (ublFirstBlock=0; ublFirstBlock < ublNumBlocks-1;ublFirstBlock++)
      if (udAddrOff < BlockOffset(ublFirstBlock+1))
         break;

   for (ublLastBlock=ublFirstBlock; ublLastBlock < ublNumBlocks-1;ublLastBlock++)
      if (udLastOff < BlockOffset(ublLastBlock+1))
         break;

   /* Step 3: Check protection status for the blocks to be programmed */
   for (ublCurBlock = ublFirstBlock; ublCurBlock <= ublLastBlock; ublCurBlock++){
      if ( (rProtStatus = FlashCheckBlockProtection(ublCurBlock)) != Flash_BlockUnprotected ){
         rRetVal = Flash_BlockProtected;
         if (ublCurBlock == ublFirstBlock){
            eiErrorInfo.udGeneralInfo[0] = udAddrOff;
            return rRetVal;
         } else {
            eiErrorInfo.udGeneralInfo[0] = BlockOffset(ublCurBlock);
            udLastOff = BlockOffset(ublCurBlock)-1;
         } /* EndIf ublCurBlock */
      } /* EndIf rProtStatus */
   } /* Next ublCurBlock */

   /* Step 4: Issue the Unlock Bypass command */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) ); /* 1st cycle */
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) ); /* 2nd cycle */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0020) ); /* 3nd cycle */

   ucpArrayPointer = (uCPUBusType *)pArray;

   /* Step 5: Unlock Bypass Program command */
   while( udAddrOff <= udLastOff ){
      FlashWrite( ANY_ADDR, CMD(0x00A0) ); /* 1st cycle */
      FlashWrite( udAddrOff, *ucpArrayPointer ); /* 2nd Cycle */

      /* Step 6: Wait until Program/Erase Controller has completed */
      if( FlashDataToggle(5) != Flash_Success){
         /* Step 7: Return to Read Mode */
         FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
         rRetVal=Flash_ProgramFailed;
         eiErrorInfo.udGeneralInfo[0] = udAddrOff;
         break; /* exit while cycle */
      } /* EndIf */

      /* Step 8: Decision between direct and single value programming */
      if (udMode == 0) /* Decision between direct and single value programming */
         ucpArrayPointer++;

      udAddrOff++;
   } /* EndWhile */

   /* Step 9: Unlock Bypass Reset */
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x0090) ); /* 1st cycle */
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x0000) ); /* 2st cycle */

   return rRetVal;

} /* EndFunction FlashProgram */






/*******************************************************************************
Function:     ReturnType FlashReadCfi( uword uwCfiFunc, uCPUBusType *ucpCfiValue )
Arguments:    uwCfiFunc is set to the offset of the CFI parameter to be read.
   The CFI value read from offset uwCfiFunc is passed back to the calling
   function by *ucpCfiValue.

Return Value: The function returns the following conditions:
   Flash_Success
   Flash_CfiFailed

Description: This function checks whether the flash CFI is present and operable,
   then reads the CFI value at the specified offset. The CFI value requested is
   then passed back to the calling function.

Pseudo Code:
   Step 1: Send the Read CFI Instruction
   Step 2: Check that the CFI interface is operable
   Step 3: If CFI is operable read the required CFI value
   Step 4: Return the flash to Read Array mode
*******************************************************************************/
ReturnType FlashReadCfi( uword uwCfiFunc, uCPUBusType *ucpCfiValue ) {
   ReturnType rRetVal = Flash_Success; /* Holds the return value */
   udword udCfiAddr; /* Holds CFI address */

   /* Step 1: Send the Read CFI Instruction */
   FlashWrite( ConvAddr(0x55), (uCPUBusType)CMD(0x0098) );

   /* Step 2: Check that the CFI interface is operable */
   if( ((FlashRead( ShAddr(0x00000010) ) & CMD(0x00FF) ) != CMD(0x0051)) ||
       ((FlashRead( ShAddr(0x00000011) ) & CMD(0x00FF) ) != CMD(0x0052)) ||
       ((FlashRead( ShAddr(0x00000012) ) & CMD(0x00FF) ) != CMD(0x0059)) )
      rRetVal = Flash_CfiFailed;
   else {
      /* Step 3: Read the required CFI Info */
      udCfiAddr = (udword)uwCfiFunc;
      *ucpCfiValue = FlashRead( ShAddr((udCfiAddr & 0x000000FF)) );
   } /* EndIf */

   FlashReset(); /* Step 4: Return to Read Array mode */
   return rRetVal;
} /* EndFunction FlashReadCfi */





/*******************************************************************************
Function:     ReturnType FlashReadDeviceId( uCPUBusType *ucpDeviceId )
Arguments:    - *ucpDeviceId = <return value> The function returns the Device Code.
   The device code for the part is:
   M29W640G   0x227E

Note:         In case a common response of more flash chips is not identical the real
   read value will be given (Flash_ResponseUnclear)

Return Value: The function returns the following conditions:
   Flash_Success
   Flash_ResponseUnclear

Description: This function can be used to read the device code of the flash.

Pseudo Code:
   Step 1:  Send the Auto Select instruction
   Step 2:  Read the DeviceId
   Step 3:  Return the device to Read Array mode
   Step 4:  Check flash response (more flashes could give different results)
*******************************************************************************/
ReturnType FlashReadDeviceId( uCPUBusType *ucpDeviceId ) {

   /* Step 1: Send the AutoSelect command */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0090) ); /* 3rd Cycle */

   /* Step 2: Read the DeviceId */
   *ucpDeviceId = FlashRead(ShAddr(0x1)); /* A0 = 1, A1 = 0 */

   /* Step 3: Return to Read Array Mode */
   FlashReset();

   /* Step 4: Check flash response (more flashes could give different results) */
   return FlashResponseIntegrityCheck( ucpDeviceId );

} /* EndFunction FlashReadDeviceId */





/*******************************************************************************
Function: ReturnType FlashReadExtendedBlockVerifyCode( uCPUBusType *ucpVerifyCode )
Arguments:    - *ucpVerifyCode = <return value>
                 The function returns the Extended Memory Block Verify Code.

   The Extended Memory Block Verify Code for the part are:

   M29W128G
   		0x80 (factory locked)
            0x00 (not factory locked)

Note:         In case a common response of more flash chips is not identical the real
   read value will be given (Flash_ResponseUnclear)

Return Value: The function returns the following conditions:
   Flash_Success
   Flash_ResponseUnclear

Description: This function can be used to read the Extended Memory Block Verify Code.
   The verify code is used to specify if the Extended Memory Block was locked/not locked
   by the manufacturer.

Pseudo Code:
   Step 1:  Send the Auto Select instruction
   Step 2:  Read the Extended Memory Block Verify Code
   Step 3:  Return the device to Read Array mode
   Step 4:  Check flash response (more flashes could give different results)
*******************************************************************************/
ReturnType FlashReadExtendedBlockVerifyCode( uCPUBusType *ucpVerifyCode ) {

   /* Step 1: Send the AutoSelect command */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0090) ); /* 3rd Cycle */

   /* Step 2: Read the Extended Memory Block Verify Code */
   *ucpVerifyCode = FlashRead(ShAddr(0x3)); /* A0 = 1, A1 = 1 */

   /* Step 3: Return to Read Array Mode */
   FlashReset();

   /* Step 4: Check flash response (more flashes could give different results) */
   return FlashResponseIntegrityCheck( ucpVerifyCode );

} /* EndFunction FlashReadExtendedBlockVerifyCode */





/*******************************************************************************
Function:     ReturnType FlashReadManufacturerCode( uCPUBusType *ucpManufacturerCode )
Arguments:    - *ucpManufacturerCode = <return value> The function returns
   the manufacture code (for ST = 0x0020).
   In case a common response of more flash chips is not identical the real
   read value will be given (Flash_ResponseUnclear)

Return Value: The function returns the following conditions:
   Flash_Success
   Flash_ResponseUnclear

Description:   This function can be used to read the manufacture code of the flash.

Pseudo Code:
   Step 1:  Send the Auto Select instruction
   Step 2:  Read the Manufacturer Code
   Step 3:  Return the device to Read Array mode
   Step 4:  Check flash response (more flashes could give different results)
*******************************************************************************/
ReturnType FlashReadManufacturerCode( uCPUBusType *ucpManufacturerCode ) {

   /* Step 1: Send the AutoSelect command */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0090) ); /* 3rd Cycle */

   /* Step 2: Read the DeviceId */
   *ucpManufacturerCode = FlashRead( ShAddr(0x0) ); /* A0 = 0, A1 = 0 */

   /* Step 3: Return to Read Array Mode */
   FlashReset();

   /* Step 4: Check flash response (more flashes could give different results) */
   return FlashResponseIntegrityCheck( ucpManufacturerCode );

} /* EndFunction FlashReadManufacturerCode */


/*******************************************************************************
Function:     ReturnType FlashReadMultipleDeviceId(uword uwDeviceIdNr,
                                                   uCPUBusType *ucpDeviceId )
Arguments:
        uwDeviceIdNr, specify which Device ID to return.
   *ucpDeviceId, after the execution the variable contains:
   -> The specified device ID of a single device if all flash chips give
      an identical result
   -> The complete response of all devices in any other situation.

Description:
   This function can be used to read the device code of the flash chips, in the
   case they are identical.

Return Value:
   Flash_Success         = if the Device ID(s) are equal or there is a single flash chip
   Flash_ResponseUnclear = if the Device Ids are different.
      Notes: With A1=1 and A2=0 the device codes for the parts are:
                M29W128F   0x227E

Pseudo Code:
   Step 1:  Select the correct address to read the required device ID
   Step 2:  Send the Auto Select instruction
   Step 3:  Read the DeviceId
   Step 4:  Return to Read Array Mode
   Step 5:  Check flash response (more flashes could give different results)
*******************************************************************************/
ReturnType FlashReadMultipleDeviceId(uword uwDeviceIdNr, uCPUBusType *ucpDeviceId ) {
   uword uwRequiredDeviceIdAddr;

   /* Step 1: Select the correct address to read the required device ID */
   switch (uwDeviceIdNr) {
      case 0:
             uwRequiredDeviceIdAddr=0x0001;
             break;

      case 1:
             uwRequiredDeviceIdAddr=0x000E;
             break;

      case 2:
             uwRequiredDeviceIdAddr=0x000F;
             break;

      default:
             eiErrorInfo.sprRetVal = FlashSpec_InvalidDeviceIdNr;
             return Flash_SpecificError;
   } /* EndSwitch */

   /* Step 2: Send the Auto Select instruction */

   FlashWrite( ConvAddr(0x0555), CMD(0x00AA) );
   /* 1st Cycle */
   FlashWrite( ConvAddr(0x02AA), CMD(0x0055) );
   /* 12nd Cycle */
   FlashWrite( ConvAddr(0x0555), CMD(0x0090) );
   /* 13rd Cycle */

   /* Step 3: Read the DeviceId */
   *ucpDeviceId = FlashRead ( ShAddr(uwRequiredDeviceIdAddr) );

   /* Step 4: Return to Read Array Mode */
   FlashReset();

   /* Step 5: Check flash response (more flashes could give different results) */
   return FlashResponseIntegrityCheck( ucpDeviceId );

} /* EndFunction FlashReadMultipleDeviceId */



/*******************************************************************************
Function:      void FlashReset( void )
Arguments:     none
Return Value:  Flash_Success
Description:   This function places the flash in the Read Array mode described
   in the Data Sheet. In this mode the flash can be read as normal memory.

   All of the other functions leave the flash in the Read Array mode so this is
   not strictly necessary. It is provided for completeness and in case of
   problems.

Pseudo Code:
   Step 1: write command sequence (see Instructions Table of the Data Sheet)
*******************************************************************************/
ReturnType FlashReset( void ) {

   /* Step 1: write command sequence */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* 3rd Cycle: write 0x00F0 to ANY address */
   return Flash_Success;

} /* EndFunction FlashReset */





/*******************************************************************************
Function:     ReturnType FlashResponseIntegrityCheck(uCPUBusType *ucpFlashResponse)
Arguments:    - ucpFlashResponse <parameter> + <return value>
   The function returns a unique value in case one flash or an
   array of flashes return all the same value (Consistent Response = Flash_Success).
   In case an array of flashes returns different values the function returns the
   received response without any changes (Inconsistent Response = Flash_ResponseUnclear).

Return Value: The function returns the following conditions:
   Flash_Success
   Flash_ResponseUnclear

Description:   This function is used to create one response in multi flash
   environments, instead of giving multiple answers of the single flash
   devices.

   For example: Using a 32bit CPU and two 16bit Flash devices, the device Id
   would be directly read: 00170017h, because each device gives an answer
   within the range of the databus. In order to give a simple response
   like 00000017h in all possible configurations, this subroutine is used.

   In case the two devices give different results for the device Id, the
   answer would then be: 00150017h. This allows debugging and helps to
   discover multiple flash configuration problems.

Pseudo Code:
   Step 1:  Extract the first single flash response
   Step 2:  Compare all next possible flash responses with the first one
   Step 3a: Return all flash responses in case of different values
   Step 3b: Return only the first single flash response in case of matching values

*******************************************************************************/
ReturnType FlashResponseIntegrityCheck(uCPUBusType *ucpFlashResponse) {
   ubyte a;
   union {
      uCPUBusType ucFlashResponse;
      ubyte       ubBytes[sizeof(uCPUBusType)];
   } FullResponse;

   union {
      uCPUBusType ucSingleResponse;
      ubyte       ubBytes[FLASH_BIT_DEPTH/8];
   } SingleResponse;

   SingleResponse.ucSingleResponse = 0;
   FullResponse.ucFlashResponse    = *ucpFlashResponse;

   /* Step 1: Extract the first single flash response */
   memcpy(SingleResponse.ubBytes, FullResponse.ubBytes, FLASH_BIT_DEPTH/8);

   /* Step 2: Compare all next possible flash responses with the first one */
   for (a = 0; a < sizeof(uCPUBusType); a += FLASH_BIT_DEPTH/8) {
      if (memcmp (&FullResponse.ubBytes[a], SingleResponse.ubBytes, FLASH_BIT_DEPTH/8) != 0)
         /* Step 3a: Return all flash responses in case of different values */
         return Flash_ResponseUnclear;
   } /* Next a */

   /* Step 3b: Return only the first single flash response in case of matching values */
   *ucpFlashResponse = SingleResponse.ucSingleResponse;
   return Flash_Success;
} /* EndFunction FlashResponseIntegrityCheck */





/*******************************************************************************
Function:      ReturnType FlashResume( void )
Arguments:     none
Return Value:  The function returns the following conditions:
   Flash_Success

Description:   This function resume a suspended operation.

Pseudo Code:
   Step 1:     Send the Erase resume command to the device
*******************************************************************************/
ReturnType FlashResume( void ) {

   /* Step 1: Send the Erase Resume command */
   FlashWrite( ANY_ADDR,CMD(0x0030) );
   return Flash_Success;

} /* EndFunction FlashResume */





/*******************************************************************************
Function:     ReturnType FlashSingleProgram( udword udAddrOff, uCPUBusType ucVal )
Arguments:    udAddrOff is the offset in the flash to write to.
              ucVal is the value to be written
Return Value: The function returns the following conditions:
   Flash_Success
   Flash_AddressInvalid
   Flash_BlockProtected
   Flash_ProgramFailed

Description: This function is used to write a single element to the flash.

Pseudo Code:
   Step 1: Check the offset range is valid
   Step 2: Check if the start block is protected
   Step 3: Program sequence command
   Step 4: Follow Data Toggle Flow Chart until Program/Erase Controller has
           completed
   Step 5: Return to Read Mode (if an error occurred)
*******************************************************************************/
ReturnType FlashSingleProgram( udword udAddrOff, uCPUBusType ucVal) {
   uBlockType ublCurBlock;

   /* Step 1: Check the offset and range are valid */
   if( udAddrOff >= udDeviceSize )
      return Flash_AddressInvalid;

   /* compute the start block */
   for (ublCurBlock=0; ublCurBlock < ublNumBlocks-1;ublCurBlock++)
      if (udAddrOff < BlockOffset(ublCurBlock+1))
         break;

   /* Step 2: Check if the start block is protected */
   if (FlashCheckBlockProtection(ublCurBlock)== Flash_BlockProtected) {
      return Flash_BlockProtected;
   } /* EndIf */

   /*Step 3: Program sequence command */

   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) ); /* 1st cycle */
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) ); /* 2nd cycle */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00A0) ); /* Program command */
   FlashWrite( udAddrOff,ucVal ); /* Program val */

   /* Step 4: Follow Data Toggle Flow Chart until Program/Erase Controller
              has completed */

   /* See Data Toggle Flow Chart of the Data Sheet */
   if( FlashDataToggle(5) != Flash_Success) {

      /* Step 5: Return to Read Mode (if an error occurred) */
      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
      return Flash_ProgramFailed ;
   } /* EndIf */
   return Flash_Success;

} /* EndFunction FlashSingleProgram */





/*******************************************************************************
Function:     ReturnType FlashSuspend( void )
Arguments:    none
Return Value: The function returns the following conditions:
   Flash_Success

Description:  This function suspends an operation.

Pseudo Code:
   Step 1:  Send the Erase suspend command to the device
*******************************************************************************/
ReturnType FlashSuspend( void ) {

   /* Step 1: Send the Erase Suspend command */
   FlashWrite( ANY_ADDR,CMD(0x00B0) );
   return Flash_Success;

} /* EndFunction FlashSuspend */




/*******************************************************************************
Function:     uCPUBusType FlashRead( udword udAddrOff )
Arguments:    udAddrOff is the offset into the flash to read from.
Return Value: The uCPUBusType content at the address offset.
Description: This function is used to read a uCPUBusType from the flash.
   On many microprocessor systems a macro can be used instead, increasing the
   speed of the flash routines. For example:

   #define FlashRead( udAddrOff ) ( BASE_ADDR[udAddrOff] )

   A function is used here instead to allow the user to expand it if necessary.

Pseudo Code:
   Step 1: Return the value at double-word offset udAddrOff
*******************************************************************************/
uCPUBusType FlashRead( udword udAddrOff )
{
#ifdef PLATFORM_ORION
	volatile uCPUBusType *tempbase  = BASE_ADDR;
	uCPUBusType ret;
 	if (udDieSize)
		udAddrOff %= udDieSize;
	if (((udword)BASE_ADDR & (udDieSize<<1)) &&
		(udDeviceSize == 0x8000000) )
	{
		UPDIE_ACTIVATE; // ACTIVATE 2Gb functionality on Orion
		UPDIE_ENABLE; // ENABLE UP DIE
		(udword)BASE_ADDR &= ~(udDieSize<<1);
	}
    ret = BASE_ADDR[udAddrOff];
	BASE_ADDR = tempbase;
	UPDIE_DISABLE;

	return ret;

#else

	return BASE_ADDR[udAddrOff];
#endif

} /* EndFunction FlashRead */



/*******************************************************************************
Function:     void FlashWrite( udword udAddrOff, uCPUBusType ucVal )
Arguments:    udAddrOff is double-word offset in the flash to write to.
   ucVal is the value to be written
Return Value: None
Description:  This function is used to write a uCPUBusType to the flash.
*******************************************************************************/
void FlashWrite( udword udAddrOff, uCPUBusType ucVal )
{
   /* Write ucVal to the word offset in flash */
#ifdef PLATFORM_ORION
		volatile uCPUBusType *tempbase  = BASE_ADDR;

		/* Step 1 Return the value at word offset udAddrOff */
		if (udDieSize)
			udAddrOff %= udDieSize;
	if (((udword)BASE_ADDR & (udDieSize<<1)) &&
		(udDeviceSize == 0x8000000) )
		{
			UPDIE_ACTIVATE; // ACTIVATE 2Gb functionality on Orion
			UPDIE_ENABLE; // ENABLE UP DIE
			(udword)BASE_ADDR &= ~(udDieSize<<1);
		}
		BASE_ADDR[udAddrOff] = ucVal;
		BASE_ADDR = tempbase;
		UPDIE_DISABLE;
#else
	BASE_ADDR[udAddrOff] = ucVal;

#endif
} /* EndFunction FlashWrite */


/*******************************************************************************
Function:     ReturnType FlashWriteToBufferProgram( udword udMode,udword udAddrOff,
                                       udword udNrOfElementsInArray, void *pArray )
Arguments:  udMode changes between programming modes
   udNrOfElementsInArray holds the number of elements (uCPUBusType) in the array.
      it must be between 1 to 16(16bit Mode) or 32(8bit Mode)
   pArray is a void pointer to the array with the contents to be programmed.
Return Value: The function returns the following conditions:
   Flash_Success                 successful operation
   Flash_AddressInvalid          program range outside device
   Flash_BlockProtected          block to program is protected
   Flash_ProgramFailed           any other failure

Description: This function is used to program an array into the flash. It does
   not erase the flash first and will not produce proper results, if the block(s)
   are not erased first.
   Any errors are returned without any further attempts to program other addresses
   of the device. The function returns Flash_Success when all addresses have
   successfully been programmed.

   Note: Two program modes are available:
   - udMode = 0, Normal Program Mode
   The number of elements (udNumberOfElementsInArray) contained in pArray
   are programmed directly to the flash starting with udAddrOff.
   - udMode = 1, Single Value Program Mode
   Only the first value of the pArray will be programmed to the flash
   starting from udAddrOff.
   Note:
   - Please refer to the M29EW datasheet for detailed list of buffer boundary
    condition

Pseudo Code:
   Step 1:  Check whether the data to be programmed are within the
             Flash memory
   Step 2: Determine first and last block to program
   Step 3: Check protection status for the blocks to be programmed

   Step 4: Issue Write to Program Buffer command and Write Content to buffer

*******************************************************************************/
ReturnType FlashWriteToBufferProgram(udword udMode,udword udAddrOff, udword udNrOfElementsInArray, void *pArray ) {

   ReturnType  rRetVal = Flash_Success; /* Return Value: Initially optimistic */
   ReturnType  rProtStatus; /* Protection Status of a block */
   uCPUBusType *ucpArrayPointer; /* Use an uCPUBusType to access the array */
   udword      udLastOff; /* Holds the last offset to be programmed */
   uBlockType  ublFirstBlock; /* The block where start to program */
   uBlockType  ublLastBlock; /* The last block to be programmed */
   uBlockType  ublCurBlock; /* Current block */

   if (udMode > 1)
      return Flash_FunctionNotSupported;
   if (udNrOfElementsInArray<1)
      return Flash_AddressInvalid;

   /* Step 1: Check if the data to be programmed are within the Flash memory space   */
   udLastOff = udAddrOff + udNrOfElementsInArray - 1;
   if( udLastOff >= udDeviceSize )
      return Flash_AddressInvalid;

   if (udNrOfElementsInArray  >  (udBufferSize-(udAddrOff&(udBufferSize-1))) )
       return Flash_AddressInvalid;

   /* Step 2: Determine first and last block to program */
   for (ublFirstBlock=0; ublFirstBlock < ublNumBlocks-1;ublFirstBlock++)
      if (udAddrOff < BlockOffset(ublFirstBlock+1))
         break;
   for (ublLastBlock=ublFirstBlock; ublLastBlock < ublNumBlocks-1;ublLastBlock++)
      if (udLastOff < BlockOffset(ublLastBlock+1))
         break;

   /* Step 3: Check protection status for the blocks to be programmed */
   for (ublCurBlock = ublFirstBlock; ublCurBlock <= ublLastBlock; ublCurBlock++) {
      if ( (rProtStatus = FlashCheckBlockProtection(ublCurBlock)) != Flash_BlockUnprotected ) {
         rRetVal = Flash_BlockProtected;
         if (ublCurBlock == ublFirstBlock) {
            eiErrorInfo.udGeneralInfo[0] = udAddrOff;
            return rRetVal;
         } else {
            eiErrorInfo.udGeneralInfo[0] = BlockOffset(ublCurBlock);
            udLastOff = BlockOffset(ublCurBlock)-1;
         } /* EndIf ublCurBlock == ublFirstBlock */
      } /* EndIf rProtStatus */
   } /* Next ublCurBlock */

   ucpArrayPointer = (uCPUBusType *)pArray;

   /* Step 4a: Issue Write to Program Buffer command */
       FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) ); /* 1st cycle */
       FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) ); /* 2nd cycle */
       FlashWrite( udAddrOff, (uCPUBusType)CMD(0x0025) ); /* 3nd cycle */
       FlashWrite( udAddrOff, udNrOfElementsInArray-1); /* 4th cycle */
   /* Step 4b: Write Content to buffer */
   while( udAddrOff <= udLastOff ) {
       FlashWrite( udAddrOff, (uCPUBusType)CMD(*ucpArrayPointer) ); /* 5th cycle */

   	   if (udMode == 0) /* Decision between direct and single value programming */
         ucpArrayPointer++;
       udAddrOff++;
   } /* EndWhile */

   return rRetVal;
} /* EndFunction FlashWriteToBufferProgram */




/*******************************************************************************
Function:     ReturnType FlashBufferProgramConfirm( udword udAddrOff )
Arguments:  udAddrOff is the address offset exactly same as the one issued
			by the Write to Buffer Command
Return Value: The function returns the following conditions:
               Flash_Success
               Flash_ProgramFailed
Note:
Description: This function confirms the Write program Buffer Command(done by function FlashWriteToBufferProgram)
		and start the buffer program

Pseudo Code:
   Step 1: Issue Buffer Program Confirm command
   Step 2: Wait until the Program/Erase Controller has completed
********************************************************************************/
ReturnType FlashBufferProgramConfirm( udword udAddrOff ) {

   ReturnType  rRetVal = Flash_Success; /* Return Value: Initially optimistic */

   /* Step 1:  Issue Buffer Program Confirm command */
   FlashWrite( udAddrOff, (uCPUBusType)CMD(0x0029) ); /* 1st cycle */

   /* Step 2: Wait until Program/Erase Controller has completed */
   if( FlashDataToggle(5) != Flash_Success) {
   /* Step 3: Return to Read Mode */
         FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
         rRetVal=Flash_ProgramFailed;
         eiErrorInfo.udGeneralInfo[0] = udAddrOff;
         return rRetVal; /* exit while cycle */
      } /* EndIf */

    return rRetVal;

}/* EndFunction FlashBufferProgramConfirm */




/*******************************************************************************
Function:     ReturnType FlashBufferProgramAbort( void )
Arguments:  none
Return Value: The function returns the following conditions:
               Flash_Success
Note:
Description: This function Abort the Write program Buffer Command(done by function FlashWriteToBufferProgram)
		and Reset to read array mode

Pseudo Code:
   Step 1: Issue Buffer Program Abort command
********************************************************************************/
ReturnType FlashBufferProgramAbort( void ){

	 /* Step 1: Issue Buffer Program Abort command */
       FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) ); /* 1st cycle */
       FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) ); /* 2nd cycle */
       FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00F0) ); /* 3rd cycle */
	 return Flash_Success;
} /* EndFunction  FlashBufferProgramAbort */




/*******************************************************************************
Function:     ReturnType FlashBufferProgram( udword udMode,udword udAddrOff, udword udNrOfElementsInArray, void *pArray  )
Arguments:  udMode changes between programming modes
            udAddrOff must be 16Word/32Byte aligned, is the address offset into the flash to be programmed
            udNrOfElementsInArray holds the number of elements (uCPUBusType) in the array.
            pArray is a void pointer to the array with the contents to be programmed.
Return Value: The function returns the following conditions:
               Flash_Success
               Flash_ProgramFailed
Note:
Description: This function use buffer program method to speed up the program process
		it will call FlashWriteToBufferProgram and FlashBufferProgramConfirm

Pseudo Code:
   Step 1: Check whether the data to be programmed are within the Flash memory
   Step 2: Determine first and last block to program
   Step 3: Check protection status for the blocks to be programmed
   Step 4: Call FlashWriteToBufferProgram
   Step 5: Call FlashBufferProgramConfirm to start buffer program
   Step 6: Judge conditions
   		 if Step 5 return Flash_ProgramFailed,return;
             if Step 5 return Flash_Success and there is data remains, repeat step 4 and step 5
             if Step 5 return Flash_Success and no data remains, go to next step
   Step 7: Return to Read Mode
********************************************************************************/
ReturnType FlashBufferProgram( udword udMode,udword udAddrOff, udword udNrOfElementsInArray, void *pArray ) {

   ReturnType  rRetVal = Flash_Success; /* Return Value: Initially optimistic */
   ReturnType  rProtStatus; /* Protection Status of a block */
   uCPUBusType *ucpArrayPointer; /* Use an uCPUBusType to access the array */
   udword      udLastOff; /* Holds the last offset to be programmed */
   uBlockType  ublFirstBlock; /* The block where start to program */
   uBlockType  ublLastBlock; /* The last block to be programmed */
   uBlockType  ublCurBlock; /* Current block */
   udword remains; /* remain numbers to be written in the programming array */
   //udword curLength; /* current length need to write to the buffer */

   if (udMode > 1)
      return Flash_FunctionNotSupported;

   /* Step 1: Check if the data to be programmed are within the Flash memory Space   */
   udLastOff = udAddrOff + udNrOfElementsInArray - 1;
   if( udLastOff >= udDeviceSize )
      return Flash_AddressInvalid;

   /* Step 2: Determine first and last block to program */
   for (ublFirstBlock=0; ublFirstBlock < ublNumBlocks-1;ublFirstBlock++)
      if (udAddrOff < BlockOffset(ublFirstBlock+1))
         break;
   for (ublLastBlock=ublFirstBlock; ublLastBlock < ublNumBlocks-1;ublLastBlock++)
      if (udLastOff < BlockOffset(ublLastBlock+1))
         break;

   /* Step 3: Check protection status for the blocks to be programmed */
   for (ublCurBlock = ublFirstBlock; ublCurBlock <= ublLastBlock; ublCurBlock++) {
      if ( (rProtStatus = FlashCheckBlockProtection(ublCurBlock)) != Flash_BlockUnprotected ) {
         rRetVal = Flash_BlockProtected;
         if (ublCurBlock == ublFirstBlock) {
            eiErrorInfo.udGeneralInfo[0] = udAddrOff;
            return rRetVal;
         } else {
            eiErrorInfo.udGeneralInfo[0] = BlockOffset(ublCurBlock);
            udLastOff = BlockOffset(ublCurBlock)-1;
         } /* EndIf ublCurBlock == ublFirstBlock */
      } /* EndIf rProtStatus */
   } /* Next ublCurBlock */

   ucpArrayPointer = (uCPUBusType *)pArray;
   remains = udNrOfElementsInArray;

   do {
   	udword curLength = udBufferSize-((udAddrOff&udBufferSize-1));
		if (curLength > remains)
			curLength = remains;

   	/* Step 4: Call FlashWriteToBufferProgram */
   	rRetVal = FlashWriteToBufferProgram(udMode,udAddrOff,curLength,ucpArrayPointer);
   	if( rRetVal != Flash_Success ){
   	 	FlashBufferProgramAbort(); /* there is some error,so abort the operation */
   	 	return rRetVal;
   	}

   	/* Step 5: Call FlashBufferProgramConfirm to start buffer program */
   	rRetVal = FlashBufferProgramConfirm(udAddrOff);

   	/* Step 6: Judge conditions */
   	if( rRetVal != Flash_Success ){
   	 	FlashBufferProgramAbort(); /* there is some error,so abort the operation */
   	 	return rRetVal;
   	}

   	udAddrOff += curLength;
   	if (udMode == 0) /* Decision between direct and single value programming */
   		ucpArrayPointer += curLength;

   	remains -= curLength;

   }while (remains); /* End while */

   /* Step 7: Return to Read Mode */
   FlashWrite( ANY_ADDR, CMD(0x00F0) );

   return rRetVal;
}/* EndFunction FlashBufferProgram */


/*******************************************************************************
Function:     ReturnType FlashUnlockBypass( void )
Arguments:    None
Return Value: The function returns the following conditions:
              Flash_Success, successful operation
Description:  This function is used in conjunction with the FlashUnlockBypassProgram
              function to program faster the memory. After the FlashUnlockBypass command,
              the flash enters into Unlock Bypass mode. To return the flash in the Read
              Array mode, the FlashUnlockBypassReset() function can be used.

  Pseudo Code:
   Step 1: Issue the unlock Bypass Command
*******************************************************************************/
ReturnType FlashUnlockBypass( void ) {

   /* Step 1: Issue the Unlock Bypass Command */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) ); /* 1st cycle */
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) ); /* 2nd cycle */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0020) ); /* 3nd cycle */
   return Flash_Success;
} /* EndFunction FlashUnlockBypass */


/*******************************************************************************
Function:     ReturnType  FlashUnlockBypassProgram (udword udAddrOff, udword NumWords,
                          void *pArray)

Arguments:    udAddrOff is the word offset into the flash to be programmed.
   NumWords holds the number of words in the array.
   pArray is a pointer to the array to be programmed.

Return Value: The function returns the following conditions:
   Flash_Success
   Flash_ProgramFailed
   Flash_AddressInvalid

   When all addresses are successfully programmed the function returns Flash_Success.
   The function returns Flash_ProgramFail if a programming failure occurs:
   udFirstAddrOffProgramFailed will be filled with the first address on which
   the program operation has failed and the functions does not continue to program
   on the remaining addresses.
   If part of the address range to be programmed falls within a protected block,
   the function returns  nothing is programmed and the function no error return.
   If the address range to be programmed exceeds the address range of the Flash
   Device the function returns Flash_AddressInvalid and nothing is programmed.

Description:  This function is used to program an array into the flash. It does
   not erase the flash first and may fail if the block(s) are not erased first.
   This function can be used only when the device is in unlock bypass mode. The memory
   offers accellerated program operations through the Vpp pin. When the system asserts
   Vpp on Vpp pin the memory enters the unlock bypass mode.

Pseudo Code:
   Step 1: Check the offset range is valid
   Step 2: Check that the block(s) to be programmed are not protected
   Step 3: Send the Unlock Bypass command
   Step 4: While there is more to be programmed
   Step 5: Program the next word
   Step 6: Follow Data Toggle Flow Chart until Program/Erase Controller has
           completed
   Step 7: Return to Read Mode (if an error occurred)
   Step 8: Send the Unlock Bypass Reset command
*******************************************************************************/
ReturnType FlashUnlockBypassProgram (udword udAddrOff, udword NumWords, void *pArray){
   udword  udLastOff;
   uCPUBusType *ucpArrayPointer;
   ReturnType rRetVal = Flash_Success; /* Return Value: Initially optimistic */

   udLastOff = udAddrOff + NumWords- 1;

   /* Step 1: Check that the offset and range are valid */
   if( udLastOff >= udDeviceSize )
      return Flash_AddressInvalid;


   /* Step 4: While there is more to be programmed */
   ucpArrayPointer = (uCPUBusType *)pArray;
   while( udAddrOff <= udLastOff ){
      /* Step 5: Unlock Bypass Program the next word */
      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00A0) ); /* 1st cycle */
      FlashWrite( udAddrOff++,*ucpArrayPointer++ ); /* Program value */

      /* Step 6: Follow Data Toggle Flow Chart until Program/Erase Controller
                 has completed */

      /* See Data Toggle Flow Chart of the Data Sheet */

      if( FlashDataToggle(5) != Flash_Success){

         /* Step 7: Return to Read Mode (if an error occurred) */
         FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
         rRetVal=Flash_ProgramFailed ;
	  eiErrorInfo.udGeneralInfo[0]=udAddrOff-1;
         break;
      } /* EndIf */

   } /* EndWhile */

    return rRetVal;

} /* EndFunction FlashUnlockBypassProgram */




/*******************************************************************************
Function:     void FlashUnlockBypassReset (void);
Arguments:    None
Return Value: None
Description:  This function is used to send the Unlock Bypass Reset command to the device
Pseudo Code:
   Step 1:  Send the Unlock Bypass Reset command to the device
*******************************************************************************/
void FlashUnlockBypassReset( void ) {

   /* Step 1: Send the Unlock Bypass Reset command */
   FlashWrite( ANY_ADDR, CMD(0x0090) ); /* 1st Cycle */
   FlashWrite( ANY_ADDR, CMD(0x0000) ); /* 2nd Cycle */

} /* EndFunction FlashUnlockBypassReset */

/*******************************************************************************
Function:     ReturnType FlashUnlockBypassBlockErase( uBlockType ublBlockNr )
Arguments:    ublBlockNr is the number of the Block to be erased.
Return Value: The function returns the following conditions:
   Flash_Success
   Flash_BlockEraseFailed
   Flash_BlockNrInvalid
   Flash_BlockProtected
   Flash_OperationTimeOut

Description:  This function can be used to erase the Block specified in ublBlockNr.
   The function checks that the block nr is within the valid range and not protected
   before issuing the erase command, otherwise the block will not be erased and an
   error code will be returned.
   The function returns only when the block is erased. During the Erase Cycle the
   Data Toggle Flow Chart of the Datasheet is followed. The polling bit, DQ7, is not
   used.

Pseudo Code:
   Step 1:  Check that the block number exists
   Step 2:  Write Block Erase command
   Step 3:  Wait for the timer bit to be set
   Step 4:  Follow Data Toggle Flow Chart until the Program/Erase Controller is
            finished
   Step 5:  Return to Read mode (if an error occurred)
*******************************************************************************/
ReturnType FlashUnlockBypassBlockErase( uBlockType ublBlockNr) {

   ReturnType rRetVal = Flash_Success; /* Holds return value: optimistic initially! */

   /* Step 1: Check for invalid block. */
   if( ublBlockNr >= ublNumBlocks ) /* Check specified blocks <= ublNumBlocks */
      return Flash_BlockNrInvalid;

   /* Step 2: Write Block Erase command */
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x0080) );
   FlashWrite( BlockOffset(ublBlockNr), (uCPUBusType)CMD(0x0030) );

   /* Step 3: Wait for the Erase Timer Bit (DQ3) to be set */
   FlashTimeOut(0); /* Initialize TimeOut Counter */
   while( !(FlashRead( BlockOffset(ublBlockNr) ))& CMD(0x0008) )
   {
      if (FlashTimeOut(5) == Flash_OperationTimeOut) {
         FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
         return Flash_OperationTimeOut;
      } /* EndIf */
   } /* EndWhile */

   /* Step 4: Follow Data Toggle Flow Chart until Program/Erase Controller completes */
   if( FlashDataToggle(10) !=  Flash_Success ) {
      /* Step 5: Return to Read mode (if an error occurred) */
      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
      rRetVal=Flash_BlockEraseFailed;
   } /* EndIf */
   return rRetVal;
} /* EndFunction FlashBlockErase */



/*******************************************************************************
Function:ReturnType FlashUnlockBypassMultipleBlockErase(uBlockType ublNumBlocks,uBlockType
                    *ublpBlock,ReturnType *rpResults)
Arguments:   ublNumBlocks holds the number of blocks in the array ubBlock
   ublpBlocks is an array containing the blocks to be erased.
   rpResults is an array that it holds the results of every single block
   erase.
   Every elements of rpResults will be filled with below values:
      Flash_Success
      Flash_BlockEraseFailed
      Flash_BlockProtected
   If a time-out occurs because the MPU is too slow then the function returns
   Flash_MpuTooSlow

Return Value: The function returns the following conditions:
   Flash_Success
   Flash_BlockEraseFailed
   Flash_BlockNrInvalid
   Flash_OperationTimeOut
   Flash_SpecificError      : if a no standard error occour.In this case the
      field sprRetVal of the global variable eiErrorInfo will be filled
      with Flash_MpuTooSlow when any blocks are not erased because DQ3
      the MPU is too slow.


Description: This function erases up to ublNumBlocks in the flash. The blocks
   can be listed in any order. The function does not return until the blocks are
   erased. If any blocks are protected or invalid none of the blocks are erased,
   in this casse the function return Flash_BlockEraseFailed.
   During the Erase Cycle the Data Toggle Flow Chart of the Data Sheet is
   followed. The polling bit, DQ7, is not used.

Pseudo Code:
   Step 1:  Check for invalid block
   Step 2:  Write Block Erase command
   Step 3:  Check for time-out blocks
   Step 4:  Wait for the timer bit to be set.
   Step 5:  Follow Data Toggle Flow Chart until Program/Erase Controller has
            completed
   Step 6:  Return to Read mode (if an error occurred)

*******************************************************************************/
ReturnType FlashUnlockBypassMultipleBlockErase(uBlockType ublNumBlocks,uBlockType *ublpBlock,ReturnType *rpResults) {

   ReturnType rRetVal = Flash_Success; /* Holds return value: optimistic initially! */
   uBlockType ublCurBlock; /* Range Variable to track current block */
   uCPUBusType ucFirstRead, ucSecondRead; /* used to check toggle bit DQ2 */

   /* Step 1: Check for invalid block. */
   if( ublNumBlocks > ublNumBlocks ){ /* Check specified blocks <= ublNumBlocks */
      eiErrorInfo.sprRetVal = FlashSpec_TooManyBlocks;
      return Flash_SpecificError;
   } /* EndIf */

   /* Step 2: Write Block Erase command */
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x0080) );

   /* DSI!: Disable Interrupt, Time critical section. Additional blocks must be added
            every 50us */

   for( ublCurBlock = 0; ublCurBlock < ublNumBlocks; ublCurBlock++ ) {
      FlashWrite( BlockOffset(ublpBlock[ublCurBlock]), (uCPUBusType)CMD(0x0030) );
      /* Check for Erase Timeout Period (is bit DQ3 set?)*/
      if( (FlashRead( BlockOffset(ublpBlock[0]) ) & CMD(0x0008)) != 0 )
         break; /* Cannot set any more blocks due to timeout */
   } /* Next ublCurBlock */

   /* ENI!: Enable Interrupt */

   /* Step 3: Check for time-out blocks */
   /* if timeout occurred then check if current block is erasing or not */
   /* Use DQ2 of status register, toggle implies block is erasing */
   if ( ublCurBlock < ublNumBlocks ) {
      ucFirstRead = FlashRead( BlockOffset(ublpBlock[ublCurBlock]) ) & CMD(0x0004);
      ucSecondRead = FlashRead( BlockOffset(ublpBlock[ublCurBlock]) ) & CMD(0x0004);
      if( ucFirstRead != ucSecondRead )
         ublCurBlock++; /* Point to the next block */

      if( ublCurBlock < ublNumBlocks ){
         /* Indicate that some blocks have been timed out of the erase list */
         rRetVal = Flash_SpecificError;
         eiErrorInfo.sprRetVal = FlashSpec_MpuTooSlow;
      } /* EndIf */

      /* Now specify all other blocks as not being erased */
      while( ublCurBlock < ublNumBlocks ) {
         rpResults[ublCurBlock++] = Flash_BlockEraseFailed;
      } /* EndWhile */
   } /* EndIf ( ublCurBlock < ublNumBlocks ) */


   /* Step 4: Wait for the Erase Timer Bit (DQ3) to be set */
   FlashTimeOut(0); /* Initialize TimeOut Counter */
   while( !(FlashRead( BlockOffset(ublpBlock[0]) ) & CMD(0x0008) ) ){
      if (FlashTimeOut(5) == Flash_OperationTimeOut) {
         FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction
                                                              cycle method */
         return Flash_OperationTimeOut;
      } /* EndIf */
   } /* EndWhile */

   /* Step 5: Follow Data Toggle Flow Chart until Program/Erase Controlle completes */
   if( FlashDataToggle(ublNumBlocks*10) !=  Flash_Success ) {
      if (rpResults != NULL) {
         for (ublCurBlock=0;ublCurBlock < ublNumBlocks;ublCurBlock++)
            if (rpResults[ublCurBlock]==Flash_Success)
               rpResults[ublCurBlock] = FlashCheckBlockEraseError(ublCurBlock);
      } /* EndIf */

      /* Step 6: Return to Read mode (if an error occurred) */
      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
      rRetVal=Flash_BlockEraseFailed;
   } /* EndIf */
   return rRetVal;

} /* EndFunction FlashMultipleBlockErase */

/*******************************************************************************
Function:     ReturnType FlashUnlockBypassChipErase( ReturnType *rpResults )
Arguments:    rpResults is a pointer to an array where the results will be
   stored. If rpResults == NULL then no results have been stored.
Return Value: The function returns the following conditions:
   Flash_Success
   Flash_ChipEraseFailed

Description: The function can be used to erase the whole flash chip. Each Block
   is erased in turn. The function only returns when all of the Blocks have
   been erased. If rpResults is not NULL, it will be filled with the error
   conditions for each block.

Pseudo Code:
   Step 1: Send Chip Erase Command
   Step 2: Check for blocks erased correctly
   Step 3: Return to Read mode (if an error occurred)
*******************************************************************************/
ReturnType FlashUnlockBypassChipErase(void) {

   ReturnType rRetVal = Flash_Success; /* Holds return value: optimistic initially! */

   /* Step 1: Send Chip Erase Command */
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x0080));
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x0010) );

   /* Step 2: Check for blocks erased correctly */
   if( FlashDataToggle(120)!=Flash_Success){
      rRetVal= Flash_ChipEraseFailed;
    /* Step 3: Return to Read mode (if an error occurred) */
       FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
   } /* EndIf */
   return rRetVal;

} /* EndFunction FlashChipErase */

/*******************************************************************************
Function:     ReturnType FlashUnlockBypassBufferProgram( udword udMode,udword udAddrOff, udword udNrOfElementsInArray, void *pArray  )
Arguments:  udMode changes between programming modes
            udAddrOff must be 32Word/64Byte aligned, is the address offset into the flash to be programmed
            udNrOfElementsInArray holds the number of elements (uCPUBusType) in the array.
            pArray is a void pointer to the array with the contents to be programmed.
Return Value: The function returns the following conditions:
               Flash_Success
               Flash_ProgramFailed
Note:
Description: This function use buffer program method to speed up the program process
		it will call FlashWriteToBufferProgram and FlashBufferProgramConfirm

Pseudo Code:
   Step 1: Check whether the data to be programmed are within the Flash memory
   Step 2: Determine first and last block to program
   Step 3: Program Buffer size: 32Words/64Bytes
   Step 4: Issue Write to Program Buffer command
   Step 5: Call FlashBufferProgramConfirm to start buffer program
   Step 6: Judge conditions
********************************************************************************/
ReturnType FlashUnlockBypassBufferProgram( udword udMode,udword udAddrOff, udword udNrOfElementsInArray, void *pArray ) {

   ReturnType  rRetVal = Flash_Success; /* Return Value: Initially optimistic */
   ReturnType  rProtStatus; /* Protection Status of a block */
   uCPUBusType *ucpArrayPointer; /* Use an uCPUBusType to access the array */
   udword      udLastOff; /* Holds the last offset to be programmed */
   uBlockType  ublFirstBlock; /* The block where start to program */
   uBlockType  ublLastBlock; /* The last block to be programmed */
   uBlockType  ublCurBlock; /* Current block */
   udword remains; /* remain numbers to be written in the programming array */
   udword curLength; /* current length need to write to the buffer */
   udword udFirstOffInBuffer,udLastOffInBuffer;
   int len = 0;

   if (udMode > 1)
      return Flash_FunctionNotSupported;

   /* Step 1: Check if the data to be programmed are within the Flash memory Space   */
   udLastOff = udAddrOff + udNrOfElementsInArray - 1;
   if( udLastOff >= udDeviceSize )
      return Flash_AddressInvalid;

   /* Step 2: Determine first and last block to program */
   for (ublFirstBlock=0; ublFirstBlock < ublNumBlocks-1;ublFirstBlock++)
      if (udAddrOff < BlockOffset(ublFirstBlock+1))
         break;
   for (ublLastBlock=ublFirstBlock; ublLastBlock < ublNumBlocks-1;ublLastBlock++)
      if (udLastOff < BlockOffset(ublLastBlock+1))
         break;

   ucpArrayPointer = (uCPUBusType *)pArray;
   remains = udNrOfElementsInArray;

   /* Step 3: Program Buffer size: 512Words */

   do {
   	udword curLength = udBufferSize-((udAddrOff&udBufferSize-1));
	if (curLength > remains)
		curLength = remains;

   udFirstOffInBuffer = udAddrOff;
   udLastOffInBuffer = udAddrOff + curLength - 1;


   	/* Step 4: Call FlashWriteToBufferProgram */

       FlashWrite( udAddrOff, (uCPUBusType)CMD(0x0025) ); /* 1st cycle */
       FlashWrite( udAddrOff, curLength-1); /* 2nd cycle */

   /* Step 4b: Write Content to buffer */
   for( len=0; len < curLength; len++) {
       FlashWrite( udAddrOff, (uCPUBusType)CMD(*ucpArrayPointer) ); /* 3rd cycle */

   if (udMode == 0) /* Decision between direct and single value programming */
         ucpArrayPointer++;
      udAddrOff++;
   } /* EndWhile */

   /* Step 5: Call FlashBufferProgramConfirm to start buffer program */
   rRetVal = FlashBufferProgramConfirm(udFirstOffInBuffer);

   	remains -= curLength;

   }while (remains); /* End while */

   return rRetVal;
}/* EndFunction FlashBufferProgram */







/*******************************************************************************
Function:      ReturnType FlashCheckBlockProtection( uBlockType ublBlockNr )
Arguments:     ublBlockNr = block number to be checked
Note: the first block is Block 0

Return Values: The function returns the following conditions:
   Flash_BlockNrInvalid
   Flash_BlockUnprotected
   Flash_BlockProtected
   Flash_BlockProtectionUnclear

Description:   This function reads the protection status of a block.
Pseudo Code:
   Step 1:  Check that the block number exists
   Step 2:  Send the AutoSelect command
   Step 3:  Read Protection Status
   Step 4:  Return the device to Read Array mode
*******************************************************************************/
ReturnType FlashCheckBlockProtection( uBlockType ublBlockNr ) {
   ReturnType  rRetVal; /* Holds the return value */
   uCPUBusType ucProtStatus; /* Holds the protection status */

   /* Step 1: Check that the block number exists */
   if ( ublBlockNr >= ublNumBlocks )
      return Flash_BlockNrInvalid;

   /* Step 2: Send the AutoSelect command */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), (uCPUBusType)CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), (uCPUBusType)CMD(0x0090) ); /* 3rd Cycle */

   /* Step 3: Read Protection Status */
   ucProtStatus=FlashRead( BlockOffset(ublBlockNr) + ShAddr(0x02));
   if ( (ucProtStatus & CMD(0x00ff)) == 0)
      rRetVal = Flash_BlockUnprotected;
   else if ( (ucProtStatus & CMD(0x00ff)) == CMD(0x0001) )
      rRetVal = Flash_BlockProtected;
      else
         rRetVal = Flash_BlockProtectionUnclear;

   /* Step 4: Return to Read mode */
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
   return rRetVal;

} /* EndFunction FlashCheckBlockProtection */




/*******************************************************************************
Function:     void FlashEnterExtendedBlock (void);
Arguments:    None
Return Value: None

Description:  This function is used to send the Enter Extended block command to
   the device
Pseudo Code:
   Step 1:  Send the Enter Extended Block command to the device
*******************************************************************************/
void FlashEnterExtendedBlock( void ) {
   /* Step 1: Send the Unlock Bypass command */
   FlashWrite( ConvAddr(0x0555), (uCPUBusType)CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x02AA), (uCPUBusType)CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x0555), (uCPUBusType)CMD(0x0088) ); /* 3rd Cycle */
} /* EndFunction FlashEnterExtendedBlock */


/*******************************************************************************
Function:     void FlashExitExtendedBlock (void);
Arguments:    None
Return Value: None
Description:  This function is used to send the Exit Extended Block to the device
Pseudo Code:
   Step 1:  Send the Exit Extended Block command to the device
*******************************************************************************/
void FlashExitExtendedBlock( void ){
   /* Step 1: Send the Exit Extended Block command to the device */
   FlashWrite( ConvAddr(0x0555), (uCPUBusType)CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x02AA), (uCPUBusType)CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x0555), (uCPUBusType)CMD(0x0090) ); /* 3rd Cycle */
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x0000) );/* 4th Cycle */
} /* EndFunction FlashExitExtendedBlock */



/*******************************************************************************
Function:      ReturnType FlashCheckProtectionMode( void )
Arguments:     None
Return Values: The function returns the Modified protection mode:
   Flash_DefaultStandard,
   Flash_StandardProtection,
   Flash_PasswordProtection,
   Flash_Default_Mode

Description:   This function is used to read the Modified Protection Mode.
Pseudo Code:
    Step 1:  Send Enter Lock Register Command Set command
    Step 2:  Read Lock Register
    Step 3:  judge the mode
    Step 4:  Exit Protection Command Set and return to Read Array mode
*******************************************************************************/
ReturnType FlashCheckProtectionMode( void ) {
   ReturnType  rRetVal = Flash_Default_Mode; /* Holds the return value */
   uCPUBusType ucProtStatus;

    /* Step 1:  Send Enter Lock Register Command Set command  */
   FlashWrite( ConvAddr(0x00555), CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), CMD(0x0040) ); /* 3rd Cycle */

   /* Step 2:  Read Lock Register*/
   ucProtStatus = FlashRead( ANY_ADDR) ;

   /*	 Step 3:  judge the mode*/
   switch(ucProtStatus&(PASSWORD_MODE_LOCKBIT|NVP_MODE_LOCKBIT))
   {
        case 0x00:
			rRetVal = Flash_WrongType;         /* can't be the two mode at the same time*/
			break;
	 case 0x04:
			rRetVal = Flash_NV_Protection_Mode;   /* NVP bit is zero, NVP mode*/
			break;
        case 0x02:
			rRetVal = Flash_Password_Protection_Mode;   /* Password  bit is zero, Password protection mode*/
			break;
	 case 0x06:
	 	       rRetVal = Flash_Default_Mode;
   }

    /* Step 4:  Exit Protection Command Set and return to Read Array mode */
   FlashExitProtection();

   return rRetVal;

} /* EndFunction FlashCheckProtectionMode */




/*******************************************************************************
Function:      ReturnType FlashSetNVProtectionMode( void )
Arguments:     None
Return Values: The function returns the following conditions:
   Flash_Success,
   Flash_SpecificError,
   Flash_ProgramFailed

Description:   This function is used to set the device into Standard Protection Mode
Pseudo Code:
   Step 1:  Send Enter Lock Register Command Set command
   Step 2:  Read Lock Register
   Step 3:  Judge the Lock Register, if in Password mode or NVP mode, then return
   Step 4:  program the NVP mode lock bit
   Step 5:  Follow Data Toggle Flow Chart until Program/Erase Controller completes
   Step 6:  Exit Protection Command Set and return to Read Array mode
   Step 7:  Verify the program
*******************************************************************************/
ReturnType FlashSetNVProtectionMode( void ) {
   ReturnType  rRetVal; /* Holds the return value */
   uCPUBusType ucProtStatus;

   /* Step 1:  Send Enter Lock Register Command Set command   */
   FlashWrite( ConvAddr(0x00555), CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), CMD(0x0040) ); /* 3rd Cycle */

   /* Step 2:  Read Lock Register*/
   ucProtStatus = FlashRead( ANY_ADDR) ;

   /* Step 3a: Judge the Lock Register, if in Password mode, then return */
   if((ucProtStatus&PASSWORD_MODE_LOCKBIT)==0)
   {
       rRetVal = Flash_Password_Protection_Mode;

       FlashExitProtection();  /*exit protection command set*/

	return rRetVal;
   }

   /* Step 3b: Judge the Lock Register, if already in NVP mode, then return*/
   if((ucProtStatus&NVP_MODE_LOCKBIT)==0)
   {
       rRetVal = Flash_NV_Protection_Mode;

       FlashExitProtection();  /*exit protection command set*/

	return rRetVal;
   }

   /* Step 4: program the NVP mode lock bit*/
      FlashWrite( ANY_ADDR, CMD(0x00A0) ); /* 1st Cycle */
      FlashWrite( ANY_ADDR, CMD(ucProtStatus&(~NVP_MODE_LOCKBIT))); /* 2nd Cycle */

   /* Step 5: Follow Data Toggle Flow Chart until Program/Erase Controller completes */
   if( FlashDataToggle(5) !=  Flash_Success ) {
      /* Return to Read mode (if an error occurred) */
      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
      rRetVal=Flash_ProgramFailed;
   }

   /* Step 6:  Exit Protection Command Set and return to Read Array mode */
   FlashExitProtection();


   /* Step 7:  Verify the program */
   rRetVal = FlashCheckProtectionMode();
   if(rRetVal == Flash_NV_Protection_Mode)
   {
       rRetVal = Flash_Success;
   }
   else
   	rRetVal = Flash_ProgramFailed;

   return rRetVal;

} /* EndFunction FlashSetStandardProtection */




/*******************************************************************************
Function:      ReturnType FlashSetPasswordProtection( void )
Arguments:     None
Return Values: The function returns the following conditions:
   Flash_Success,
   Flash_SpecificError,

Description:   This function set the device into Password Protection mode
Pseudo Code:
   Step 1:   Send Enter Lock Register Command Set command
   Step 2:  Read Lock Register
   Step 3:  Judge the Lock Register, if in Password mode or NVP mode, then return
   Step 4:  program the password mode lock bit
   Step 5:  Follow Data Toggle Flow Chart until Program/Erase Controller completes
   Step 7:  Return to Read Array mode
   Step 7:  Verify the program
*******************************************************************************/
ReturnType FlashSetPasswordProtectionMode( void ) {
   ReturnType  rRetVal; /* Holds the return value */
   uCPUBusType ucProtStatus;

   /* Step 1:  Send Enter Lock Register Command Set command   */
   FlashWrite( ConvAddr(0x00555), CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), CMD(0x0040) ); /* 3rd Cycle */


   /* Step 2:  Read Lock Register*/
   ucProtStatus = FlashRead( ANY_ADDR) ;

   /* Step 3a: Judge the Lock Register, if in Password mode, then return */
   if((ucProtStatus&PASSWORD_MODE_LOCKBIT)==0)
   {
       rRetVal = Flash_Password_Protection_Mode;

       FlashExitProtection();  /*exit protection command set*/

	return rRetVal;
   }

   /* Step 3b: Judge the Lock Register, if already in NVP mode, then return*/
   if((ucProtStatus&NVP_MODE_LOCKBIT)==0)
   {
       rRetVal = Flash_NV_Protection_Mode;

       FlashExitProtection();  /*exit protection command set*/

	return rRetVal;
   }

   /* Step 4: program the Password mode lock bit*/
      FlashWrite( ANY_ADDR, CMD(0x00A0) ); /* 1st Cycle */
      FlashWrite( ANY_ADDR, CMD(ucProtStatus&(~PASSWORD_MODE_LOCKBIT))); /* 2nd Cycle */

   /* Step 5: Follow Data Toggle Flow Chart until Program/Erase Controller completes */
   if( FlashDataToggle(5) !=  Flash_Success ) {
      /* Return to Read mode (if an error occurred) */
      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
      rRetVal=Flash_ProgramFailed;
   }

   /* Step 6:  Exit Protection Command Set and return to Read Array mode */
   FlashExitProtection();

   /* Step 7:  Verify the program */
   rRetVal = FlashCheckProtectionMode();
   if(rRetVal == Flash_Password_Protection_Mode)
   {
       rRetVal = Flash_Success;
   }
   else
   	rRetVal = Flash_ProgramFailed;

   return rRetVal;

} /* EndFunction FlashSetPasswordProtection */


/*******************************************************************************
Function:      ReturnType FlashSetExtendedRomProtection( void )
Arguments:     None
Return Values: The function returns the following conditions:
   Flash_Success,
   Flash_SpecificError,
   Flash_ProgramFailed

Description:   This function is used to set the extended rom in protection mode
Pseudo Code:
   Step 1:  Send Enter Lock Register Command Set command
   Step 2:  Read Lock Register
   Step 3:  program the extended block protection bit
   Step 4:  Follow Data Toggle Flow Chart until Program/Erase Controller completes
   Step 5:  Check operation status
   Step 6:  Exit Protection Command Set and return to Read Array mode
*******************************************************************************/
ReturnType FlashSetExtendedBlockProtection( void ) {
   ReturnType  rRetVal; /* Holds the return value */
   uCPUBusType ucProtStatus;

   /* Step 1:  Send Enter Lock Register Command Set command   */
   FlashWrite( ConvAddr(0x00555), CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), CMD(0x0040) ); /* 3rd Cycle */

   /* Step 2:  Read Lock Register*/
   ucProtStatus = FlashRead( ANY_ADDR) ;


   /* Step 3: program the extended block protection bit*/
      FlashWrite( ANY_ADDR, CMD(0x00A0) ); /* 1st Cycle */
      FlashWrite( ANY_ADDR, CMD(ucProtStatus&(~EXTENDED_BLOCK_PROTECTION_BIT))); /* 2nd Cycle */

   /* Step 4: Follow Data Toggle Flow Chart until Program/Erase Controller completes */
   if( FlashDataToggle(5) !=  Flash_Success ) {
      /* Return to Read mode (if an error occurred) */
      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
      rRetVal=Flash_ProgramFailed;
   }
   /* Step 5:  Exit Protection Command Set and return to Read Array mode */
   FlashExitProtection();

   return rRetVal;
} /* EndFunction FlashSetExtendedRomProtection */



/*******************************************************************************
Function:      ReturnType FlashCheckBlockNVPB( uBlockType ublBlockNr )
Arguments:     ublBlockNr = block number to be set
   Note: the first block is Block 0
Return Values: The function returns the following conditions:
   Flash_NonVolatile_Protected
   Flash_NonVolatile_Unprotected
   Flash_NonVolatile_Unclear

Description:   This function is used to check the Non-Volatile Protection Bit of a block.
Pseudo Code:
    Step 1:  Check that the block number exists
    Step 2:  Send Enter Non-volatile Protection command
    Step 3:  Read NVPB
    Step 4:  Judge NVPB state
    Step 5:  Exit Protection Command Set and return to Read Array mode
*******************************************************************************/
ReturnType FlashCheckBlockNVPB( uBlockType ublBlockNr ) {
   ReturnType  rRetVal; /* Holds the return value */
   uCPUBusType ucProtStatus; /* Holds the protection status */

   /* Step 1: Check that the block number exists */
   if ( ublBlockNr >= ublNumBlocks )
      return Flash_BlockNrInvalid;

   /* Step 2: Send Enter Non-volatile Protection command */
   FlashWrite( ConvAddr(0x00555), CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), CMD(0x00C0) ); /* 3rd Cycle */

   /* Step 3:  Read NVPB */
   ucProtStatus = FlashRead(BlockOffset(ublBlockNr));

   /* Step 4:  Judge NVPB state*/
   if (ucProtStatus ==0x0000)
   {
      rRetVal = Flash_NonVolatile_Protected;
   }
   else if (ucProtStatus == CMD(0x0001))
      rRetVal = Flash_NonVolatile_Unprotected;
   else
      rRetVal = Flash_NonVolatile_Unclear;

   /* Step 5: Exit Protection Command Set and return to Read Array mode */
   FlashExitProtection();

   return rRetVal;

} /* EndFunction FlashCheckBlockNVPB */


/*******************************************************************************
Function:      ReturnType FlashClearBlockNVPB( uBlockType ublBlockNr )
Arguments:     ublBlockNr = block number to be checked
   Note: the first block is Block 0
Return Values: The function returns the following conditions:
   Flash_Success
   Flash_ProgramFailed
   Flash_NVPB_locked
   Flash_NVPB_Unclear

Description:   This function is used to clear the Non-Volatile Modify Protection bit of all blocks.
    All blocks NVPB bit will be set and then be cleared to prevent damage.
Pseudo Code:
    Step 1:  Check the NVPB lok bit
    Step 2:  Send Enter NVPB Command Set command
    Step 3:  Erase operation
    Step 4:  Follow Data Toggle Flow Chart until Program/Erase Controller completes
    Step 5: Exit Protection Command Set and return to Read Array mode
*******************************************************************************/
ReturnType FlashClearAllBlockNVPB( ) {
   ReturnType  rRetVal=Flash_Success; /* Holds the return value */
   uCPUBusType ucProtStatus = 1; /* Holds the protection status */

   /* Step 1: Check the NVPB lok bit*/
   rRetVal = FlashCheckNVPBLockBit();

   if(rRetVal == Flash_NVPB_Unlocked)
   {
	   /* Step 2: Send Enter NVPB Command Set command*/
	   FlashWrite( ConvAddr(0x00555), CMD(0x00AA) ); /* 1stn Cycle */
	   FlashWrite( ConvAddr(0x002AA), CMD(0x0055) ); /* 2nd Cycle */
	   FlashWrite( ConvAddr(0x00555), CMD(0x00C0) ); /* 3nd Cycle */


	   /* Step 3: Erase operation */
	    FlashWrite( ANY_ADDR, CMD(0x0080) ); /* 1st Cycle */
        FlashWrite( 0x0000, CMD(0x0030) ); /* 2nd Cycle */

	   /* Step 4: Follow Data Toggle Flow Chart until Program/Erase Controller completes */
	   if( FlashDataToggle(5) !=  Flash_Success ) {
	      /* Return to Read mode (if an error occurred) */
	      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
	      rRetVal=Flash_ProgramFailed;
	    }
    }

   /* Step 5: Exit Protection Command Set and return to Read Array mode */
   FlashExitProtection();
   return rRetVal;
} /* EndFunction FlashClearBlockNVPB */



/*******************************************************************************
Function:      ReturnType FlashSetBlockNVPB( uBlockType ublBlockNr )
Arguments:     ublBlockNr = block number to be set
   Note: the first block is Block 0
Return Values: The function returns the following conditions:
   Flash_BlockNrInvalid
   Flash_Success
   Flash_ProgramFailed
   Flash_NonVolatile_Unprotected
   Flash_NonVolatile_Unclear

Description:   This function is used to set the Non-Volatile Modify Protection bit of a block.
Pseudo Code:
    Step 1:  Check Range of Block Number Parameter
    Step 2:  verify the Non-volatile Protection Bit, if already set , then exit
    Step 3:  Send Enter Non-volatile Protection command
    Step 4:  Program the  Non-volatile Protection Bit
    Step 5:  Follow Data Toggle Flow Chart until Program/Erase Controller completes
    Step 6:  Exit Protection Command Set and return to Read Array mode
    Step 7:  verify the NVPB state
*******************************************************************************/
ReturnType FlashSetBlockNVPB( uBlockType ublBlockNr ) {
   ReturnType  rRetVal; /* Holds the return value */

   /* Step 1: Check that the block number exists */
   if ( ublBlockNr >= ublNumBlocks )
      return Flash_BlockNrInvalid;


   /* Step 2: Check the NVPB lok bit*/
   rRetVal = FlashCheckNVPBLockBit();

   if(rRetVal == Flash_NVPB_Unlocked)
   {
	   /* Step 3: verify the Non-volatile Protection Bit*/
	   rRetVal = FlashCheckBlockNVPB(ublBlockNr);

	   if(rRetVal == Flash_NonVolatile_Unprotected)
	   {
		   /* Step 4: Send Enter Non-volatile Protection command */
		   FlashWrite( ConvAddr(0x00555), CMD(0x00AA) ); /* 1st Cycle */
		   FlashWrite( ConvAddr(0x002AA), CMD(0x0055) ); /* 2nd Cycle */
		   FlashWrite( ConvAddr(0x00555), CMD(0x00C0) ); /* 3rd Cycle */


		   /* Step 5:  Program the  Non-volatile Protection Bit*/
		   FlashWrite( ANY_ADDR, CMD(0x00A0) ); /* 1st Cycle */
		   FlashWrite( BlockOffset(ublBlockNr) , CMD(0x0000) ); /* 2nd Cycle */

		   /* Step 6: Follow Data Toggle Flow Chart until Program/Erase Controller completes */
		   if( FlashDataToggle(5) !=  Flash_Success ) {
		      /* Return to Read mode (if an error occurred) */
		      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
		      rRetVal=Flash_ProgramFailed;
	           }

		   /* Step 7: Exit Protection Command Set and return to Read Array mode */
	            FlashExitProtection();

		   /* Step 8: verify the NVPB state*/
		   rRetVal = FlashCheckBlockNVPB(ublBlockNr);

		   if(rRetVal ==  Flash_NonVolatile_Protected)
		   	rRetVal = Flash_Success;
		   else
		   	rRetVal = Flash_ProgramFailed;
	   }
   }
   return rRetVal;

} /* EndFunction FlashSetBlockNVPB */




/*******************************************************************************
Function:      ReturnType FlashCheckNVPBLockBit( void )
Arguments:     none
Return Values: The function returns the following conditions:
   Flash_NVPB_Locked
   Flash_NVPB_Unlocked
   Flash_NVPB_Unclear

Description:   This function is used to check the NVPB Lock Bit status.

                     Note that:
                     1)there is only one NVPB Lock Bit per device which is volatile.
                     2)There is no software way to clear(erase to '1') of this bit
                     unless in Password Protection Mode
                     3)It can be clear(erase to '1') by hardware means in Non-
                     volatile Protection mode, like a power up or a hardware reset.
Pseudo Code:
    Step 1: Send Enter NVPB Lock Bit Command Set  command
    Step 2: Read NVPB Lock bit  Status
    Step 3: Judge the state of NVPB Lock Bit
    Step 4: Exit Protection Command Set and return to Read Array mode
*******************************************************************************/
ReturnType FlashCheckNVPBLockBit( void ) {
   ReturnType  rRetVal; /* Holds the return value */
   uCPUBusType ucProtStatus; /* Holds the protection status */

   /* Step 1: Send Enter NVPB Lock Bit Command Set  command */
   FlashWrite( ConvAddr(0x00555), CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), CMD(0x0050) ); /* 3nd Cycle */

   /* Step 2: Read NVPB Lock bit  Status */
   ucProtStatus = FlashRead(ANY_ADDR);

   /* Step 3: Judge the state of NVPB Lock Bit*/
   if (ucProtStatus ==0x0000)
   {
      rRetVal = Flash_NVPB_Locked;
   }
   else if (ucProtStatus == CMD(0x0001))
      rRetVal = Flash_NVPB_Unlocked;
   else
      rRetVal = Flash_NVPB_Unclear;

  /* Step 4: Exit Protection Command Set and return to Read Array mode */
   FlashExitProtection();

   return rRetVal;

} /* EndFunction FlashCheckNVPBLockBit */




/*******************************************************************************
Function:      ReturnType FlashSetNVPBLockBit( void )
Arguments:  none
Return Values: The function returns the following conditions:
   Flash_Success
   Flash_NVPB_Locked
   Flash_NVPB_Unclear
   Flash_ProgramFailed

Description:   This function used to set the NVPB Lock Bit.

                     Note that:
                     1)there is only one NVPB Lock Bit per device which is volatile.
                     2)There is no software way to clear(erase to '1') of this bit
                     unless in Password Protection Mode
                     3)It can be clear(erase to '1') by hardware means in Non-
                     volatile Protection mode, like a power up or a hardware reset.
Pseudo Code:
    Step 1:  check NVPB Lock Bit, if locked then exit
    Step 2 : Send Enter NVPB Lock Bit Command Set  command
    Step 3:  Send Program NVPB Lock Bit command
    Step 4:  Follow Data Toggle Flow Chart until Program/Erase Controller completes
    Step 5:  Exit Protection Command Set and return to Read Array mode
    Step 6:  Verify NVPB Lock Bit

*******************************************************************************/
ReturnType FlashSetNVPBLockBit( void) {
   ReturnType  rRetVal = Flash_Success; /* Holds the return value */


   /* Step 1 : check NVPB Lock Bit*/
   rRetVal = FlashCheckNVPBLockBit();
   if(rRetVal == Flash_NVPB_Unlocked)
  {
	   /* Step 2: Send Enter NVPB Lock Bit Command Set  command */
	   FlashWrite( ConvAddr(0x00555), CMD(0x00AA) ); /* 1st Cycle */
	   FlashWrite( ConvAddr(0x002AA), CMD(0x0055) ); /* 2nd Cycle */
	   FlashWrite( ConvAddr(0x00555), CMD(0x0050) ); /* 3nd Cycle */

	   /* Step 3: Send Program NVPB Lock Bit command*/
	   FlashWrite( ANY_ADDR, CMD(0x00A0) ); /* 1st Cycle */
	   FlashWrite( ANY_ADDR, CMD(0x0000) ); /* 2nd Cycle */

	   /* Step 4: Follow Data Toggle Flow Chart until Program/Erase Controller completes */
	   if( FlashDataToggle(5) !=  Flash_Success ) {
	      /* Return to Read mode (if an error occurred) */
	      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
	      rRetVal=Flash_ProgramFailed;
           }

	   /* Step 5: Exit Protection Command Set and return to Read Array mode */
   	   FlashExitProtection();

          /* Step 6: Verify NVPB Lock Bit*/
	    rRetVal = FlashCheckNVPBLockBit();
           if(rRetVal == Flash_NVPB_Locked)
		   	rRetVal = Flash_Success;
	    else
			rRetVal = Flash_ProgramFailed;

  }


   return rRetVal;

} /* EndFunction FlashSetNVPBLockBit */




/*******************************************************************************
Function:      ReturnType FlashCheckBlockVPB( uBlockType ublBlockNr )
Arguments:     ublBlockNr = block number to be checked
   Note: the first block is Block 0
Return Values: The function returns the following conditions:
   Flash_BlockNrInvalid
   Flash_Volatile_Protected
   Flash_Volatile_Unprotected
   Flash_Volatile_Unclear

Description:   This function used to set the Lock bit of a block.
Pseudo Code:
    Step 1:  Check Range of Block Number Parameter
    Step 2:  Send Enter Volatile Protection Command Set
    Step 3:  Read VPB
    Step 4:  Judge Volatile Protection Status
    Step 5:  Exit Protection Command Set and return to Read Array mode

*******************************************************************************/
ReturnType FlashCheckBlockVPB( uBlockType ublBlockNr ) {

  ReturnType  rRetVal; /* Holds the return value */
  uCPUBusType ucProtStatus; /* Holds the protection status */

     /* Step 1: Check that the block number exists */
   if ( ublBlockNr >= ublNumBlocks )
      return Flash_BlockNrInvalid;

   /* Step 2: Send Enter Volatile Protection Command Set */
   FlashWrite( ConvAddr(0x00555), CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), CMD(0x00E0) ); /* 3nd Cycle */

   /* Step 3: Read the VPB */
   ucProtStatus = FlashRead( BlockOffset(ublBlockNr) ); /* 1st Cycle */

    /* Step 4: Judge Volatile Protection Status */
   if( ucProtStatus == 0x0000 )
       rRetVal =  Flash_Volatile_Protected;
   else if( ucProtStatus == 0x0001 )
       rRetVal = Flash_Volatile_Unprotected;
   else
   	rRetVal =  Flash_Volatile_Unclear;

   /* Step 5: Exit Protection Command Set and return to Read Array mode */
   FlashExitProtection();

   return rRetVal;

} /* EndFunction FlashCheckBlockVPB */


/*******************************************************************************
Function:      ReturnType FlashClearBlockVPB( uBlockType ublBlockNr )
Arguments:     ublBlockNr = block number to be checked
   Note: the first block is Block 0
Return Values: The function returns the following conditions:
   Flash_BlockNrInvalid
   Flash_Success
   Flash_ProgramFailed

Description:   This function is used to clear the Lock bit of a block.
Pseudo Code:
    Step 1:  Check Range of Block Number Parameter
    Step 2:  Send Enter Volatile Protection Command Set
    Step 3:  Clear the VPB to '1'
    Step 4:  Follow Data Toggle Flow Chart until Program/Erase Controller completes
    Step 5:  Exit Protection Command Set and return to Read Array mode
    Step 6:  Read Volatile Protection Status

*******************************************************************************/
ReturnType FlashClearBlockVPB( uBlockType ublBlockNr ) {

   ReturnType  rRetVal;

   /* Step 1: Check that the block number exists */
   if ( ublBlockNr >= ublNumBlocks )
      return Flash_BlockNrInvalid;

   /* Step 2: Send Enter Volatile Protection Command Set */
   FlashWrite( ConvAddr(0x00555), CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), CMD(0x00E0) ); /* 3nd Cycle */

  /* Step 3: Clear the VPB */
   FlashWrite( ANY_ADDR, CMD(0x00A0) ); /* 1st Cycle */
   FlashWrite( BlockOffset(ublBlockNr), CMD(0x0001) ); /* 2nd Cycle */

   /* Step 4: Follow Data Toggle Flow Chart until Program/Erase Controller completes */
   if( FlashDataToggle(5) !=  Flash_Success ) {
      /* Return to Read mode (if an error occurred) */
      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
      rRetVal=Flash_ProgramFailed;
    }

   /* Step 5: Exit Protection Command Set and return to Read Array mode */
   FlashExitProtection();

   /* Step 6: Read Volatile Protection Status */
   if( FlashCheckBlockVPB(ublBlockNr) == Flash_Volatile_Unprotected )
       rRetVal =  Flash_Success;
   else
       rRetVal =  Flash_ProgramFailed;

   return rRetVal;

} /* EndFunction FlashClearBlockVPB */


/*******************************************************************************
Function:      ReturnType FlashSetBlockVPB( uBlockType ublBlockNr )
Arguments:     ublBlockNr = block number to be checked
   Note: the first block is Block 0
Return Values: The function returns the following conditions:
   Flash_BlockNrInvalid
   Flash_Success
   Flash_ProgramFailed

Description:   This function used to set the Lock bit of a block.
Pseudo Code:
    Step 1:  Check Range of Block Number Parameter
    Step 2:  Send Enter Volatile Protection Command Set
    Step 3:  Program the VPB to '0'
    Step 4:  Follow Data Toggle Flow Chart until Program/Erase Controller completes
    Step 5:  Exit Protection Command Set and return to Read Array mode
    Step 6:  Read Volatile Protection Status

*******************************************************************************/
ReturnType FlashSetBlockVPB( uBlockType ublBlockNr ) {

   ReturnType  rRetVal;

   /* Step 1: Check that the block number exists */
   if ( ublBlockNr >= ublNumBlocks )
      return Flash_BlockNrInvalid;

   /* Step 2: Send Enter Volatile Protection Command Set */
   FlashWrite( ConvAddr(0x00555), CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), CMD(0x00E0) ); /* 3nd Cycle */

   /* Step 3: Program the VPB */
   FlashWrite( ANY_ADDR, CMD(0x00A0) ); /* 1st Cycle */
   FlashWrite( BlockOffset(ublBlockNr), CMD(0x0000) ); /* 2nd Cycle */

   /* Step 4: Follow Data Toggle Flow Chart until Program/Erase Controller completes */
   if( FlashDataToggle(5) !=  Flash_Success ) {
      /* Return to Read mode (if an error occurred) */
      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
      rRetVal=Flash_ProgramFailed;
    }

   /* Step 5: Exit Protection Command Set and return to Read Array mode */
   FlashExitProtection();

    /* Step 6: Read Volatile Protection Status */
   if( FlashCheckBlockVPB(ublBlockNr) == Flash_Volatile_Protected )
       rRetVal =  Flash_Success;
   else
       rRetVal =  Flash_ProgramFailed;

   return rRetVal;

} /* EndFunction FlashSetBlockVPB */




/*******************************************************************************
Function:      ReturnType FlashPasswordProgram( uCPUBusType *ucpPWD )
Arguments:     uwPWD = Password to program
Return Values: The function returns the following conditions:
   Flash_Success
   Flash_ProgramFailed

Description:   This function is used to set the password(64 bit) for password protection mode.
Pseudo Code:
    Step 1: Send Enter Password Command Set command
    Step 2: Write password (1 word)
    Step 3: Wait until Program/Erase Controller has completed
    Step 4: Return to Read Array mode

*******************************************************************************/
ReturnType FlashPasswordProgram( uCPUBusType *ucpPWD ) {
   ReturnType   rRetVal = Flash_Success; /* Holds the return value */
   udword i,data;

   /* Step 1: Send Enter Password Command Set command */
   /*
      Note:
      For 2-Gbit (1-Gbit/1-Gbit) device, all the set-up command should be re-issued to the device
      when different die is selected.
   */
       FlashWrite( ConvAddr(0x00555), CMD(0x00AA) ); /* 1st Cycle */
       FlashWrite( ConvAddr(0x002AA), CMD(0x0055) ); /* 2nd Cycle */
       FlashWrite( ConvAddr(0x00555), CMD(0x0060) ); /* 3nd Cycle */

   /* Step 2: Write password (1 word) */
#if defined (USE_16BIT_FLASH)
   for(i=0;i<4;i++)
#endif
#if defined (USE_8BIT_FLASH)
   for(i=0;i<8;i++)
#endif
   {
       FlashWrite( ANY_ADDR, CMD(0x00A0) );
       FlashWrite(  i , CMD(*(ucpPWD+i)) );
       FlashPause(2);
   }
         /* Step 4: Exit Protection Command Set and return to Read Array mode */
       FlashExitProtection();

   return rRetVal;

} /* EndFunction FlashPasswordProgram */




/*******************************************************************************
Function:      ReturnType FlashVerifyPassword( unsigned short *uwPWD )
Arguments:     *uwPWD = Password to read
Return Values: The function always returns :
   Flash_Success

Description:   This function verify the password(4 word).
Pseudo Code:
    Step 1: Send Enter Password Command Set command
    Step 2: Read password
    Step 3: Return to Read Array mode

*******************************************************************************/
ReturnType FlashVerifyPassword( uCPUBusType *uwPWD ) {
   udword i,data;

   /* Step 1: Send Enter Password Command Set command */
       FlashWrite( ConvAddr(0x00555), CMD(0x00AA) ); /* 1st Cycle */
       FlashWrite( ConvAddr(0x002AA), CMD(0x0055) ); /* 2nd Cycle */
       FlashWrite( ConvAddr(0x00555), CMD(0x0060) ); /* 3nd Cycle */

   /* Step 2: Read password  */
#if defined (USE_16BIT_FLASH)
   for(i=0;i<4;i++)
#endif
#if defined (USE_8BIT_FLASH)
   for(i=0;i<8;i++)
#endif
   {
       *(uwPWD+i) = FlashRead( i );
   }

    /* Step 3: Exit Protection Command Set  */
       FlashExitProtection();

   return Flash_Success;

} /* EndFunction FlashVerifyPassword */




/*******************************************************************************
Function:      ReturnType FlashPasswordProtectionUnlock( uCPUBusType *ucpPWD )
Arguments:     *uwPWD = Password to write
Return Values: The function always returns :
   Flash_Success

Description:   This function is used to clear the NVPB Lock Bit under Password Protect mode.
	This operation will unprotect all Non-volatile Modify Protection bits when the device is in
    Password Protection mode.
Pseudo Code:
    Step 1: Send unlock cycle  and issue program command
    Step 2: Write password (4 words)
    Step 3: Follow Data Toggle Flow Chart until Program/Erase Controller completes
    Step 4: Exit Protection Command Set

*******************************************************************************/
ReturnType FlashPasswordProtectionUnlock( uCPUBusType *ucpPWD ) {
   ReturnType rRetVal;
   ubyte i = 0;

      /* Step 1a: Check Input parameters */
   if (NULL == ucpPWD)
      return Flash_ResponseUnclear;

    /* Step 1: Send Enter Password Command Set command  */
   FlashWrite( ConvAddr(0x00555), CMD(0x00AA) ); /* 1st Cycle */
   FlashWrite( ConvAddr(0x002AA), CMD(0x0055) ); /* 2nd Cycle */
   FlashWrite( ConvAddr(0x00555), CMD(0x0060) ); /* 3nd Cycle */

   /* Step 2: Write password (4 words) */
   FlashWrite( 0x00000, CMD(0x0025) ); /* 1st Cycle */
   FlashWrite( 0x00000, CMD(0x0003) ); /* 2nd Cycle */


 #if defined (USE_16BIT_FLASH)
	for(i=0;i<4;i++)
  #endif
 #if defined (USE_8BIT_FLASH)
     for(i=0;i<8;i++)
  #endif
   {
       FlashWrite( i, CMD(*(ucpPWD+i)));
   }

   FlashPause(10);

   FlashWrite( 0x00000, CMD(0x0029) ); /*7th Cycle*/

    /* Step 3: Follow Data Toggle Flow Chart until Program/Erase Controller completes */
   if( FlashDataToggle(5) !=  Flash_Success ) {
    /* Return to Read mode (if an error occurred) */
      FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x00F0) ); /* Use single instruction cycle method */
      rRetVal=Flash_ProgramFailed;
    }

   /* Step 4: Exit Protection Command Set  */
   FlashExitProtection();

   return Flash_Success;

} /* EndFunction FlashPasswordProtectionUnlock */



/*******************************************************************************
Function:     void FlashExitProtection (void);
Arguments:    None
Return Value: None
Description:  This function is used to send the Exit Protection command to the device
Pseudo Code:
   Step 1:  Send the Exit Protection  command to the device
*******************************************************************************/
void FlashExitProtection( void ){
   /* Step 1: Send the Exit Protection  command to the device */
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x0090) ); /* 1st Cycle */
   FlashWrite( ANY_ADDR, (uCPUBusType)CMD(0x0000) ); /* 2nd Cycle */
} /* EndFunction FlashExitProtection */


/*******************************************************************************
Function:    void FlashPause( udword udMicroSeconds )
Arguments:   udMicroSeconds is the length of the pause in microseconds
Returns:     None
Description: This routine returns after udMicroSeconds have elapsed. It is used
   in several parts of the code to generate a pause required for correct
   operation of the flash part.

Pseudo Code:
   Step 1: Initilize clkReset variable.
   Step 2: Count to the required size.
*******************************************************************************/

#ifdef TIME_H_EXISTS
/*-----------------------------------------------------------------------------
 Note:The Routine uses the function clock() inside of the ANSI C library "time.h".
-----------------------------------------------------------------------------*/

static void FlashPause(udword udMicroSeconds){
   clock_t clkReset,clkCounts;

   /* Step 1: Initialize clkReset variable */
   clkReset=clock();

   /* Step 2: Count to the required size */
   do
      clkCounts = clock()-clkReset;
   while (clkCounts < ((CLOCKS_PER_SEC/1e6L)*udMicroSeconds));

} /* EndFunction FlashPause */
#else

/*-----------------------------------------------------------------------------
Note: The routine here works by counting. The user may already have a more suitable
      routine for timing which can be used.
-----------------------------------------------------------------------------*/

static void FlashPause(udword udMicroSeconds) {
   static udword udCounter;

   /* Step 1: Compute the count size */
   udCounter = udMicroSeconds * COUNT_FOR_A_MICROSECOND;

   /* Step 2: Count to the required size */
   while( udCounter > 0 ) /* Test to see if finished */
      udCounter--; /* and count down */
} /* EndFunction FlashPause */

#endif

#ifdef VERBOSE
/*******************************************************************************
Function:     byte *FlashErrorStr( ReturnType rErrNum );
Arguments:    rErrNum is the error number returned from other Flash Routines
Return Value: A pointer to a string with the error message
Description:  This function is used to generate a text string describing the
   error from the flash. Call with the return value from other flash routines.

Pseudo Code:
   Step 1: Return the correct string.
*******************************************************************************/
byte *FlashErrorStr( ReturnType rErrNum ) {

   switch(rErrNum) {
      case Flash_Success:
         return "Flash - Success";
      case Flash_FunctionNotSupported:
         return "Flash - Function not supported";
      case Flash_AddressInvalid:
         return "Flash - Address is out of Range";
      case Flash_BlockEraseFailed:
         return "Flash - Block Erase failed";
      case Flash_BlockNrInvalid:
         return "Flash - Block Number is out of Range";
      case Flash_BlockProtected:
         return "Flash - Block is protected";
      case Flash_BlockProtectFailed:
         return "Flash - Block Protection failed";
      case Flash_BlockProtectionUnclear:
         return "Flash - Block Protection Status is unclear";
      case Flash_BlockUnprotected:
         return "Flash - Block is unprotected";
      case Flash_CfiFailed:
         return "Flash - CFI Interface failed";
      case Flash_ChipEraseFailed:
         return "Flash - Chip Erase failed";
      case Flash_ChipUnprotectFailed:
         return "Flash - Chip Unprotect failed";
      case Flash_GroupProtectFailed:
         return "Flash - Group Protect Failed";
      case Flash_NoInformationAvailable:
         return "Flash - No Information Available";
      case Flash_OperationOngoing:
         return "Flash - Operation ongoing";
      case Flash_OperationTimeOut:
         return "Flash - Operation TimeOut";
      case Flash_ProgramFailed:
         return "Flash - Program failed";
      case Flash_ResponseUnclear:
         return "Flash - Response unclear";
      case Flash_SpecificError:

         switch (eiErrorInfo.sprRetVal) {
            case FlashSpec_MpuTooSlow:
               return "Flash - Flash MPU too slow";
            case FlashSpec_TooManyBlocks:
               return "Flash - Too many Blocks";
            case FlashSpec_ToggleFailed:
               return "Flash - Toggle failed";
            default:
               return "Flash - Undefined Specific Error";
         } /* EndSwitch eiErrorInfo */

      case Flash_WrongType:
         return "Flash - Wrong Type";
      default:
         return "Flash - Undefined Error Value";
   } /* EndSwitch */
} /* EndFunction FlashErrorString */
#endif /* VERBOSE Definition */


/*******************************************************************************
Function:     ReturnType FlashDataToggle( void )
Arguments:    none
Return Value: The function returns Flash_Success if the Program/Erase Controller
   is successful or Flash_SpecificError if there is a problem.In this case
   the field eiErrorInfo.sprRetVal will be filled with FlashSpec_ToggleFailed value.
   If the Program/Erase Controller do not finish before time-out expired
   the function return Flash_OperationTimeout.

Description:  The function is used to monitor the Program/Erase Controller during
   erase or program operations. It returns when the Program/Erase Controller has
   completed. In the Data Sheets, the Bit Toggle Flow Chart shows the operation
   of the function.

Pseudo Code:
   Step 1: Read DQ5 and DQ6 (into word)
   Step 2: Read DQ6 (into  another a word)
   Step 3: If DQ6 did not toggle between the two reads then return Flash_Success
   Step 4: Else if DQ5 is zero and DQ1 is zero then operation is not yet complete,
              goto 1
   Step 5: Else (DQ5 != 0) or (DQ1 != 0), read DQ6 again
   Step 6: If DQ6 did not toggle between the last two reads then return
              Flash_Success
   Step 7: Else return Flash_ToggleFail
*******************************************************************************/
static ReturnType FlashDataToggle( int timeout ){

   uCPUBusType ucVal1, ucVal2; /* hold values read from any address offset within
                                  the Flash Memory */

   FlashTimeOut(0); /* Initialize TimeOut Counter */
   while(FlashTimeOut(timeout) != Flash_OperationTimeOut) {
      /* TimeOut: If, for some reason, the hardware fails then this
         loop exit and the function return flash_OperationTimeOut.  */

      /* Step 1: Read DQ5 and DQ6 (into  word) */
      ucVal2 = FlashRead( ANY_ADDR ); /* Read DQ5 and DQ6 from the Flash (any
                                         address) */

      /* Step 2: Read DQ6 (into another a word) */
      ucVal1 = FlashRead( ANY_ADDR ); /* Read DQ6 from the Flash (any address) */


      /* Step 3: If DQ6 did not toggle between the two reads then return
                 Flash_Success */
      if( (ucVal1&CMD(0x0040)) == (ucVal2&CMD(0x0040)) ) /* DQ6 == NO Toggle */
         return Flash_Success;

      /* Step 4: Else if DQ5 is zero then check abort bit */
      if( (ucVal2&CMD(0x0020)) == 0)
		  	/* if the abort bit is clear continue, else check for toggle */
		  	if ( (ucVal2&CMD(0x02)) == 0)
      	   continue;

      /* Step 5: Else (DQ5 == 1) or (DQ1 == 1), read DQ6 twice */
      ucVal1 = FlashRead( ANY_ADDR ); /* Read DQ6 from the Flash (any address) */
      ucVal2 = FlashRead( ANY_ADDR );
      /* Step 6: If DQ6 did not toggle between the last two reads then
                 return Flash_Success */
      if( (ucVal2&CMD(0x0040)) == (ucVal1&CMD(0x0040)) ) /* DQ6 == NO Toggle  */
         return Flash_Success;

      /* Step 7: Else return Flash_ToggleFailed */
      else {
         /* DQ6 == Toggle here means fail */
			if ((ucVal2&CMD(0x02)) != 0)
			{
         	eiErrorInfo.sprRetVal=(SpecificReturnType)Flash_WriteToBufferAbort;
	         return Flash_SpecificError;
			}
			else
			{
         	eiErrorInfo.sprRetVal=FlashSpec_ToggleFailed;
	         return Flash_SpecificError;
			}
      } /* EndInf */
   } /* EndWhile */
   return Flash_OperationTimeOut; /*if exit from while loop then time out exceeded */
} /* EndFunction FlashDataToggle */


/*******************************************************************************
Function:     ReturnType FlashTimeOut(udword udSeconds)

Arguments:    fSeconds holds the number of seconds before giving a TimeOut
Return Value: The function returns the following conditions:
   Flash_OperationTimeOut
   Flash_OperationOngoing

Example:   FlashTimeOut(0)  // Initializes the Timer

           While(1) {
              ...
              If (FlashTimeOut(5) == Flash_OperationTimeOut) break;
              // The loop is executed for 5 Seconds before leaving it
           } EndWhile

*******************************************************************************/
#ifdef TIME_H_EXISTS
/*-----------------------------------------------------------------------------
Description:   This function realizes a timeout for flash polling actions or
   other operations which would otherwise never return.
   The Routine uses the function clock() inside ANSI C library "time.h".
-----------------------------------------------------------------------------*/
ReturnType FlashTimeOut(udword udSeconds){
   static clock_t clkReset,clkCount;

   if (udSeconds == 0) { /* Set Timeout to 0 */
      clkReset=clock();
   } /* EndIf */

   clkCount = clock() - clkReset;

   if (clkCount<(CLOCKS_PER_SEC*(clock_t)udSeconds))
      return Flash_OperationOngoing;
   else
      return Flash_OperationTimeOut;
} /* EndFunction FlashTimeOut */

#else
/*-----------------------------------------------------------------------------
Description:   This function realizes a timeout for flash polling actions or
   other operations which would otherwise never return.
   The Routine uses COUNT_FOR_A_SECOND which describes the performance of
   the current Hardware. If I count in a loop to COUNT_FOR_A_SECOND
   I would reach 1 Second. Needs to be adopted to the current Hardware.
-----------------------------------------------------------------------------*/
ReturnType FlashTimeOut(udword udSeconds) {

   static udword udCounter;

   if (udSeconds == 0) { /* Set Timeout to 0 */
      udCounter = 0;
   } /* EndIf */

   if (udCounter == (udSeconds * COUNT_FOR_A_SECOND)) {
      return Flash_OperationTimeOut;
   } else {
      udCounter++;
      return Flash_OperationOngoing;
   } /* EndIf */

} /* EndFunction FlashTimeOut */
#endif /* TIME_H_EXISTS */


/*******************************************************************************
 End of M29EW_128Mb-2Gb_device_driver.c
*******************************************************************************/

