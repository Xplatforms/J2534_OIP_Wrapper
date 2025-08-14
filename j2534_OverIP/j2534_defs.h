#ifndef J2534_DEFS_H
#define J2534_DEFS_H


//PASS_FILTER: PassThru device adds receive messages matching the Mask and Pattern criteria to its receive message queue.
//BLOCK_FILTER: PassThru device ignores receive messages matching the Mask and Pattern criteria.
//FLOW_CONTROL_FILTER: PassThru device adds receive messages matching the Mask and Pattern criteria to its receive message queue.
//The PassThru device transmits a flow control message (only for ISO 15765-4) when receiving multi-segmented frames.


/*************************************************/
/* PassThruStartMsgFilter FilterType definitions */
/*************************************************/
enum FilterDef
{
    NONE = 0x00000000,
    PASS_FILTER = 0x00000001,
    BLOCK_FILTER = 0x00000002,
    FLOW_CONTROL_FILTER = 0x00000003,
};

enum PassThroughConnect
{
    CAN_29BIT_ID = 0x00000100,
    ISO9141_NO_CHECKSUM = 0x00000200,
    NO_CHECKSUM = ISO9141_NO_CHECKSUM,
    CAN_ID_BOTH = 0x00000800,
    ISO9141_K_LINE_ONLY = 0x00001000,

    ETH_FLAG_MANUAL_NEGOTIATE = 0x00010000,
    ETH_FLAG_HALF_DUPLEX = 0x00020000,
    ETH_FLAG_MDIX_MANUAL = 0x00040000,
    ETH_FLAG_MDIX_SWAPPED = 0x00080000,

    LISTEN_ONLY_DT = 0x10000000, //DT - listen only mode CAN
};

enum RxTxFlags
{
    ISO15765_FRAME_PAD = 0x00000040,
    WAIT_P3_MIN_ONLY = 0x00000200,
    SW_CAN_HV_TX = 0x00000400,
    TP2_0_BROADCAST_MSG = 0x00010000,
    SCI_MODE = 0x00400000,
    SCI_TX_VOLTAGE = 0x00800000,

    TX_MSG_TYPE = 0x00000001,
    START_OF_MESSAGE = 0x00000002,
    ISO15765_FIRST_FRAME = 0x00000002,
    RX_BREAK = 0x00000004,
    TX_DONE = 0x00000008,
    ISO15765_PADDING_ERROR = 0x00000010,
    ISO15765_ADDR_TYPE = 0x00000080,
    ISO15765_EXT_ADDR = 0x00000080, // Accidentally refered to in spec


    SW_CAN_HV_RX = 0x00010000,
    SW_CAN_HS_RX = 0x00020000,
    SW_CAN_NS_RX = 0x00040000,

    J1939_ADDRESS_CLAIMED   /*-2*/      = 0x00010000, // Indication
    J1939_ADDRESS_LOST      /*-2*/      = 0x00020000, // Indication
    CONNECTION_ESTABLISHED  /*-2*/      = 0x00010000, // Indication
    CONNECTION_LOST         /*-2*/      = 0x00020000, // Indication
    LINK_UP                 /*-2*/      = 0x00010000, // Indication
    LINK_DOWN               /*-2*/      = 0x00020000, // Indication
    LINK_FAULT              /*-2*/      = 0x00020000, // Indication Fault Tolerant CAN
    FD_CAN_BRS              /*-2*/      = 0x00080000, // Baud rate Select
    FD_CAN_FORMAT           /*-2*/      = 0x00100000, // CAN or CAN FD format
    FD_CAN_ESI              /*-2*/      = 0x00200000, //

    // used during Sniff mode
    DT_CRC_ERROR = 0x40000000,  // CRC error detected in message
    DT_BUF_OVERFLOW	    = 0x80000000  // Message(s) lost around this message
};

enum MiscDef
{
    SHORT_TO_GROUND = 0xFFFFFFFE,     // pin 15 only
    VOLTAGE_OFF = 0xFFFFFFFF,

    NO_PARITY = 0,
    ODD_PARITY = 1,
    EVEN_PARITY = 2,

    //SWCAN
    DISBLE_SPDCHANGE		/*-2*/	= 0,
    ENABLE_SPDCHANGE		/*-2*/	= 1,
    DISCONNECT_RESISTOR     /*-2*/  = 0,
    CONNECT_RESISTOR        /*-2*/  = 1,
    AUTO_RESISTOR			/*-2*/	= 2,

    //Mixed Mode
    CAN_MIXED_FORMAT_OFF	/*-2*/	= 0,
    CAN_MIXED_FORMAT_ON		/*-2*/	= 1,
    CAN_MIXED_FORMAT_ALL_FRAMES	/*-2*/	= 2, // Not supported

    // -2 Discovery
    NOT_SUPPORTED = 0,
    SUPPORTED = 1,

