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


*************** User Change Area *******************************************

   This section is meant to give all the opportunities to customize the
   SW Drivers according to the requirements of hardware and flash configuration.
   It is possible to choose flash start address, CPU Bitdepth, number of flash
   chips, hardware configuration and performance data (TimeOut Info).

   The options are listed and explained below:

   ********* Data Types *********
   The source code defines hardware independent datatypes assuming the
   compiler implements the numerical types as

   unsigned char    8 bits (defined as ubyte)
   char             8 bits (defined as byte)
   unsigned short  16 bits (defined as uword)
   short           16 bits (defined as word)
   unsigned int    32 bits (defined as udword)
   int             32 bits (defined as dword)

   In case the compiler does not support the currently used numerical types,
   they can be easily changed just once here in the user area of the headerfile.
   The data types are consequently referenced in the source code as (u)byte,
   (u)word and (u)dword. No other data types like 'CHAR','SHORT','INT','LONG'
   directly used in the code.


   ********* Flash Type *********
   This source file provides library C code for using the M29W flash devices.
   The following device is supported in the code:
      M29W128, M29W256, M29W512

   This file can be used to access the devices in 8bit and 16bit mode.

   ********* Base Address *********
   The start address where the flash memory chips are "visible" within
   the memory of the CPU is called the BASE_ADDR. This address must be
   set according to the current system. This value is used by FlashRead()
   FlashWrite(). Some applications which require a more complicated
   FlashRead() or FlashWrite() may not use BASE_ADDR.


   ********* Flash and Board Configuration *********
   The driver supports also different configurations of the flash chips
   on the board. In each configuration a new data Type called
   'uCPUBusType' is defined to match the current CPU data bus width.
   This data type is then used for all accesses to the memory.

   The different options (defines) are explained below:

   - USE_16BIT_CPU_ACCESSING_2_8BIT_FLASH
   Using this define enables a configuration consisting of an environment
   containing a CPU with 16bit databus and 2 8bit flash chips connected
   to it.

   - USE_16BIT_CPU_ACCESSING_1_16BIT_FLASH
   Using this define enables a configuration consisting of an environment
   containing a CPU with 16bit databus and 1 16bit flash chip connected
   to it. Standard Configuration

   - USE_32BIT_CPU_ACCESSING_4_8BIT_FLASH
   Using this define enables a configuration consisting of an environment
   containing a CPU with 32bit databus and 4 8bit flash chips connected
   to it.

   - USE_32BIT_CPU_ACCESSING_2_16BIT_FLASH
   Using this define enables a configuration consisting of an environment
   containing a CPU with 32bit databus and 2 16bit flash chips connected
   to it.

   - USE_8BIT_CPU_ACCESSING_1_8BIT_FLASH
  Using this define enables a configuration consisting of an environment
  containing a CPU with 8bit databus and 1 8bit flash chips connected
  to it.

  ********* FlashInit **********
   This function has to be used in order to initialize the driver.
   The FlashInit() function set the internal variables and the flash device description,
   so it should be called before using any other function.
   The function returns Flash_Success if the device is supported.

   ********* TimeOut *********
   There are timeouts implemented in the loops of the code, in order
   to enable a timeout for operations that would otherwise never terminate.
   There are two possibilities:

   1) The ANSI Library functions declared in 'time.h' exist

      If the current compiler supports 'time.h' the define statement
      TIME_H_EXISTS should be activated. This makes sure that
      the performance of the current evaluation HW does not change
      the timeout settings.

   2) or they are not available (COUNT_FOR_A_SECOND)

      If the current compiler does not support 'time.h', the define
      statement can not be used. To overcome this constraint the value
      COUNT_FOR_A_SECOND has to be defined in order to create a one
      second delay. For example, if 100000 repetitions of a loop are
      needed, to give a time delay of one second, then
      COUNT_FOR_A_SECOND should have the value 100000.

   ********* Pause Constant *********
   The function Flashpause() is used in several areas of the code to
   generate a delay required for correct operation of the flash device.
   There are two options provided:


   1) The Option ANSI Library functions declared in 'time.h' exists
      If the current compiler supports 'time.h' the define statement TIME_H_EXISTS should be
	  activated. This makes sure that the performance of the current evaluation HW does not
	  change the timeout settings.

      #define TIME_H_EXISTS

    2) The Option COUNT_FOR_A_MICROSECOND
      If the current compiler does not support 'time.h', the define statement TIME_H_EXISTS can
	  not be used. To overcome this constraint the value COUNT_FOR_A_MICROSECOND has to be defined
	  in order to create a one micro second delay.
      Depending on a 'While(count-- != 0);' loop a value has to be found which creates the
	  necessary delay.
      - An approximate approach can be given by using the clock frequency of the test plattform.
	  That means if an evaluation board with 200 Mhz is used, the value for COUNT_FOR_A_MICROSECOND
	  would be: 200.
      - The real exact value can only be found using a logic state analyser.

      #define COUNT_FOR_A_MICROSECOND (chosen value).

      Note: This delay is HW (Performance) dependent and needs,
      therefore, to be updated with every new HW.

      This driver has been tested with a certain configuration and other
      target platforms may have other performance data, therefore, the
      value may have to be changed.

      It is up to the user to implement this value to avoid the code
      timing out too early instead of completing correctly.


   ********* Additional Routines *********
   The drivers provides also a subroutine which displays the full
   error message instead of just an error number.

   The define statement VERBOSE activates additional Routines.
   Currently it activates the function FlashErrorStr()

   No further changes should be necessary.

