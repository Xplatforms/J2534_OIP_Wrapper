//PassThruStruct.h

/************************************************************************************************************\
* Description :  Structures for the PassThru API                                                             *
**************************************************************************************************************
* Project     :  PassThru API DLL                                                                            *
* File        :  PassThruStruct.h                                                                            *
* Version     :  1.00                                                                                        *
* Reference   :  SAE J2534 Februar 2002 Final = 02.02                                                        *
**************************************************************************************************************
* Company     :  I+ME Actia GmbH                                                                             *
\************************************************************************************************************/
#ifndef _PASSTHRU_STRUCT_H
#define _PASSTHRU_STRUCT_H

#ifdef _MSC_VER
#pragma pack (push, 1)
#endif

///////////////////////////////////////////////////////////////////////////////
//Ioctl FAST_INIT InputPtr OutputPtr
//
/**
@struct PASSTHRU_MSG
@brief This message structure will be used for all messages. The total message
 size (in bytes) is the DataSize. The ExtraDataIndex points to the IFR or
 checksum/CRC byte(s) when applicable. For consistency, all interfaces should
 detect only the errors listed for each protocol in the following sections when
 returning ERR_INVALID_MSG.
@author JT, KE, PJ
@version 1.00
@date 2002
@note defined by SAE J2435 FEB2002
*/
typedef struct
{
    unsigned long ProtocolID; /**< Protocol type */
    unsigned long RxStatus; /**< Receive message status - See RxStatus in "Message Flags and Status Definition" section */
    unsigned long TxFlags; /**< Transmit message flags - See TxFlags in "Message Flags and Status Definition" section */
    unsigned long Timestamp; /**< Received message timestamp (microseconds) */
    unsigned long DataSize; /**< Data size in bytes */
    unsigned long ExtraDataIndex; /**< Start position of extra data in received message (e.g., IFR; CRC; checksum, ...). The extra data bytes follow the body bytes in the Data array. The index is zero-based. */
    unsigned char Data[4128]; /**< Array of data bytes. */
} PASSTHRU_MSG;