    // -2 Analog averaging
    SIMPLE_AVERAGE = 0x00000000, // Simple arithmetic mean
    MAX_LIMIT_AVERAGE = 0x00000001, // Choose the biggest value
    MIN_LIMIT_AVERAGE = 0x00000002, // Choose the lowest value
    MEDIAN_AVERAGE = 0x00000003,

    // Connect Media
    DCM_UNKNOWN = 0x00,
    DCM_UART = 0x01,
    DCM_USB = 0x02,
    DCM_BLUETOOTH = 0x03,
    DCM_802_11 = 0x04,
    DCM_ETHERNET = 0x05,

    // Repeat messaging
    REPEAT_MESSAGE_UNTIL_MATCH = 0,
    REPEAT_MESSAGE_WHILE_MATCH = 1,

    // FIVE_BAUD_MOD values
    ISO5BAUD_ISO9141_2 = 0,
    ISO5BAUD_INV_KEYBYTE2 = 1,
    ISO5BAUD_INV_ADDRESS = 2,
    ISO5BAUD_ISO9141 = 3,
    ISO5BAUD_5KEYBYTES = 4,
};

enum ProtocolId
{
    PROTOCOL_OVERLAPPED = 0x00,
    J1850VPW = 0x01,
    J1850PWM = 0x02,
    ISO9141 = 0x03,
    ISO14230 = 0x04,
    CAN = 0x05,
    ISO15765 = 0x06,
    SCI_A_ENGINE = 0x07,
    SCI_A_TRANS = 0x08,
    SCI_B_ENGINE = 0x09,
    SCI_B_TRANS = 0x0A,

    // J2534-2 Pin Switched ProtocolIDs
    J1850VPW_PS          = /*-2*/      0x8000, // supported
    J1850PWM_PS          = /*-2*/      0x8001, // supported
    ISO9141_PS           = /*-2*/      0x8002, // supported
    ISO14230_PS          = /*-2*/      0x8003, // supported
    CAN_PS               = /*-2*/      0x8004, // supported
    ISO15765_PS          = /*-2*/      0x8005, // supported
    J2610_PS             = /*-2*/      0x8006, // supported
    SW_ISO15765_PS       = /*-2*/      0x8007, // supported
    SW_CAN_PS            = /*-2*/      0x8008, // supported
    GM_UART_PS           = /*-2*/      0x8009, // Supported
    UART_ECHO_BYTE_PS    = /*-2*/      0x800A, // supported
    HONDA_DIAGH_PS       = /*-2*/      0x800B, // Supported
    J1939_PS             = /*-2*/      0x800C, // Supported
    J1708_PS             = /*-2*/      0x800D, // supported
    TP2_0_PS             = /*-2*/      0x800E, // supported
    FT_CAN_PS            = /*-2*/      0x800F, // Supported
    FT_ISO15765_PS       = /*-2*/      0x8010, // Supported
    FD_CAN_PS            = /*-2*/      0x8011, // CarDAQ-Plus 3
    FD_ISO15765_PS       = /*-2*/      0x8012, // CarDAQ-Plus 3
    ETHERNET_PS          = /*-2*/      0x8013, // CarDAQ-Plus 2 and 3
    TP1_6_PS             = /*-2*/      0x8014,
    KW82_PS              = /*Conti*/   0x80BC, // supported

    //ANALOG_IN_1 = 0xC000,
    //ANALOG_IN_2 = 0xC001, // Supported on AVIT only
    //ANALOG_IN_32 = 0xC01F,  // Not supported
};

enum IoctlId
{
    //Reserved for SAE                              0x0F - 0xFFFF
    GET_CONFIG          = 0x01,
    SET_CONFIG          = 0x02,
    READ_VBATT          = 0x03,
    FIVE_BAUD_INIT      = 0x04,
    FAST_INIT           = 0x05,

    //NOT_USED = 0x06,
    CLEAR_TX_BUFFER     = 0x07,
    CLEAR_RX_BUFFER     = 0x08,
    CLEAR_PERIODIC_MSGS = 0x09,
    CLEAR_MSG_FILTERS   = 0x0A,
    CLEAR_FUNCT_MSG_LOOKUP_TABLE = 0x0B,
    ADD_TO_FUNCT_MSG_LOOKUP_TABLE = 0x0C,
    DELETE_FROM_FUNCT_MSG_LOOKUP_TABLE = 0x0D,
    READ_PROG_VOLTAGE = 0x0E,