*****************************************************************************/

#ifndef __M29_DEVICE_DRIVER__H__
#define __M29_DEVICE_DRIVER__H__

typedef unsigned char  ubyte; /* All HW dependent Basic Data Types */
typedef          char   byte;
typedef unsigned short uword;
typedef          short  word;
typedef unsigned int  udword;
typedef          int   dword;

typedef ubyte      NMX_uint8; /* All HW dependent Basic Data Types */
typedef byte       NMX_sint8;
typedef uword      NMX_uint16;
typedef word       NMX_sint16;
typedef udword     NMX_uint32;
typedef dword      NMX_sint32;

#define FLASH_BASE_ADDRESS 0

#undef USE_8BIT_FLASH
#undef USE_16BIT_FLASH

#define USE_16BIT_CPU_ACCESSING_1_16BIT_FLASH /* Current PCB Info */
/* Possible Values:
                    USE_16BIT_CPU_ACCESSING_2_8BIT_FLASH
                    USE_16BIT_CPU_ACCESSING_1_16BIT_FLASH
                    USE_32BIT_CPU_ACCESSING_4_8BIT_FLASH
                    USE_32BIT_CPU_ACCESSING_2_16BIT_FLASH
                    USE_8BIT_CPU_ACCESSING_1_8BIT_FLASH*/


#define TIME_H_EXISTS  /* set this macro if C-library "time.h" is supported */
/* Possible Values: TIME_H_EXISTS
                    - no define - TIME_H_EXISTS */


#ifndef TIME_H_EXISTS
   #define COUNT_FOR_A_SECOND 100000    /* Timer Usage */
   #define COUNT_FOR_A_MICROSECOND 20  /* Used in FlashPause function */
#endif

//#define VERBOSE /* Activates additional Routines */
/* Currently the Error String Definition */

/********************** End of User Change Area *****************************/


/*****************************************************************************
HW Structure Info, Usage of the Flash Memory (Circuitry)
*****************************************************************************/

/******************************************************************************
   For 2-Gbit (1-Gbit/1-Gbit) device, all the set-up command should be re-issued to the device
   when different die is selected.
******************************************************************************/

