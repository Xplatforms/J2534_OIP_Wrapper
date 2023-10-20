#ifndef J2534_DEV_TYPEDEFS_H
#define J2534_DEV_TYPEDEFS_H

#include "j2534.h"

typedef long (CALLBACK* PTOPEN) (const void*, unsigned long*);
typedef long ( CALLBACK * PTCLOSE ) ( unsigned long );

//#define J2534ERROR long

typedef long ( CALLBACK * PTCONNECT ) ( unsigned long, unsigned long, unsigned long, unsigned long, unsigned long * );
typedef long ( CALLBACK * PTDISCONNECT ) ( unsigned long );
typedef long ( CALLBACK * PTREADMSGS ) ( unsigned long, void *, unsigned long *, unsigned long );
typedef long ( CALLBACK * PTWRITEMSGS ) ( unsigned long, const void *, unsigned long *, unsigned long );
typedef long ( CALLBACK * PTSTARTPERIODICMSG ) ( unsigned long, void *, unsigned long *, unsigned long );
typedef long ( CALLBACK * PTSTOPPERIODICMSG ) ( unsigned long, unsigned long );
typedef long ( CALLBACK * PTSTARTMSGFILTER ) ( unsigned long, unsigned long, void *, void *, void *, unsigned long * );
typedef long ( CALLBACK * PTSTOPMSGFILTER ) ( unsigned long, unsigned long );
typedef long ( CALLBACK * PTSETPROGRAMMINGVOLTAGE ) ( unsigned long, unsigned long, unsigned long );
typedef long ( CALLBACK * PTREADVERSION ) ( unsigned long, char *, char *, char * );
typedef long ( CALLBACK * PTGETLASTERROR ) ( char * );
typedef long ( CALLBACK * PTIOCTL ) ( unsigned long, unsigned long, const void *, void * );
// Drew Tech specific function calls
typedef long ( CALLBACK * PTLOADFIRMWARE ) ( void );
typedef long ( CALLBACK * PTRECOVERFIRMWARE ) ( void );
typedef long ( CALLBACK * PTREADIPSETUP ) ( unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr );
typedef long ( CALLBACK * PTWRITEIPSETUP ) ( unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr );
typedef long ( CALLBACK * PTREADPCSETUP ) ( char *host_name, char *ip_addr );
typedef long ( CALLBACK * PTGETPOINTER ) ( long vb_pointer );
typedef long ( CALLBACK * PTGETNEXTCARDAQ ) ( char **name, unsigned long * version, char **addr );

typedef long ( CALLBACK * PTEXCONFIGWIFI ) ( void );
typedef long ( CALLBACK * PTEXDEVWATCHDOG ) ( void );
typedef long ( CALLBACK * PTEXDLCYPHFLASHDATA ) ( void );
typedef long ( CALLBACK * PTEXERASEFLASH ) ( void );
typedef long ( CALLBACK * PTEXINITCYPHFLASHDL ) ( void );
typedef long ( CALLBACK * PTEXREADFLASH ) ( void );
typedef long ( CALLBACK * PTEXRESETFLASH ) ( void );
typedef long ( CALLBACK * PTEXRINSELFTEST ) ( void );
typedef long ( CALLBACK * PTEXWRITEFLASH ) ( void );

//v05.00
typedef long ( CALLBACK * PTSCANFORDEVICES ) ( unsigned long *pDeviceCount );
typedef long ( CALLBACK * PTGETNEXTDEV ) ( SDEVICE *psDevice );
typedef long ( CALLBACK * PTSELECT ) ( SCHANNELSET *ChannelSetPtr, unsigned long SelectType, unsigned long Timeout );
typedef long ( CALLBACK * PTREADDETAILS )(unsigned long* pName);

typedef long (CALLBACK * PTQUEUEMSGS) (unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs);

#endif // J2534_DEV_TYPEDEFS_H