    //Reserved_for_SAE-2	0x00008004 - 0x0000FFFF
    SW_CAN_HS = 0x00008000,
    SW_CAN_NS = 0x00008001,
    SET_POLL_RESPONSE = 0x00008002,
    BECOME_MASTER = 0x00008003,
    START_REPEAT_MESSAGE = 0x00008004,
    QUERY_REPEAT_MESSAGE = 0x00008005,
    STOP_REPEAT_MESSAGE = 0x00008006,
    GET_DEVICE_CONFIG = 0x00008007,
    SET_DEVICE_CONFIG = 0x00008008,
    PROTECT_J1939_ADDR = 0x00008009,
    REQUEST_CONNECTION = 0x0000800A,
    TEARDOWN_CONNECTION = 0x0000800B,
    GET_DEVICE_INFO = 0x0000800C,
    GET_PROTOCOL_INFO = 0x0000800D,
    READ_PIN_VOLTAGE = 0x0000800E,

    //Tool manufacturer specific                    0x10000 - 0xFFFFFFFF
    K_LINE_MULTIPLEXER_CONTROL              = ((unsigned long)0x10000), /**< To direct the pass-thru device to drive the I+ME K-Line Multiplexer*/
    READ_VBATT_PIN_1			            = ((unsigned long)0x10001), /**< Special for BMW to read the Voltage on OBD Pin 1*/
    SELFTEST					            = ((unsigned long)0x10002), /**< To direct the pass-thru device to do some hardware test operations*/
    CAN_SET_BTR					            = ((unsigned long)0x10003), /**< To direct the pass-thru device to select the can parameters by the BTR register of the CAN-Chip*/
    CAN_MULTIPLEXER_CONTROL		            = ((unsigned long)0x10004), /**< To direct the pass-thru device to select the can physical layer*/
    GET_HARDWARE_INFO			            = ((unsigned long)0x10006), /**< To Show Hardwareinformation (serialnumber...)*/
    GET_TRACE_INFO							= ((unsigned long)0x10007), /**< Trace information from firmware*/
    READ_CAN_ERR                            = ((unsigned long)0x10008), /**< Read CAN Error Information*/
    SET_CAN_MODE                            = ((unsigned long)0x10009) /**< Set special CAN Modes e.g. Listen only...*/

};

enum ConfigParamId
{
    DATA_RATE = 0x01,
    LOOPBACK = 0x03,
    NODE_ADDRESS = 0x04,
    NETWORK_LINE = 0x05,
    P1_MIN = 0x06,                      // Don't use
    P1_MAX = 0x07,
    P2_MIN = 0x08,                      // Don't use
    P2_MAX = 0x09,                      // Don't use
    P3_MIN = 0x0A,
    P3_MAX = 0x0B,                      // Don't use
    P4_MIN = 0x0C,
    P4_MAX = 0x0D,                      // Don't use

    W1 = 0x0E,
    W2 = 0x0F,
    W3 = 0x10,
    W4 = 0x11,               // W4_MIN
    W5 = 0x12,
    TIDLE = 0x13,
    TINIL = 0x14,
    TWUP = 0x15,
    PARITY = 0x16,
    BIT_SAMPLE_POINT = 0x17,
    SYNC_JUMP_WIDTH = 0x18,
    W0 = 0x19,
    T1_MAX = 0x1A,
    T2_MAX = 0x1B,
    T4_MAX = 0x1C,

    T5_MAX = 0x1D,
    ISO15765_BS = 0x1E,
    ISO15765_STMIN = 0x1F,
    DATA_BITS = 0x20,
    FIVE_BAUD_MOD = 0x21,
    BS_TX = 0x22,
    STMIN_TX = 0x23,
    T3_MAX = 0x24,
    ISO15765_WFT_MAX = 0x25,

    W1_MIN	=	0x26, // don't use
    W2_MIN	=	0x27, // don't use
    W3_MIN	=   0x28, // don't use
    W4_MAX	=   0x29, //

    N_BR_MIN = 0x2A,
    ISO15765_PAD_VALUE = 0x2B,
    N_AS_MAX = 0x2C,
    N_AR_MAX = 0x2D,
    N_BS_MAX = 0x2E,
    N_CR_MAX = 0x2F,
    N_CS_MIN = 0x30,

    // J2534-2
    CAN_MIXED_FORMAT = 0x8000,

    J1962_PINS = 0x8001,
    SW_CAN_HS_DATA_RATE = 0x8010,
    SW_CAN_SPEEDCHANGE_ENABLE = 0x8011,
    SW_CAN_RES_SWITCH = 0x8012,
    ACTIVE_CHANNELS = 0x8020,
    SAMPLE_RATE = 0x8021,
    SAMPLES_PER_READING = 0x8022,
    READINGS_PER_MSG = 0x8023,
    AVERAGING_METHOD = 0x8024,
    SAMPLE_RESOLUTION = 0x8025,
    INPUT_RANGE_LOW = 0x8026,
    INPUT_RANGE_HIGH = 0x8027,

    // J2534-2 UART Echo Byte protocol parameters
    UEB_T0_MIN = 0x8028,