#ifdef USE_16BIT_CPU_ACCESSING_2_8BIT_FLASH
   typedef uword uCPUBusType;
   typedef  word  CPUBusType;
   #define USE_8BIT_FLASH
   #define FLASH_BIT_DEPTH 8
   #define HEX "04Xh"
   #define CMD(A)  ((A<<8)+A)
   #define NUM_DEVICES 2
#endif

#ifdef USE_16BIT_CPU_ACCESSING_1_16BIT_FLASH
   typedef uword uCPUBusType;
   typedef  word  CPUBusType;
   #define USE_16BIT_FLASH
   #define FLASH_BIT_DEPTH 16
   #define HEX "04Xh"
   #define CMD(A) (A)
   #define NUM_DEVICES 1
#endif

#ifdef USE_32BIT_CPU_ACCESSING_4_8BIT_FLASH
   typedef udword uCPUBusType;
   typedef  dword  CPUBusType;
   #define USE_8BIT_FLASH
   #define FLASH_BIT_DEPTH 8
   #define HEX "08Xh"
   #define CMD(A) (A+(A<<24)+(A<<16)+(A<<8))
   #define NUM_DEVICES 4
#endif

#ifdef USE_32BIT_CPU_ACCESSING_2_16BIT_FLASH
   typedef udword uCPUBusType;
   typedef  dword  CPUBusType;
   #define USE_16BIT_FLASH
   #define HEX "08Xh"
   #define CMD(A) (A+(A<<16))
   #define NUM_DEVICES 2
#endif

#ifdef USE_8BIT_CPU_ACCESSING_1_8BIT_FLASH
   typedef ubyte uCPUBusType;
   typedef  byte  CPUBusType;
   #define USE_8BIT_FLASH
   #define FLASH_BIT_DEPTH 8
   #define HEX "02Xh"
   #define CMD(A) (A)
   #define NUM_DEVICES 1
#endif


#ifdef USE_8BIT_FLASH
   #define FLASH_BIT_DEPTH 8
   #define MANUFACTURER (0x89)  /* Intel Code is 0x89 */
   #define EXPECTED_DEVICE (0x7E)  /* Device code for the M29EW128_8 */

   #define ShAddr(A) (A<<1)            /* Exclude A-1 address bit in ReadCFi
                                          and Protect/Unprotect commands */
   #define ConvAddr(A) (2*A+!(A&0x1))  /* Convert a word mode command to byte mode command :
                                           Word Mode Command    Byte Mode Command
                                                0x555      ->     0xAAA
                                                0x2AA      ->     0x555
                                                0x55       ->     0xAA            */
   #define BlockOffset(BlockNr)    (BlockNr<<0x11)
   #define CONFIGURATION_DEFINED
#endif

#ifdef USE_16BIT_FLASH
   #define FLASH_BIT_DEPTH 16
   #define MANUFACTURER (0x0089)  /* Intel Code is 0x89 */
   #define EXPECTED_DEVICE (0x227E)  /* Device code for the M29EW128_8 */
   #define ShAddr(A) (A)               /* Used to supports the 8bit Commands */
   #define ConvAddr(A) (A)             /* Used to supports the 8bit Commands */
   #define BlockOffset(BlockNr)    (BlockNr<<0x10)
   #define CONFIGURATION_DEFINED
#endif


/*******************************************************************************
Device Specific Return Codes
*******************************************************************************/

typedef enum {
    FlashSpec_InvalidDeviceIdNr,
    FlashSpec_MpuTooSlow,
    FlashSpec_ToggleFailed,
    FlashSpec_TooManyBlocks
} SpecificReturnType;

/*******************************************************************************
     CONFIGURATION CHECK
*******************************************************************************/

#ifndef CONFIGURATION_DEFINED
#error  User Change Area Error: PCB Info uncorrect Check the USE_xxBIT_CPU_ACCESSING_n_yyBIT_FLASH Value
#endif

/*******************************************************************************
     DERIVED DATATYPES
*******************************************************************************/