///////////////////////////////////////////////////////////////////////////////
//Ioctl GET_CONFIG / SET_CONFIG InputPtr
//
/*
+------------------+-----------------+----------------------------------------+
+------------------+-----------------+----------------------------------------+
| Valid values for | Valid values    | Description                            |
|    Parameter     |   for Value     |                                        |
+------------------+-----------------+----------------------------------------+
+------------------+-----------------+----------------------------------------+
| DATA_RATE        | 5-500000        | Represents the desired baud rate.      |
|                  |                 |                                        |
|                  |                 | There is no default value.             |
+------------------+-----------------+----------------------------------------+
| PARITY           | 0 (NO_PARITY)   | For a protocol ID of ISO9141 only.     |
|                  | 1 (ODD_PARITY)  |                                        |
|                  | 2 (EVEN_PARITY) | The default value is NO_PARITY.        |
+------------------+-----------------+----------------------------------------+
| LOOPBACK         | 0 (OFF)         | Echo transmitted messages in the       |
|                  | 1 (ON)          | receive queue. Don't echo transmitted  |
|                  |                 | messages in the receive queue.         |
|                  |                 |                                        |
|                  |                 | The default value is OFF.              |
+------------------+-----------------+----------------------------------------+
| NODE_ADDRESS     | 0x00-0xFF       | For a protocol ID of J1850PWM, this    |
|                  |                 | sets the node address in the physical  |
|                  |                 | layer of the vehicle network.          |
+------------------+-----------------+----------------------------------------+
| NETWORK_LINE     | 0 (BUS_NORMAL)  | For a protocol ID of J1850PWM, this    |
|                  | 1 (BUS_PLUS)    | sets the network line(s) that are      |
|                  | 2 (BUS_MINUS)   | active during communication (for cases |
|                  |                 | where the physical layer allows this). |
|                  |                 |                                        |
|                  |                 | The default value is BUS_NORMAL.       |
+------------------+-----------------+----------------------------------------+
| P1_MIN           | 0x0-0xFFFF      | For protocol ID of ISO9141, this sets  |
|                  |                 | the minimum inter-byte time (in        |
|                  |                 | milli-seconds) for ECU responses.      |
|                  |                 |                                        |
|                  |                 | The default value is 0 milli-seconds.  |
+------------------+-----------------+----------------------------------------+
| P1_MAX           | 0x0-0xFFFF      | For protocol ID of ISO9141, this sets  |
|                  |                 | the maximum inter-byte time (in        |
|                  |                 | milli-seconds) for ECU responses (in   |
|                  |                 | milli-seconds).                        |
|                  |                 |                                        |
|                  |                 | The default value is 20 milli-seconds. |
+------------------+-----------------+----------------------------------------+
| P2_MIN           | 0x0-0xFFFF      | For protocol ID of ISO9141, this sets  |
|                  |                 | the minimum time (in milli-seconds)    |
|                  |                 | between tester request and ECU         |
|                  |                 | responses or two ECU responses.        |
|                  |                 |                                        |
|                  |                 | The default value is 25 milli-seconds. |
+------------------+-----------------+----------------------------------------+
| P2_MAX           | 0x0-0xFFFF      | For protocol ID of ISO9141, this sets  |
|                  |                 | the maximum time (in milli-seconds)    |
|                  |                 | between tester request and ECU         |
|                  |                 | responses or two ECU responses.        |
|                  |                 |                                        |
|                  |                 | The default value is 50 milli-seconds. |
+------------------+-----------------+----------------------------------------+
| P3_MIN           | 0x0-0xFFFF      | For protocol ID of ISO9141, this sets  |
|                  |                 | the minimum time (in milli-seconds)    |
|                  |                 | between end of ECU response and start  |
|                  |                 | of new tester request.                 |
|                  |                 |                                        |
|                  |                 | The default value is 55 milli-seconds. |
+------------------+-----------------+----------------------------------------+
| P3_MAX           | 0x0-0xFFFF      | For protocol ID of ISO9141, this sets  |
|                  |                 | the maximum time (in milli-seconds)    |
|                  |                 | between end of ECU response and start  |
|                  |                 | of new tester request.                 |
|                  |                 |                                        |
|                  |                 | The default value is                   |
|                  |                 | 5000 milli-seconds.                    |
+------------------+-----------------+----------------------------------------+
| P4_MIN           | 0x0-0xFFFF      | For protocol ID of ISO9141, this sets  |
|                  |                 | the minimum inter-byte (in             |
|                  |                 | milli-seconds) time for a tester       |
|                  |                 | request.                               |
|                  |                 |                                        |
|                  |                 | The default value is 5 milli-seconds.  |
+------------------+-----------------+----------------------------------------+
| P4_MAX           | 0x0-0xFFFF      | For protocol ID of ISO9141, this sets  |
|                  |                 | the maximum inter-byte (in             |
|                  |                 | milli-seconds) time for a tester       |
|                  |                 | request.                               |
|                  |                 |                                        |
|                  |                 | The default value is 20 milli-seconds. |
+------------------+-----------------+----------------------------------------+
| W1               | 0x0-0xFFFF      | For protocol ID of ISO9141, this sets  |
|                  |                 | the maximum time (in milli-seconds)    |
|                  |                 | from the end of the address byte to    |
|                  |                 | the start of the synchronization       |
|                  |                 | pattern.                               |
|                  |                 |                                        |
|                  |                 | The default value is 300 milli-seconds.|
+------------------+-----------------+----------------------------------------+
| W2               | 0x0-0xFFFF      | For protocol ID of ISO9141, this sets  |
|                  |                 | the maximum time (in milli-seconds)    |
|                  |                 | from the end of the synchronization    |
|                  |                 | pattern to the start of key byte 1.    |
|                  |                 |                                        |
|                  |                 | The default value is 20 milli-seconds. |
+------------------+-----------------+----------------------------------------+
| W3               | 0x0-0xFFFF      | For protocol ID of ISO9141, this sets  |
|                  |                 | the maximum time (in milli-seconds)    |
|                  |                 | between key byte 1 and key byte 2.     |
|                  |                 |                                        |
|                  |                 | The default value is 20 milli-seconds. |
+------------------+-----------------+----------------------------------------+
| W4               | 0x0-0xFFFF      | For protocol ID of ISO9141, this sets  |
|                  |                 | the maximum time (in milli-seconds)    |
|                  |                 | between key byte 2 and its inversion   |
|                  |                 | from the tester.                       |
|                  |                 |                                        |
|                  |                 | The default value is 50 milli-seconds. |
+------------------+-----------------+----------------------------------------+
| W5               | 0x0-0xFFFF      | For protocol ID of ISO9141, this sets  |
|                  |                 | the minimum time (in milli-seconds)    |
|                  |                 | before the tester start to transmit    |
|                  |                 | the address byte.                      |
|                  |                 |                                        |
|                  |                 | The default value is 300 milli-seconds.|
+------------------+-----------------+----------------------------------------+
| TIDLE            | 0x0-0xFFFF      | For protocol ID of ISO9141, this sets  |
|                  |                 | the amount of bus idle time that is    |
|                  |                 | needed before a fast initialization    |
|                  |                 | sequence will begin.                   |
|                  |                 |                                        |
|                  |                 | The default is the value of W5.        |
+------------------+-----------------+----------------------------------------+
| TINIL            | 0x0-0xFFFF      | For protocol ID of ISO9141, this sets  |
|                  |                 | the duration (in milli-seconds) for    |
|                  |                 | the low pulse in fast initialization.  |
|                  |                 |                                        |
|                  |                 | The default value is 25 milli-seconds. |
+------------------+-----------------+----------------------------------------+
| BIT_SAMPLE_POINT | 0-100           | For a protocol ID of CAN, this sets    |
|                  |                 | the desired bit sample point as a      |
|                  |                 | percentage of the bit time.            |
|                  |                 |                                        |
|                  |                 | The default is 80%.                    |
+------------------+-----------------+----------------------------------------+
| SYNC_JUMP_WIDTH  | 0-100           | For a protocol ID of CAN, this sets    |
|                  |                 | the desired syncronization jump width  |
|                  |                 | as a percentage of the bit time.       |
|                  |                 |                                        |
|                  |                 | The default is 15%.                    |
+------------------+-----------------+----------------------------------------+
| TWUP             | 0x0-0xFFFF      | For protocol ID of ISO9141, this sets  |
|                  |                 | the duration (in milli-seconds) of the |
|                  |                 | wake-up pulse in fast initialization.  |
|                  |                 |                                        |
|                  |                 | The default value is 50 milli-seconds. |
+------------------+-----------------+----------------------------------------+
*/

