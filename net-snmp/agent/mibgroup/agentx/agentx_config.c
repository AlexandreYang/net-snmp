/*
 *  AgentX Configuration
 */
#include <net-snmp/net-snmp-config.h>

#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include <net-snmp/asn1.h>
#include "snmpd.h"
#include <net-snmp/snmp_debug.h>
#include <net-snmp/default_store.h>
#include <net-snmp/agent/ds_agent.h>
#include <net-snmp/snmp_logging.h>
#include <net-snmp/agent/mib_module_config.h>
#include <net-snmp/read_config.h>
#include <net-snmp/agent/agent_read_config.h>
#include "agentx/agentx_config.h"

#ifdef USING_AGENTX_MASTER_MODULE
void agentx_parse_master(const char *token, char *cptr)
{
    int i = -1;
    char buf[BUFSIZ];

    if ( !strcmp( cptr, "agentx" ) ||
         !strcmp( cptr, "all"    ) ||
         !strcmp( cptr, "yes"    ) ||
         !strcmp( cptr, "on"     )) {
                i = 1;
         snmp_log(LOG_INFO,
                "Turning on AgentX master support.\n");
         snmp_log(LOG_INFO,
                "Note this is still experimental and shouldn't be used on critical systems.\n");
    }
    else if ( !strcmp( cptr, "no"  ) ||
              !strcmp( cptr, "off" ))
                i = 0;
    else
                i = atoi(cptr);

    if (i < 0 || i > 1) {
        sprintf(buf, "master '%s' unrecognised", cptr);
        config_perror( buf );
    }
    else
        ds_set_boolean(DS_APPLICATION_ID, DS_AGENT_AGENTX_MASTER, i );
}
#endif /* USING_AGENTX_MASTER_MODULE */

void agentx_parse_agentx_socket(const char *token, char *cptr)
{
    DEBUGMSGTL(("agentx/config", "port spec: %s\n", cptr));
    ds_set_string(DS_APPLICATION_ID, DS_AGENT_X_SOCKET, cptr);
}

void init_agentx_config(void)
{
    /*
     * Don't set this up as part of the per-module initialisation.
     * Delay this until the 'init_master_agent()' routine is called,
     *   so that the config settings have been processed.
     * This means that we can use a config directive to determine
     *   whether or not to run as an AgentX master.
     */
#ifdef USING_AGENTX_MASTER_MODULE
    if(ds_get_boolean(DS_APPLICATION_ID, DS_AGENT_ROLE) == MASTER_AGENT)
        snmpd_register_config_handler("master",
                                      agentx_parse_master, NULL,
                                      "specify 'agentx' for AgentX support");
#endif /* USING_AGENTX_MASTER_MODULE */
    snmpd_register_config_handler("agentxsocket",
                        agentx_parse_agentx_socket, NULL,
                        "AgentX bind address");
}