/******** CommandsType ********/

typedef enum {
   BankErase,
   BankReset,
   BankResume,
   BankSuspend,
   BlockErase,
   BlockProtect,
   BlockUnprotect,
   BufferProgram,
   CheckBlockProtection,
   CheckCompatibility,
   ChipErase,
   ChipUnprotect,
   GroupProtect,
   Program,
   Read,
   ReadCfi,
   ReadDeviceId,
   ReadManufacturerCode,
   Reset,
   Resume,
   SingleProgram,
   Suspend,
   Write
} CommandType;


/******** ReturnType ********/

typedef enum {
   Flash_AddressInvalid, 				// 0
   Flash_BankEraseFailed,
   Flash_BlockEraseFailed,
   Flash_BlockNrInvalid,
   Flash_BlockProtected,
   Flash_BlockProtectFailed, 			// 5
   Flash_BlockProtectionUnclear,
   Flash_BlockUnprotected,
   Flash_BlockUnprotectFailed,
   Flash_CfiFailed,
   Flash_InvalidParameter,				// 10
   Flash_ChipEraseFailed,
   Flash_ChipUnprotectFailed,
   Flash_FunctionNotSupported,
   Flash_GroupProtectFailed,
   Flash_NoInformationAvailable,		//15
   Flash_NoOperationToSuspend,
   Flash_OperationOngoing,
   Flash_OperationTimeOut,
   Flash_ProgramFailed,
   Flash_ResponseUnclear, 				// 20
   Flash_SpecificError,
   Flash_Success, 							// 22
   Flash_VppInvalid,
   Flash_WrongType,
   Flash_Default_Mode,						// 25
   Flash_NV_Protection_Mode,
   Flash_Password_Protection_Mode,
   Flash_ExtendedRom_Protection,
   Flash_NVPB_Unlocked,
   Flash_NVPB_Locked,						// 30
   Flash_NVPB_Unclear,
   Flash_NonVolatile_Protected,
   Flash_NonVolatile_Unprotected,
   Flash_NonVolatile_Unclear,
   Flash_Volatile_Protected,			// 35
   Flash_Volatile_Unprotected,
   Flash_Volatile_Unclear,
   Flash_WriteToBufferAbort
} ReturnType;

/******** BlockType ********/

typedef uword uBlockType;

/******** ParameterType ********/

typedef union {
    /**** BankErase Parameters ****/
    struct {
      uBlockType ublBlockNr;
      ReturnType *rpResults;
    } BankErase;

    /**** BankReset Parameters ****/
    struct {
      udword udBankAddrOff;
    } BankReset;

    /**** BankResume Parameters ****/
    struct {
      udword udAddrOff;
    } BankResume;

    /**** BankSuspend Parameters ****/
    struct {
      udword udAddrOff;
    } BankSuspend;

    /**** BlockErase Parameters ****/
    struct {
      uBlockType ublBlockNr;
    } BlockErase;

    /**** BlockProtect Parameters ****/
    struct {
      uBlockType ublBlockNr;
    } BlockProtect;

    /**** BlockUnprotect Parameters ****/
    struct {
      uBlockType ublBlockNr;
    } BlockUnprotect;

    /**** CheckBlockProtection Parameters ****/
    struct {
      uBlockType ublBlockNr;
    } CheckBlockProtection;

    /**** CheckCompatibility has no parameters ****/

    /**** ChipErase Parameters ****/
    struct {
      ReturnType *rpResults;
    } ChipErase;

    /**** ChipUnprotect Parameters ****/
    struct {
      ReturnType *rpResults;
    } ChipUnprotect;

    /**** GroupProtect Parameters ****/
    struct {
      uBlockType ublBlockNr;
    } GroupProtect;

    /**** Program Parameters ****/
    struct {
      udword udAddrOff;
      udword udNrOfElementsInArray;
        void *pArray;
      udword udMode;
    } Program;

    /**** Read Parameters ****/
    struct {
      udword  udAddrOff;
      uCPUBusType ucValue;
    } Read;

    /**** ReadCfi Parameters ****/
    struct {
      uword  uwCfiFunc;
      uCPUBusType ucCfiValue;
    } ReadCfi;

    /**** ReadDeviceId Parameters ****/
    struct {
      uCPUBusType ucDeviceId;
    } ReadDeviceId;

    /**** ReadManufacturerCode Parameters ****/
    struct {
      uCPUBusType ucManufacturerCode;
    } ReadManufacturerCode;

    /**** Reset has no parameters ****/

    /**** Resume has no parameters ****/

    /**** SingleProgram Parameters ****/
    struct {
      udword udAddrOff;
      uCPUBusType ucValue;
    } SingleProgram;

    /**** Suspend has no parameters ****/

    /**** Write Parameters ****/
    struct {
      udword udAddrOff;
      uCPUBusType ucValue;
    } Write;

} ParameterType;