    UEB_T1_MAX = 0x8029,
    UEB_T2_MAX = 0x802A,
    UEB_T3_MAX = 0x802B,
    UEB_T4_MIN = 0x802C,
    UEB_T5_MAX = 0x802D,
    UEB_T6_MAX = 0x802E,
    UEB_T7_MIN = 0x802F,
    UEB_T7_MAX = 0x8030,
    UEB_T9_MIN = 0x8031,

    // Pin selection
    J1939_PINS = 0x803D,

    J1708_PINS = 0x803E,

    // J2534-2 J1939 config parameters
    J1939_T1 = 0x803F,
    J1939_T2 = 0x8040,
    J1939_T3 = 0x8041,
    J1939_T4 = 0x8042,
    J1939_BRDCST_MIN_DELAY = 0x8043,

    // J2534-2 TP2.0
    TP2_0_T_BR_INT = 0x8044,
    TP2_0_T_E = 0x8045,
    TP2_0_MNTC = 0x8046,
    TP2_0_T_CTA = 0x8047,
    TP2_0_MNCT = 0x8048,
    TP2_0_MNTB = 0x8049,
    TP2_0_MNT = 0x804A,
    TP2_0_T_WAIT = 0x804B,
    TP2_0_T1 = 0x804C,
    TP2_0_T3 = 0x804D,
    TP2_0_IDENTIFER = 0x804E,
    TP2_0_RXIDPASSIVE = 0x804F,
    TP1_6_T_E = 0x8051,
    TP1_6_MNTC = 0x8052,
    TP1_6_MNT = 0x8053,
    TP1_6_T1 = 0x8054,
    TP1_6_T2 = 0x8055,
    TP1_6_T3 = 0x8056,
    TP1_6_T4 = 0x8057,
    TP1_6_IDENTIFER = 0x8058,
    TP1_6_RXIDPASSIVE = 0x8059,
    T2_MIN = 0x805A,
    TP2_0_ACK_DELAY = 0x805B,

    FD_CAN_DATA_PHASE_RATE = 0x805C,
    FD_ISO15765_TX_DATA_LENGTH = 0x805D,
    HS_CAN_TERMINATION = 0x805E,
    FD_CAN_N_CR_MAX = 0x805F,
    FD_ISO15765_PAD_VALUE = 0x8060,

    ETH_LINK_SET_STATE = 0x8061,
    ETH_LINK_STATUS = 0x8062,
    ETH_NEGOTIATE_SET_MANUAL = 0x8063,
    ETH_NEGOTIATE_STATUS = 0x8064,
    ETH_MDIX_SET_MANUAL = 0x8065,
    ETH_MDIX_SET_SWAPPED = 0x8066,
    ETH_MDIX_STATUS = 0x8067,
    ETH_ALL_FRAMES = 0x8068,
    ETH_DUPLEX_SET_HALF = 0x8069,
    ETH_DUPLEX_STATUS = 0x806A,

    // J2534-2 Device Config parameters
    NON_VOLATILE_STORE_1 = 0xC001, /* use SCONFIG_LIST */

    NON_VOLATILE_STORE_2 = 0xC002,
    NON_VOLATILE_STORE_3 = 0xC003,
    NON_VOLATILE_STORE_4 = 0xC004,
    NON_VOLATILE_STORE_5 = 0xC005,
    NON_VOLATILE_STORE_6 = 0xC006,
    NON_VOLATILE_STORE_7 = 0xC007,
    NON_VOLATILE_STORE_8 = 0xC008,
    NON_VOLATILE_STORE_9 = 0xC009,
    NON_VOLATILE_STORE_10 = 0xC00A,

    DT_ISO_INIT_BAUD      = 0x10000008, // Mongoose Pro GMII, Omega Mega-K (KW82)
    DT_ISO_CHECKSUM_TYPE  = 0x10000009 // Mongoose Pro GMII, Omega Mega-K (KW82)
};

