#ifndef NET_SNMP_TYPES_H
#define NET_SNMP_TYPES_H

    /**
     *  Definitions of data structures, used within the library API.
     */

#include <net-snmp/net-snmp-config.h>
#include <sys/types.h>

#ifndef MAX_SUBID	/* temporary - duplicate definition protection */
#ifndef EIGHTBIT_SUBIDS
typedef u_long	oid;
#define MAX_SUBID   0xFFFFFFFF
#else
typedef u_char	oid;
#define MAX_SUBID   0xFF
#endif
#endif


    /*
     *  For the initial release, this will just refer to the
     *  relevant UCD header files.
     *    In due course, the types and structures relevant to the
     *  Net-SNMP API will be identified, and defined here directly.
     *
     *  But for the time being, this header file is primarily a placeholder,
     *  to allow application writers to adopt the new header file names.
     */

#include <net-snmp/definitions.h>
#include <net-snmp/library/snmp_api.h>
/* #include <net-snmp/library/libsnmp.h> */

#endif /* NET_SNMP_TYPES_H */