/******** ErrorInfoType ********/

typedef struct {
  SpecificReturnType sprRetVal;
  udword             udGeneralInfo[4];
} ErrorInfoType;

/******************************************************************************
    Global variables
*******************************************************************************/
extern ErrorInfoType eiErrorInfo;



/*******************************************************************************
Device Constants
*******************************************************************************/

#define ANY_ADDR (0)  /* Any address offset within the Flash Memory will do */
#define PASSWORD_MODE_LOCKBIT (0x04)
#define NVP_MODE_LOCKBIT  (0x02)
#define EXTENDED_BLOCK_PROTECTION_BIT (0x01)

/******************************************************************************
    Standard functions
*******************************************************************************/
	ReturnType	Flash_2Gb( CommandType cmdCommand, ParameterType *fp );
  ReturnType  Flash( CommandType cmdCommand, ParameterType *fp );
  ReturnType  FlashBlockErase( uBlockType ublBlockNr );
  ReturnType  FlashCheckCompatibility( void );
  ReturnType  FlashChipErase( ReturnType  *rpResults );
  ReturnType  FlashMultipleBlockErase(uBlockType ublNumBlocks,uBlockType *ublpBlock,ReturnType * rpResult);
  ReturnType  FlashProgram( udword udMode, udword udAddrOff, udword udNrOfElementsInArray, void *pArray );
  uCPUBusType FlashRead( udword udAddrOff );
  ReturnType  FlashReadCfi( uword uwCFIFunc, uCPUBusType *ucpCFIValue );
  ReturnType  FlashInit ( void );
  ReturnType  FlashReadDeviceId( uCPUBusType *ucpDeviceID);
  ReturnType  FlashReadManufacturerCode( uCPUBusType *ucpManufacturerCode);
  ReturnType  FlashReadMultipleDeviceId(uword uwDeviceIdNr, uCPUBusType *ucpDeviceId );
  ReturnType  FlashReset( void );
  ReturnType  FlashResume( void );
  ReturnType  FlashSingleProgram( udword udAddrOff, uCPUBusType ucVal );
  ReturnType  FlashSuspend( void );
  void       FlashWrite(udword udAddrOff, uCPUBusType ucVal);


/******************************************************************************
    Fast program functions
*******************************************************************************/
ReturnType  FlashBufferProgram( udword udMode,udword udAddrOff, udword udNrOfElementsInArray, void *pArray );
ReturnType  FlashBufferProgramAbort( void );
ReturnType  FlashBufferProgramConfirm( udword udAddrOff );
ReturnType  FlashWriteToBufferProgram(udword udMode,udword udAddrOff, udword udNrOfElementsInArray, void *pArray );

ReturnType  FlashUnlockBypass( void );
ReturnType  FlashUnlockBypassProgram (udword udAddrOff, udword NumWords, void *pArray);
      void  FlashUnlockBypassReset( void );