enum SParamParameters
{
    // J2534-2 GET_DEVICE_INFO defines
    SERIAL_NUMBER = 0x00000001,
    J1850PWM_SUPPORTED = 0x00000002,
    J1850VPW_SUPPORTED = 0x00000003,
    ISO9141_SUPPORTED = 0x00000004,
    ISO14230_SUPPORTED = 0x00000005,
    CAN_SUPPORTED = 0x00000006,
    ISO15765_SUPPORTED = 0x00000007,
    SCI_A_ENGINE_SUPPORTED = 0x00000008,
    SCI_A_TRANS_SUPPORTED = 0x00000009,
    SCI_B_ENGINE_SUPPORTED = 0x0000000A,
    SCI_B_TRANS_SUPPORTED = 0x0000000B,
    SW_ISO15765_SUPPORTED = 0x0000000C,
    SW_CAN_SUPPORTED = 0x0000000D,
    GM_UART_SUPPORTED = 0x0000000E,
    UART_ECHO_BYTE_SUPPORTED = 0x0000000F,
    HONDA_DIAGH_SUPPORTED = 0x00000010,
    J1939_SUPPORTED = 0x00000011,
    J1708_SUPPORTED = 0x00000012,
    TP2_0_SUPPORTED = 0x00000013,
    J2610_SUPPORTED = 0x00000014,
    ANALOG_IN_SUPPORTED = 0x00000015,
    MAX_NON_VOLATILE_STORAGE = 0x00000016,
    SHORT_TO_GND_J1962 = 0x00000017,
    PGM_VOLTAGE_J1962 = 0x00000018,
    J1850PWM_PS_J1962 = 0x00000019,
    J1850VPW_PS_J1962 = 0x0000001A,
    ISO9141_PS_K_LINE_J1962 = 0x0000001B,
    ISO9141_PS_L_LINE_J1962 = 0x0000001C,
    ISO14230_PS_K_LINE_J1962 = 0x0000001D,
    ISO14230_PS_L_LINE_J1962 = 0x0000001E,
    CAN_PS_J1962 = 0x0000001F,
    ISO15765_PS_J1962 = 0x00000020,
    SW_CAN_PS_J1962 = 0x00000021,
    SW_ISO15765_PS_J1962 = 0x00000022,
    GM_UART_PS_J1962 = 0x00000023,
    UART_ECHO_BYTE_PS_J1962 = 0x00000024,
    HONDA_DIAGH_PS_J1962 = 0x00000025,
    J1939_PS_J1962 = 0x00000026,
    J1708_PS_J1962 = 0x00000027,
    TP2_0_PS_J1962 = 0x00000028,
    J2610_PS_J1962 = 0x00000029,
    J1939_PS_J1939 = 0x0000002A,
    J1708_PS_J1939 = 0x0000002B,
    ISO9141_PS_K_LINE_J1939 = 0x0000002C,
    ISO9141_PS_L_LINE_J1939 = 0x0000002D,
    ISO14230_PS_K_LINE_J1939 = 0x0000002E,
    ISO14230_PS_L_LINE_J1939 = 0x0000002F,
    J1708_PS_J1708 = 0x00000030,
    FT_CAN_SUPPORTED = 0x00000031,
    FT_ISO15765_SUPPORTED = 0x00000032,
    FT_CAN_PS_J1962 = 0x00000033,
    FT_ISO15765_PS_J1962 = 0x00000034,
    J1850PWM_SIMULTANEOUS = 0x00000035,
    J1850VPW_SIMULTANEOUS = 0x00000036,
    ISO9141_SIMULTANEOUS = 0x00000037,
    ISO14230_SIMULTANEOUS = 0x00000038,
    CAN_SIMULTANEOUS = 0x00000039,
    ISO15765_SIMULTANEOUS = 0x0000003A,
    SCI_A_ENGINE_SIMULTANEOUS = 0x0000003B,
    SCI_A_TRANS_SIMULTANEOUS = 0x0000003C,
    SCI_B_ENGINE_SIMULTANEOUS = 0x0000003D,
    SCI_B_TRANS_SIMULTANEOUS = 0x0000003E,
    SW_ISO15765_SIMULTANEOUS = 0x0000003F,
    SW_CAN_SIMULTANEOUS = 0x00000040,
    GM_UART_SIMULTANEOUS = 0x00000041,
    UART_ECHO_BYTE_SIMULTANEOUS = 0x00000042,
    HONDA_DIAGH_SIMULTANEOUS = 0x00000043,
    J1939_SIMULTANEOUS = 0x00000044,
    J1708_SIMULTANEOUS = 0x00000045,
    TP2_0_SIMULTANEOUS = 0x00000046,
    J2610_SIMULTANEOUS = 0x00000047,
    ANALOG_IN_SIMULTANEOUS = 0x00000048,
    PART_NUMBER = 0x00000049,
    FT_CAN_SIMULTANEOUS = 0x004A,
    FT_ISO15765_SIMULTANEOUS = 0x004B,
    FD_CAN_SUPPORTED = 0x004C,
    FD_ISO15765_SUPPORTED = 0x004D,
    FD_CAN_SIMULTANEOUS = 0x004E,
    FD_ISO15765_SIMULTANEOUS = 0x004F,
    FD_CAN_PS_J1962 = 0x0050,
    FD_ISO15765_PS_J1962 = 0x0051,
    CONNECT_MEDIA = 0x0100,
    KW82_SUPPORTED = 0x0101,
    KW82_SIMULTANEOUS = 0x0102,
    KW82_PS_J1962 = 0x0103,

