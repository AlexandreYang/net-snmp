/* include file for event module */

#ifndef _MIBGROUP_EVENT_H
#define _MIBGROUP_EVENT_H

struct variable;
struct timeval;

extern int eventFreeSpace (void);
extern int eventNotifyFreeSpace (void);
extern FindVarMethod var_eventnextindex;
extern FindVarMethod var_eventtab;
extern FindVarMethod var_eventnotifyvars;
extern FindVarMethod var_eventnotifytab;
extern void eventGenerate (int, int, void *);
extern void eventTimer (struct timeval *);

#define MAX_COMMUNITY_LEN 128

struct eventEntry {
	struct eventEntry *next;
	int index;				/* 1..65535 */
	oid id[MAX_OID_LEN];
	int idLen;
	char description[MAX_DESCRIPTION_LEN];		/* 0..127 */
	int descriptionLen;
	int numEvents;
	u_long lastTimeSent;	/* timeticks */
	u_long minInterval;
	u_long maxRetransmissions;
	int status;
	struct eventEntry *shadow;	/* copy for row creates and changes */
	u_long bitmask;			/* mask of valid variables */
};

struct eventNotifyEntry {
    struct eventNotifyEntry *next;
    int index;
    oid srcParty[MAX_OID_LEN];
    int srcPartyLen;
    oid dstParty[MAX_OID_LEN];
    int dstPartyLen;
    oid context[MAX_OID_LEN];
    int contextLen;
    int interval;
    int retransmissions;
    int lifetime;
    int status;
    struct snmp_session *ss;
    struct get_req_state *magic;
    struct eventNotifyEntry *shadow;
    u_long bitmask;
};


/* defines passed to eventGenerate() to tell it what type of event occured */
#define EVENT_TYPE_STARTUP_RISING 1
#define EVENT_TYPE_STARTUP_FALLING 2
#define EVENT_TYPE_RISING 3
#define EVENT_TYPE_FALLING 4
#define EVENT_TYPE_UNAVAILABLE 5

/* does this mean anything? */
#define TRAP_RISING_ALARM 1
#define TRAP_FALLING_ALARM 2
#define TRAP_UNAVAILABLE_ALARM 3

/* masks for the bitmask field in struct eventEntry */
#define EVENTTABINDEXMASK			0x00000001
#define EVENTTABIDMASK				0x00000002
#define EVENTTABDESCRIPTIONMASK			0x00000004
#define EVENTTABEVENTSMASK			0x00000008
#define EVENTTABLASTTIMESENTMASK		0x00000010
#define EVENTTABSTATUSMASK			0x00000020

#define EVENTTABCOMPLETEMASK			0x0000003F

#define EVENTNOTIFYTABINTERVALMASK	    	0x00000001
#define EVENTNOTIFYTABRETRANSMISSIONSMASK 	0x00000002
#define EVENTNOTIFYTABLIFETIMEMASK	    	0x00000004
#define EVENTNOTIFYTABSTATUSMASK	    	0x00000008

#define EVENTNOTIFYTABCOMPLETEMASK		0x0000000F

#define SNMPV2EVENTNEXTINDEX	SNMPV2M2M, 1, 2, 1
#define SNMPV2EVENTENTRY	SNMPV2M2M, 1, 2, 2, 1
#define SNMPV2EVENTNOTIFYMININT	SNMPV2M2M, 1, 2, 3
#define SNMPV2EVENTNOTIFYMAXRET	SNMPV2M2M, 1, 2, 4
#define SNMPV2EVENTNOTIFYENTRY	SNMPV2M2M, 1, 2, 5, 1

#ifdef IN_SNMP_VARS_C

struct variable2 eventnextindex_variables[] = {
    {EVENTNEXTINDEX, ASN_INTEGER, RONLY, var_eventnextindex, 1, {0}}
};

struct variable2 eventtab_variables[] = {
        {EVENTTABID, ASN_OBJECT_ID, RWRITE, var_eventtab, 1, {2 }},
        {EVENTTABDESCRIPTION, ASN_OCTET_STR, RWRITE, var_eventtab, 1, {3 }},
        {EVENTTABEVENTS, ASN_COUNTER, RONLY, var_eventtab, 1, {4 }},
        {EVENTTABLASTTIMESENT, ASN_TIMETICKS, RONLY, var_eventtab, 1, {5 }},
        {EVENTTABSTATUS, ASN_INTEGER, RWRITE, var_eventtab, 1, {6 }}
};

struct variable2 eventmininterval_variables[] = {
    {EVENTMININTERVAL, ASN_INTEGER, RONLY, var_eventnotifyvars, 1, {0}}
};

struct variable2 eventmaxretrans_variables[] = {
    {EVENTMAXRETRANS, ASN_INTEGER, RONLY, var_eventnotifyvars, 1, {0}}
};

struct variable2 eventnotifytab_variables[] = {
        {EVENTNOTIFYTABINTERVAL, ASN_INTEGER, RWRITE, var_eventnotifytab, 1, {1 }},
        {EVENTNOTIFYTABRETRANSMISSIONS, ASN_INTEGER, RWRITE, var_eventnotifytab, 1, {2 }},
        {EVENTNOTIFYTABLIFETIME, ASN_INTEGER, RWRITE, var_eventnotifytab, 1, {3 }},
        {EVENTNOTIFYTABSTATUS, ASN_INTEGER, RWRITE, var_eventnotifytab, 1, {4 }},
};

config_load_mib( SNMPV2EVENTNEXTINDEX, 10, eventnextindex_variables)
config_load_mib( SNMPV2EVENTENTRY, 11, eventtab_variables)
config_load_mib( SNMPV2EVENTNOTIFYMININT, 10, eventmininterval_variables)
config_load_mib( SNMPV2EVENTNOTIFYMAXRET, 10, eventmaxretrans_variables)
config_load_mib( SNMPV2EVENTNOTIFYENTRY, 11, eventnotifytab_variables)

#endif
#endif /* _MIBGROUP_ALARM_H */
