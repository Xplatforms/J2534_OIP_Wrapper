//PassThruSpeacial.h

/************************************************************************************************************\
* Description :  Speccial defines for the IME PassThru                                                       *
**************************************************************************************************************
* Project     :  PassThru API DLL                                                                            *
* File        :  PassThruSpecial.h                                                                           *
* Version     :  1.00                                                                                        *
* Reference   :  SAE J2534 December 2001 Ballot = 01.02                                                      *
**************************************************************************************************************
* Author      :  Joerg Petersen                                                                              *
* Company     :  I+ME Actia GmbH                                                                             *
**************************************************************************************************************
* History     :  14.03.2002         Joerg Petersen      Creation                                             *
*                                                                                                            *
\************************************************************************************************************/
#ifndef _PASSTHRU_SPECIAL_H
#define _PASSTHRU_SPECIAL_H

// protid>0x10000-> does not match in a byte,so we translate it to a valid range
// Prot Id in Firmware
#define SWCAN_FW			((unsigned long)0x0B)
#define SW_ISO15765_FW	    ((unsigned long)0x0C)
#define ISO15765_CH1_FW     ((unsigned long)0x1c) //< translated from 0x0009400 to 0x1c ISO15765 2


#define ISO14230_2I			((unsigned long)29)
#define ISO9141_2I			((unsigned long)30)
#define CAN_CH1_FW			((unsigned long)31)


#define HWR_CAN2  0x0001
#define HWR_CAN1  0x0002
#define HWR_UART1 0x0004
#define HWR_UART2 0x0008
#define HWR_VPW   0x0010
#define HWR_PWM   0x0020


#endif //_PASSTHRU_SPECIAL_H