    // J2534-2 GET_PROTOCOL_INFO defines
    MAX_RX_BUFFER_SIZE      /*-2*/	    = 0x0001,
    MAX_PASS_FILTER         /*-2*/	    = 0x0002,
    MAX_BLOCK_FILTER        /*-2*/	    = 0x0003,
    MAX_FILTER_MSG_LENGTH   /*-2*/	    = 0x0004,
    MAX_PERIODIC_MSGS       /*-2*/	    = 0x0005,
    MAX_PERIODIC_MSG_LENGTH /*-2*/	    = 0x0006,
    DESIRED_DATA_RATE       /*-2*/	    = 0x0007,
    MAX_REPEAT_MESSAGING    /*-2*/	    = 0x0008,
    MAX_REPEAT_MESSAGING_LENGTH     /*-2*/	    = 0x0009,
    NETWORK_LINE_SUPPORTED  /*-2*/	    = 0x000A,
    MAX_FUNCT_MSG_LOOKUP    /*-2*/	    = 0x000B,
    PARITY_SUPPORTED        /*-2*/	    = 0x000C,
    DATA_BITS_SUPPORTED     /*-2*/	    = 0x000D,
    FIVE_BAUD_MOD_SUPPORTED /*-2*/	    = 0x000E,
    L_LINE_SUPPORTED        /*-2*/	    = 0x000F,
    CAN_11_29_IDS_SUPPORTED /*-2*/	    = 0x0010,
    CAN_MIXED_FORMAT_SUPPORTED     /*-2*/	    = 0x0011,
    MAX_FLOW_CONTROL_FILTER /*-2*/	    = 0x0012,
    MAX_ISO15765_WFT_MAX    /*-2*/	    = 0x0013,
    MAX_AD_ACTIVE_CHANNELS  /*-2*/	    = 0x0014,
    MAX_AD_SAMPLE_RATE      /*-2*/	    = 0x0015,
    MAX_AD_SAMPLES_PER_READING     /*-2*/	    = 0x0016,
    AD_SAMPLE_RESOLUTION    /*-2*/	    = 0x0017,
    AD_INPUT_RANGE_LOW      /*-2*/	    = 0x0018,
    AD_INPUT_RANGE_HIGH     /*-2*/	    = 0x0019,
    RESOURCE_GROUP          /*-2*/	    = 0x001A,
    TIMESTAMP_RESOLUTION    /*-2*/	    = 0x001B,
    DESIRED_DATA_PHASE_DATA_RATE    /*-2*/	    = 0x001C
};

enum HardwareInformationRequests {
    GET_HW_SERIALNUMBER   = 0,
    GET_HW_NAME			  = 1,
    GET_HW_MANUFACTURER   = 2,
    GET_HW_PRODUCTIONDATE = 3,
    GET_HW_USEDINTERFACE  = 4
};

enum CANPhysicalLayerTypes {
    CAN_82C251_A        	= 0x00000000, /**< High Speed CAN with 82C251*/
    CAN_82C251_B        	= 0x00000001, /**< High Speed CAN with 82C251 (optional second tranceiver)*/
    CAN_TJA1054          	= 0x00000002, /**< Low  Speed CAN with TJA1054*/
    CAN_TLE6255_LS         	= 0x00000003, /**< Single Wire CAN with TLE6255*/
    CAN_TLE6255_HS         	= 0x00000004 /**< Single Wire CAN with TLE6255*/
};

// Values for <DeviceAvailable>
#define DEVICE_STATE_UNKNOWN  0x00
#define DEVICE_AVAILABLE      0x01
#define DEVICE_IN_USE         0x02

// Values for <DeviceDLLFWStatus>
#define DEVICE_DLL_FW_COMPATIBILTY_UNKNOWN  0x00000000
#define DEVICE_DLL_FW_COMPATIBLE            0x00000001
#define DEVICE_DLL_OR_FW_NOT_COMPATIBLE     0x00000002
#define DEVICE_DLL_NOT_COMPATIBLE           0x00000003
#define DEVICE_FW_NOT_COMPATIBLE            0x00000004

// Values for <DeviceConnectedMedia>
#define DEVICE_CONN_UNKNOWN  0x00000000
#define DEVICE_CONN_WIRELESS 0x00000001
#define DEVICE_CONN_WIRED    0x00000002

#define USE_DEFAULT_PASS_FILTER     1ul<<24ul // tool manufacturer specific bit to use the filters as defined in J2534 (all to pass as default)
#define NO_INDICATIONS			    1ul<<25ul // tool manufacturer specific bit to supress the indications from ISO9141/14230/15765
#define CAN_SINGLE_TX			    1ul<<26ul // tool manufacturer specific bit to use can single try tx
#define CAN_LISTEN_ONLY			    1ul<<27ul // tool manufacturer specific bit to use can listen only mode
#define NO_OBD_PIN					1ul<<28ul // tool manufacturer specific bit to suppress conection to OBD pin