ReturnType  FlashUnlockBypassBlockErase( uBlockType ublBlockNr);
ReturnType  FlashUnlockBypassBufferProgram( udword udMode,udword udAddrOff, udword udNrOfElementsInArray, void *pArray );
ReturnType  FlashUnlockBypassChipErase( void );
ReturnType  FlashUnlockBypassMultipleBlockErase(uBlockType ublNumBlocks,uBlockType *ublpBlock,ReturnType * rpResult);



/******************************************************************************
    Utility functions
*******************************************************************************/
static ReturnType FlashCheckBlockEraseError( uBlockType ublBlock );
static ReturnType FlashDataToggle( int timeout );

#ifdef VERBOSE
   byte *FlashErrorStr( ReturnType rErrNum );
#endif

static void  FlashPause(udword udMicroSeconds);
ReturnType  FlashResponseIntegrityCheck( uCPUBusType *ucpFlashResponse );
ReturnType  FlashTimeOut( udword udSeconds );
/*******************************************************************************
Specific Function Prototypes
********************************************************************************/


/*******************************************************************************
Protection Function Prototypes
********************************************************************************/
ReturnType  FlashCheckBlockProtection( uBlockType ublBlockNr );
      void  FlashEnterExtendedBlock( void );
      void  FlashExitExtendedBlock( void );
ReturnType  FlashReadExtendedBlockVerifyCode( uCPUBusType *ucpVerifyCode );


ReturnType FlashCheckProtectionMode( void );

ReturnType FlashSetNVProtectionMode( void );
ReturnType FlashSetPasswordProtectionMode( void );
ReturnType FlashSetExtendedBlockProtection( void );

ReturnType FlashSetNVPBLockBit( void );
ReturnType FlashCheckNVPBLockBit( void );

ReturnType FlashCheckBlockNVPB( uBlockType ublBlockNr );
ReturnType FlashClearAllBlockNVPB( void );
ReturnType FlashSetBlockNVPB( uBlockType ublBlockNr );

ReturnType FlashCheckBlockVPB( uBlockType ublBlockNr );
ReturnType FlashClearBlockVPB( uBlockType ublBlockNr );
ReturnType FlashSetBlockVPB( uBlockType ublBlockNr );

ReturnType FlashPasswordProgram( uCPUBusType *uwPWD );
ReturnType FlashVerifyPassword( uCPUBusType *pwd );
ReturnType FlashPasswordProtectionUnlock( uCPUBusType *pwd );

void       FlashExitProtection( void );

extern NMX_uint32  udDeviceSize;  		/* Flash size in uCPUBusType elems */
extern uBlockType ublNumBlocks;  		/* Number of Blocks in the device  */
extern uBlockType ublBlocksPerDie;  	/* used for multiple-die devices   */
extern NMX_uint32  udBlockSize;  		/* Block size of the device        */
extern NMX_uint32  udDieSize;			/* used for multiple-die devices   */