/**  
@struct SCONFIG
@brief This type will be used by PassThruIoctl to set respectively get
 configuration data. Therefore this structure will be embbeded in a list named
 SCONFIG_LIST
@author JT, KE, PJ
@version 1.00
@date 2002
@note defined by SAE J2435 FEB2002
*/
typedef struct
{
    unsigned long Parameter;    /**< name of parameter */
    unsigned long Value;        /**< value of the parameter */
} SCONFIG;

/**  
@struct SCONFIG_LIST
@brief This type will be used by PassThruIoctl to set respectively get
 configuration data.
@author JT, KE, PJ
@version 1.00
@date 2002
@note defined by SAE J2435 FEB2002
*/
typedef struct
{
    unsigned long NumOfParams;     /**< number of SCONFIG elements */
    SCONFIG *ConfigPtr;            /**< array of SCONFIG */
} SCONFIG_LIST;



///////////////////////////////////////////////////////////////////////////////
//Ioctl FIVE_BAUD_INIT InputPtr OutputPtr
//Ioctl ADD_TO_FUNCT_MSG_LOOKUP_TABLE InputPtr
//Ioctl DELETE_FROM_FUNCT_MSG_LOOKUP_TABLE InputPtr
//
/**  
@struct SBYTE_ARRAY
@brief This type will be used by PassThruIoctl by subfunction FIVE_BAUD_INIT,
 ADD_TO_FUNCT_MSG_LOOLUP_TABLE AND DELETE_FROM_FUNCT_MSG_LOOKUP_TABLE
@author JT, KE, PJ
@version 1.00
@date 2002
@note defined by SAE J2435 FEB2002
*/
typedef struct
{
     unsigned long NumOfBytes;      /**< number of bytes in the array */
     unsigned char *BytePtr;        /**< array of bytes */
} SBYTE_ARRAY;



#pragma pack (pop)

#endif //_PASSTHRU_STRUCT_H