/**************************/
/* ProtocolID definitions */
/**************************/
/*
#define J1850VPW          1
#define J1850PWM          2
#define ISO9141           3
#define ISO14230          4
#define CAN             5
#define ISO15765          6
#define SCI_A_ENGINE        7
#define SCI_A_TRANS         8
#define SCI_B_ENGINE        9
#define SCI_B_TRANS         10

#define CAN_PS              0x8004
#define FD_CAN_PS           0x8011
#define ISO15765_LOGICAL    0x200

// Ioctls
#define   GET_CONFIG        0x01  // SCONFIG_LIST   NULL
#define   SET_CONFIG        0x02  // SCONFIG_LIST   NULL
#define   READ_VBATT        0x03  // NULL     unsigned long
#define   FIVE_BAUD_INIT        0x04  // SBYTE_ARRAY    SBYTE_ARRAY
#define   FAST_INIT       0x05  // PASSTHRU_MSG   PASSTHRU_MSG
#define   CLEAR_TX_BUFFER       0x07  // NULL     NULL
#define   CLEAR_RX_BUFFER       0x08  // NULL     NULL
#define   CLEAR_PERIODIC_MSGS     0x09  // NULL     NULL
#define   CLEAR_MSG_FILTERS     0x0A  // NULL     NULL
#define   CLEAR_FUNCT_MSG_LOOKUP_TABLE    0x0B  // NULL     NULL
#define   ADD_TO_FUNCT_MSG_LOOKUP_TABLE   0x0C  // SBYTE_ARRAY    NULL
#define   DELETE_FROM_FUNCT_MSG_LOOKUP_TABLE  0x0D  // SBYTE_ARRAY    NULL
#define   READ_PROG_VOLTAGE     0x0E  // NULL     unsigned long
*/
//
// J2534-1 v04.04 RxStatus Definitions
//
/*
#define TX_MSG_TYPE         0x0001
#define START_OF_MESSAGE      0x0002
#define RX_BREAK          0x0004
#define TX_INDICATION       0x0008
#define ISO15765_PADDING_ERROR    0x0010
#define ISO15765_ADDR_TYPE      0x0080
//#define CAN_29BIT_ID        0x0100    // Defined above
*/
/*************/
/* Error IDs */
/*************/

enum J2534Err
{
    STATUS_NOERROR              = ((long)0x00),    /**< Function call successful */
    ERR_NOT_SUPPORTED           = ((long)0x01),    /**< Function not supported */
    ERR_INVALID_CHANNEL_ID      = ((long)0x02),    /**< Invalid ChannelID value */
    ERR_INVALID_PROTOCOL_ID     = ((long)0x03),    /**< Invalid ProtocolID value */
    ERR_NULL_PARAMETER          = ((long)0x04),    /**< NULL pointer supplied where a valid pointer is required */
    ERR_INVALID_IOCTL_VALUE     = ((long)0x05),    /**< Invalid Ioctl Parameter Value */
    ERR_INVALID_FLAGS           = ((long)0x06),    /**< Invalid flag values */
    ERR_FAILED                  = ((long)0x07),    /**< Undefined error, use PassThruGetLastError for description of error. */
    ERR_DEVICE_NOT_CONNECTED    = ((long)0x08),    /**< unable to communicate with device*/
    ERR_TIMEOUT                 = ((long)0x09),    /**< Timeout. No message available to read or could not read the specified number of messages. The actual number of messages read is placed in <NumMsgs> */
    ERR_INVALID_MSG             = ((long)0x0A),    /**< Invalid message structure pointed to by pMsg (Reference Section 7 Message Structure), */
    ERR_INVALID_TIME_INTERVAL   = ((long)0x0B),    /**< Invalid TimeInterval value */
    ERR_EXCEEDED_LIMIT          = ((long)0x0C),    /**< Exceeded maximum number of message IDs or allocated space */
    ERR_INVALID_MSG_ID          = ((long)0x0D),    /**< Invalid MsgID value */
    ERR_DEVICE_IN_USE           = ((long)0x0E),    /**< Device is already in use */
    ERR_INVALID_IOCTL_ID        = ((long)0x0F),    /**< Invalid IoctlID value */
    ERR_BUFFER_EMPTY            = ((long)0x10),    /**< Protocol message buffer empty */
    ERR_BUFFER_FULL             = ((long)0x11),    /**< Protocol message buffer full */
    ERR_BUFFER_OVERFLOW         = ((long)0x12),    /**< Protocol message buffer overflow */
    ERR_PIN_INVALID             = ((long)0x13),    /**< Invalid pin number */
    ERR_CHANNEL_IN_USE          = ((long)0x14),    /**< Channel already in use */
    ERR_MSG_PROTOCOL_ID         = ((long)0x15),    /**< Protocol type does not match the protocol associated with the Channel ID */

