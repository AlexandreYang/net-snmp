/*
 *  Interface MIB architecture support
 *
 * $Id$
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include "mibII/mibII_common.h"

#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/data_access/ipaddress.h>

#include "ip-mib/ipAddressTable/ipAddressTable_constants.h"

#include <errno.h>
#include <sys/ioctl.h>

#if defined (INET6)
#include <linux/types.h>
#include <linux/rtnetlink.h>
#endif

#include "ipaddress_ioctl.h"

int _load_v6(netsnmp_container *container, int idx_offset);

/**
 *
 * @retval  0 no errors
 * @retval !0 errors
 */
int
netsnmp_access_ipaddress_container_arch_load(netsnmp_container *container)
{
    int rc = 0, idx_offset = 0;

    rc = _netsnmp_access_ipaddress_container_ioctl_load_v4(container, idx_offset);
    if(rc < 0) {
        u_int flags = NETSNMP_ACCESS_IPADDRESS_FREE_KEEP_CONTAINER;
        netsnmp_access_ipaddress_container_free(container, flags);
        return rc;
    }

#if defined (INET6)
    idx_offset = rc;

    /*
     * load ipv6, ignoring errors if file not found
     */
    rc = _load_v6(container, idx_offset);
    if (-2 == rc)
        rc = 0;
    else if(rc < 0) {
        u_int flags = NETSNMP_ACCESS_IPADDRESS_FREE_KEEP_CONTAINER;
        netsnmp_access_ipaddress_container_free(container, flags);
    }
#endif

    /*
     * return no errors (0) if we found any interfaces
     */
    if(rc > 0)
        rc = 0;

    return rc;
}

#if defined (INET6)
/**
 */
int
_load_v6(netsnmp_container *container, int idx_offset)
{
    FILE           *in;
    char            line[80], addr[33], if_name[9];
    u_char          *buf;
    int             if_index, pfx_len, scope, flags, rc = 0, in_len, out_len;
    netsnmp_ipaddress_entry *entry;
    static int      log_open_err = 1;
    
    netsnmp_assert(NULL != container);

#define PROCFILE "/proc/net/if_inet6"
    if (!(in = fopen(PROCFILE, "r"))) {
        if (1 == log_open_err) {
            snmp_log(LOG_ERR,"could not open " PROCFILE "\n");
            log_open_err = 0;
        }
        return -2;
    }
    /*
     * if we hadn't been able to open file and turned of err logging,
     * turn it back on now that we opened the file.
     */
    if (0 == log_open_err)
        log_open_err = 1;

    /*
     * address index prefix_len scope status if_name
     */
    while (fgets(line, sizeof(line), in)) {
        /*
         * fe800000000000000200e8fffe5b5c93 05 40 20 80 eth0
         *             A                    D  P  S  F  I
         * A: address
         * D: device number
         * P: prefix len
         * S: scope (see include/net/ipv6.h, net/ipv6/addrconf.c)
         * F: flags (see include/linux/rtnetlink.h, net/ipv6/addrconf.c)
         * I: interface
         */
        rc = sscanf(line, "%32s %02x %02x %02x %02x %8s\n",
                    addr, &if_index, &pfx_len, &scope, &flags, if_name);
        if( 6 != rc ) {
            snmp_log(LOG_ERR, PROCFILE " data format error (%d!=6), line ==|%s|\n",
                     rc, line);
            continue;
        }
        DEBUGMSGTL(("access:ipaddress:container",
                    "addr %s, index %d, pfx %d, scope %d, flags 0x%X, name %s\n",
                    addr, if_index, pfx_len, scope, flags, if_name));
        /*
         */
        entry = netsnmp_access_ipaddress_entry_create();
        if(NULL == entry) {
            rc = -3;
            break;
        }

        in_len = entry->ia_address_len = sizeof(entry->ia_address);
        netsnmp_assert(16 == in_len);
        out_len = 0;
        buf = entry->ia_address;
        if(1 != snmp_hex_to_binary(&buf,
                                   &in_len, &out_len, 0, addr)) {
            snmp_log(LOG_ERR,"error parsing '%s', skipping\n",
                     entry->ia_address);
            netsnmp_access_ipaddress_entry_free(entry);
            continue;
        }
        netsnmp_assert(16 == out_len);
        entry->ia_address_len = out_len;

        entry->ns_ia_index = ++idx_offset;

        entry->ia_flags = flags;

#ifndef NETSNMP_USE_IOCTL_IFINDEX
        /*
         * there is an iotcl to get an ifindex, but I'm not sure that
         * it has the correct characteristics required to be the actual
         * ifIndex for the mib, so we'll use the netsnmp interface method
         * (which is based on the interface name).
         */
        entry->if_index = netsnmp_access_interface_index_find(if_name);
#else
        entry->if_index = if_index;
#endif

        /*
          #define IPADDRESSSTATUSTC_PREFERRED  1
          #define IPADDRESSSTATUSTC_DEPRECATED  2
          #define IPADDRESSSTATUSTC_INVALID  3
          #define IPADDRESSSTATUSTC_INACCESSIBLE  4
          #define IPADDRESSSTATUSTC_UNKNOWN  5
          #define IPADDRESSSTATUSTC_TENTATIVE  6
          #define IPADDRESSSTATUSTC_DUPLICATE  7
        */
        if(flags & IFA_F_PERMANENT)
            entry->ia_status = IPADDRESSSTATUSTC_PREFERRED; /* ?? */
        else if(flags & IFA_F_DEPRECATED)
            entry->ia_status = IPADDRESSSTATUSTC_DEPRECATED;
        else if(flags & IFA_F_TENTATIVE)
            entry->ia_status = IPADDRESSSTATUSTC_TENTATIVE;
        else {
            entry->ia_status = IPADDRESSSTATUSTC_UNKNOWN;
            DEBUGMSGTL(("access:ipaddress:ipv6",
                        "unknown flags 0x%x\n", flags));
        }

        /*
         * if it's not multi, it must be uni.
         *  (an ipv6 address is never broadcast)
         */
        if (IN6_IS_ADDR_MULTICAST(entry->ia_address))
            entry->ia_type = IPADDRESSTYPE_ANYCAST;
        else
            entry->ia_type = IPADDRESSTYPE_UNICAST;


        /** entry->ia_prefix_oid ? */

        /*
         * can we figure out if an address is from DHCP?
         * use manual until then...
         *
         *#define IPADDRESSORIGINTC_OTHER  1
         *#define IPADDRESSORIGINTC_MANUAL  2
         *#define IPADDRESSORIGINTC_DHCP  4
         *#define IPADDRESSORIGINTC_LINKLAYER  5
         *#define IPADDRESSORIGINTC_RANDOM  6
         *
         * are 'local' address assigned by link layer??
         */
        if (IN6_IS_ADDR_LINKLOCAL(entry->ia_address) ||
            IN6_IS_ADDR_SITELOCAL(entry->ia_address))
            entry->ia_origin = IPADDRESSORIGINTC_LINKLAYER;
        else
            entry->ia_origin = IPADDRESSORIGINTC_MANUAL;

        // xxx-rks: what can we do with scope?

        /*
         * add entry to container
         */
        CONTAINER_INSERT(container, entry);
    }

    fclose(in);

    if(rc<0)
        return rc;

    return idx_offset;
}
#endif