/*******************************************************************************
List of Errors and Return values, Explanations and Help.
********************************************************************************

Error Name:   Flash_AddressInvalid
Description:  The address offset given is out of the range of the flash device.
Solution:     Check the address offset whether it is in the valid range of the
              flash device.
********************************************************************************

Error Name:   Flash_BankEraseFailed
Description:  The bank erase command did not finish successfully.
Solution:     Check that Vpp is not floating. Try erasing the block again. If
              this fails once more, the device may be faulty and needs to be
              replaced.
********************************************************************************

Error Name:   Flash_BlockEraseFailed
Description:  The block erase command did not finish successfully.
Solution:     Check that Vpp is not floating. Try erasing the block again. If
              this fails once more, the device may be faulty and needs to be
              replaced.
********************************************************************************

Error Name:   Flash_BlockNrInvalid
Note:         This is not a flash problem.
Description:  A selection for a block has been made (Parameter), which is not
              within the valid range. Valid block numbers are from 0 to
              ublNumBlocks-1.
Solution:     Check that the block number given is in the valid range.
********************************************************************************

Error Name:   Flash_BlockProtected
Description:  The user has attempted to erase, program or protect a block of
              the flash that is protected. The operation failed because the
              block in question is protected. This message appears also after
              checking the protection status of a block.
Solutions:    Choose another (unprotected) block for erasing or programming.
              Alternatively change the block protection status of the current
              block (see Datasheet for more details). In case of the user is
              protecting a block that is already protected, this warning notifies
              the user that the command had no effect.
********************************************************************************

Error Name:   Flash_BlockProtectFailed
Description:  This error return value indicates that a block protect command did
              not finish successfully.
Solution:     Check that Vpp is not floating but is tied to a valid voltage. Try
              the command again. If it fails a second time then the block cannot
              be protected and it may be necessary to replace the device.
********************************************************************************

Error Name:   Flash_BlockProtectionUnclear
Description:  The user has attempted to erase, program or protect a block of
              the flash which did not return a proper protection status. The
              operation has been cancelled.
Solutions:    This should only happen in configurations with more than one
              flash device. If the response of each device does not match, this
              return code is given. Mostly it means the usage of not properly
              initialized flash devices.
********************************************************************************

Error Name:   Flash_BlockUnprotected
Description:  This message appears after checking the block protection status.
              This block is ready to get erased, programmed or protected.
********************************************************************************

Error Name:   Flash_BlockUnprotectFailed
Description:  This error return value indicates that a block unprotect command
              did not finish successfully.
Solution:     Check that Vpp is not floating but is tied to a valid voltage. Try
              the command again. If it fails a second time then the block cannot
              be unprotected and it may be necessary to replace the device.
********************************************************************************

Error Name:   Flash_CfiFailed
Description:  This return value indicates that a Common Flash Interface (CFI)
              read access was unsuccessful.
Solution:     Try to read the Identifier Codes (Manufacture ID, Device ID)
              if these commands fail as well it is likely that the device is
              faulty or the interface to the flash is not correct.
********************************************************************************

Error Name:   Flash_ChipEraseFailed
Description:  This message indicates that the erasure of the whole device has
              failed.
Solution:     Investigate this failure further by checking the results array
              (parameter), where all blocks and their erasure results are listed.
              What is more, try to erase each block individually. If erasing a
              single block still causes failure, then the Flash device may need
              to be replaced.
********************************************************************************

Error Name:   Flash_ChipUnprotectFailed
Description:  This return value indicates that the chip unprotect command
              was unsuccessful.
Solution:     Check that Vpp is not floating but is tied to a valid voltage. Try
              the command again. If it fails a second time then it is likely that
              the device cannot be unprotected and will need to be replaced.
********************************************************************************

Return Name:  Flash_FunctionNotSupported
Description:  The user has attempted to make use of functionality not
              available on this flash device (and thus not provided by the
              software drivers).
Solution:     This can happen after changing Flash SW Drivers in existing
              environments. For example an application tries to use
              functionality which is then no longer provided with a new device.
********************************************************************************

Error Name:   Flash_GroupProtectFailed
Description:  This error return value indicates that a group protect command did
              not finish successfully.
Solution:     Check that Vpp is not floating but is tied to a valid voltage. Try
              the command again. If it fails a second time then the group cannot
              be protected and it may be necessary to replace the device.
********************************************************************************

Return Name:  Flash_NoInformationAvailable
Description:  The system can't give any additional information about the error.
Solution:     None
********************************************************************************

Error Name:   Flash_NoOperationToSuspend
Description:  This message is returned by a suspend operation if there isn't
              operation to suspend (i.e. the program/erase controller is inactive).
********************************************************************************

Error Name:   Flash_OperationOngoing
Description:  This message is one of two messages which are given by the TimeOut
              subroutine. That means the flash operation still operates within
              the defined time frame.
********************************************************************************

Error Name:   Flash_OperationTimeOut
Description:  The Program/Erase Controller algorithm could not finish an
              operation successfully. It should have set bit 7 of the status
              register from 0 to 1, but that did not happen within a predefined
              time. The program execution has been, therefore, cancelled by a
              timeout. This may be because the device is damaged.
Solution:     Try the previous command again. If it fails a second time then it
              is likely that the device will need to be replaced.
********************************************************************************

Error Name:   Flash_ProgramFailed
Description:  The value that should be programmed has not been written correctly
              to the flash.
Solutions:    Make sure that the block which is supposed to receive the value
              was erased successuly before programming. Try erasing the block and
              programming the value again. If it fails again then the device may
              be faulty.
********************************************************************************

Error Name:   Flash_ResponseUnclear
Description:  This message appears in multiple flash configurations, if the
              single flash responses are different and, therefore, a sensible
              reaction of the SW driver is not possible.
Solutions:    Check all the devices currently used and make sure they are all
              working properly. Use only equal devices in multiple configurations.
              If it fails again then the devices may be faulty and need to be
              replaced.
********************************************************************************

Error Name:   Flash_SpecificError
Description:  The function makes an error depending on the device.
              More information about the error are available into the ErrorInfo
              variable.
Solutions:    See SpecificReturnType remarks
********************************************************************************

Return Name:  Flash_Success
Description:  This value indicates that the flash command has executed
              correctly.
********************************************************************************

Error Name:   Flash_VppInvalid
Description:  A Program or a Block Erase has been attempted with the Vpp supply
              voltage outside the allowed ranges. This command had no effect
              since an invalid Vpp has the effect of protecting the whole of
              flash device.
Solution:     The (hardware) configuration of Vpp will need to be modified to
              enable programming or erasing of the device.
*******************************************************************************

Error Name:   Flash_WrongType
Description:  This message appears if the Manufacture and Device ID read from
              the current flash device do not match with the expected identifier
              codes. That means the source code is not explicitely written for
              the currently used flash chip. It may work, but it cannot be
              guaranteed.
Solutions:    Use a different flash chip with the target hardware or contact
              STMicroelectronics for a different source code library.
********************************************************************************/
/*******************************************************************************
List of Specific Errors, Explanations and Help.
********************************************************************************


Error Name:   FlashSpec_MpuTooSlow
Notes:        Applies to M29 series Flash only.
Description:  The MPU has not managed to write all of the selected blocks to the
              device before the timeout period expired. See BLOCK ERASE COMMAND
              section of the Data Sheet for details.
Solutions:    If this occurs occasionally then it may be because an interrupt is
              occuring between writing the blocks to be erased. Search for "DSI!" in
              the code and disable interrupts during the time critical sections.
              If this error condition always occurs then it may be time for a faster
              microprocessor, a better optimising C compiler or, worse still, learn
              assembly. The immediate solution is to only erase one block at a time.

********************************************************************************

Error Name:   FlashSpec_ToggleFailed
Notes:        This applies to M29 series Flash only.
Description:  The Program/Erase Controller algorithm has not managed to complete
              the command operation successfully. This may be because the device is
              damaged.
Solution:     Try the command again. If it fails a second time then it is likely that
              the device will need to be replaced.

******************************************************************************
Error Name:   FlashSpec_Too_ManyBlocks
Notes:        Applies to M29 series Flash only.
Description:  The user has chosen to erase more blocks than the device has.
              This may be because the array of blocks to erase contains the same
              block more than once.
Solutions:    Check that the program is trying to erase valid blocks. The device
              will only have ublNumBlocks blocks (defined at the top of the file).
              Also check that the same block has not been added twice or more to
              the array.

*******************************************************************************/

#endif /* In order to avoid a repeated usage of the header file */

/*******************************************************************************
     End of M29EW_128Mb-2Gb_device_driver.h
********************************************************************************/