    ERR_INVALID_FILTER_ID       = ((long)0x16),    /**< Invalid FilterID value */
    ERR_NO_FLOW_CONTROL         = ((long)0x17),    /**< No matching flow control defined (ISO15765 only)*/
    ERR_NOT_UNIQUE              = ((long)0x18),    /**< Filter pattern or flowcontrol message matches an already defined*/
    ERR_INVALID_BAUDRATE        = ((long)0x19),    /**< Baudrate cannot be achived with the specified tolerance*/
    ERR_INVALID_DEVICE_ID       = ((long)0x1A),    /**< Invalid Device ID value */
    //Unused                            0x1B - 0xFFFF			Reserved for SAE 2534-1
    //Unused							0x10000 - 0xFFFFFFFF	Reserved for SAE 2534-2


    ERR_INVALID_IOCTL_PARAM_ID /*-2*/	= 0x1E,
    ERR_VOLTAGE_IN_USE		/*-2*/	    = 0x1F,
    ERR_PIN_IN_USE          /*-2*/      = 0x20,

    ERR_ADDRESS_NOT_CLAIMED /*-2*/      = 0x10000,
    ERR_NO_CONNECTION_ESTABLISHED/*-2*/ = 0x10001,
    ERR_RESOURCE_IN_USE     /*-2*/      = 0x10002,
    ERR_LINK_DOWN           /*-2*/      = 0x10002,
    ERR_INCORRECT_STATE     /*-2*/      = 0x10003,

    ERR_NULLPARAMETER       /*v2*/      = ERR_NULL_PARAMETER
};
/*
// Function call successful
#define STATUS_NOERROR        0x00

// Device cannot support requested functionality mandated in this
// document. Device is not fully SAE J2534 compliant
#define ERR_NOT_SUPPORTED     0x01

// Invalid ChannelID value
#define ERR_INVALID_CHANNEL_ID    0x02

// Invalid ProtocolID value, unsupported ProtocolID, or there is a resource conflict (i.e. trying to connect to
// multiple protocols that are mutually exclusive such as J1850PWM and J1850VPW, or CAN and SCI A, etc.)
#define ERR_INVALID_PROTOCOL_ID   0x03

// NULL pointer supplied where a valid pointer is required
#define ERR_NULL_PARAMETER      0x04

// Invalid value for Ioctl parameter
#define ERR_INVALID_IOCTL_VALUE   0x05

// Invalid flag values
#define ERR_INVALID_FLAGS     0x06

// Undefined error, use PassThruGetLastError for text description
#define ERR_FAILED          0x07

// Device ID invalid
#define ERR_DEVICE_NOT_CONNECTED  0x08

// Timeout.
// PassThruReadMsg: No message available to read or could not read the specified number of
//   messages. The actual number of messages read is placed in <NumMsgs>
// PassThruWriteMsg: Device could not write the specified number of messages. The actual number of
//   messages sent on the vehicle network is placed in <NumMsgs>.
#define ERR_TIMEOUT         0x09

// Invalid message structure pointed to by pMsg (Reference Section 8 � Message Structure)
#define ERR_INVALID_MSG       0x0A

// Invalid TimeInterval value
#define ERR_INVALID_TIME_INTERVAL 0x0B

// Exceeded maximum number of message IDs or allocated space
#define ERR_EXCEEDED_LIMIT      0x0C

// Invalid MsgID value
#define ERR_INVALID_MSG_ID      0x0D

// Device is currently open
#define ERR_DEVICE_IN_USE     0x0E

// Invalid IoctlID value
#define ERR_INVALID_IOCTL_ID    0x0F

// Protocol message buffer empty, no messages available to read
#define ERR_BUFFER_EMPTY      0x10

// Protocol message buffer full. All the messages specified may not have been transmitted
#define ERR_BUFFER_FULL       0x11

// Indicates a buffer overflow occurred and messages were lost
#define ERR_BUFFER_OVERFLOW     0x12

// Invalid pin number, pin number already in use, or voltage already applied to a different pin
#define ERR_PIN_INVALID       0x13

// Channel number is currently connected
#define ERR_CHANNEL_IN_USE      0x14

// Protocol type in the message does not match the protocol associated with the Channel ID
#define ERR_MSG_PROTOCOL_ID     0x15

// Invalid Filter ID value
#define ERR_INVALID_FILTER_ID   0x16

// No flow control filter set or matched (for protocolID ISO15765 only)
#define ERR_NO_FLOW_CONTROL     0x17

// A CAN ID in pPatternMsg or pFlowControlMsg matches either ID in an existing FLOW_CONTROL_FILTER
#define ERR_NOT_UNIQUE        0x18

// The desired baud rate cannot be achieved within the tolerance specified in Section 6.5
#define ERR_INVALID_BAUDRATE    0x19

// Unable to communicate with device
#define ERR_INVALID_DEVICE_ID   0x1A

#define ERR_NULLPARAMETER     ERR_NULL_PARAMETER  //v2
*/
#endif // J2534_DEFS_H
