/*
 * SNMPv3 View-based Access Control Model
 */

#include <config.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#if HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <ctype.h>
#include <sys/types.h>
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include <netdb.h>

#include "mibincl.h"
#include "read_config.h"
#include "agent_read_config.h"
#include "../../../snmplib/system.h"
#include "vacm.h"
#include "vacm_vars.h"
#include "../../mib_module_config.h"
#ifdef USING_MIBII_SYSORTABLE_MODULE
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include "sysORTable.h"
#endif

#include "vacm.h"

void
init_vacm_vars (void) 
{

#ifdef USING_MIBII_SYSORTABLE_MODULE
  static oid reg[] = {1,3,6,1,6,3,13};
#endif
  
#define PRIVRW	(SNMPV2ANY | 0x5000)

  struct variable2 vacm_sec2group[] = {
    {SECURITYMODEL, ASN_INTEGER, PRIVRW, var_vacm_sec2group, 1, {1}},
    {SECURITYNAME, ASN_OCTET_STR, PRIVRW, var_vacm_sec2group, 1, {2}},
    {SECURITYGROUP, ASN_OCTET_STR, PRIVRW, var_vacm_sec2group, 1, {3}},
    {SECURITYSTORAGE, ASN_INTEGER, PRIVRW, var_vacm_sec2group, 1, {4}},
    {SECURITYSTATUS, ASN_INTEGER, PRIVRW, var_vacm_sec2group, 1, {5}},
  };

  struct variable2 vacm_access[] = {
    {ACCESSPREFIX, ASN_OCTET_STR, PRIVRW, var_vacm_access, 1, {1}},
    {ACCESSMODEL, ASN_INTEGER, PRIVRW, var_vacm_access, 1, {2}},
    {ACCESSLEVEL, ASN_INTEGER, PRIVRW, var_vacm_access, 1, {3}},
    {ACCESSMATCH, ASN_INTEGER, PRIVRW, var_vacm_access, 1, {4}},
    {ACCESSREAD, ASN_OCTET_STR, PRIVRW, var_vacm_access, 1, {5}},
    {ACCESSWRITE, ASN_OCTET_STR, PRIVRW, var_vacm_access, 1, {6}},
    {ACCESSNOTIFY, ASN_OCTET_STR, PRIVRW, var_vacm_access, 1, {7}},
    {ACCESSSTORAGE, ASN_INTEGER, PRIVRW, var_vacm_access, 1, {8}},
    {ACCESSSTATUS, ASN_INTEGER, PRIVRW, var_vacm_access, 1, {9}},
  };

  struct variable2 vacm_view[] = {
    {VIEWNAME, ASN_OCTET_STR, PRIVRW, var_vacm_view, 1, {1}},
    {VIEWSUBTREE, ASN_OBJECT_ID, PRIVRW, var_vacm_view, 1, {2}},
    {VIEWMASK, ASN_OCTET_STR, PRIVRW, var_vacm_view, 1, {3}},
    {VIEWTYPE, ASN_INTEGER, PRIVRW, var_vacm_view, 1, {4}},
    {VIEWSTORAGE, ASN_INTEGER, PRIVRW, var_vacm_view, 1, {5}},
    {VIEWSTATUS, ASN_INTEGER, PRIVRW, var_vacm_view, 1, {6}},
  };

/* Define the OID pointer to the top of the mib tree that we're
   registering underneath */
  oid vacm_sec2group_oid[] = { OID_VACMGROUPENTRY };
  oid vacm_access_oid[] = { OID_VACMACCESSENTRY};
  oid vacm_view_oid[] = { OID_VACMVIEWENTRY };

  /* register ourselves with the agent to handle our mib tree */
  REGISTER_MIB("mibII/vacm:sec2group", vacm_sec2group, variable2, \
               vacm_sec2group_oid);
  REGISTER_MIB("mibII/vacm:access", vacm_access, variable2, vacm_access_oid);
  REGISTER_MIB("mibII/vacm:view", vacm_view, variable2, vacm_view_oid);

  snmpd_register_config_handler("com2sec", vacm_parse_security,
                                vacm_free_security,"name source community");
  snmpd_register_config_handler("group", vacm_parse_group, vacm_free_group,
                                "name v1|v2c|usm|any security");
  snmpd_register_config_handler("access", vacm_parse_access, vacm_free_access,
                            "name context model level prefx read write notify");
  snmpd_register_config_handler("view", vacm_parse_view, vacm_free_view,
                                "name type subtree [mask]");

#ifdef USING_MIBII_SYSORTABLE_MODULE
  register_sysORTable(reg,7,"View-based Access Control Model for SNMP.");
#endif
}

static struct vacm_securityEntry *securityFirst =0, *securityLast =0;

void vacm_parse_security (char *token, 
			  char *param)
{
    char *name, *source, *community, *mask;
    char *cp;
    struct vacm_securityEntry *sp, se;
    int maskLength, maskBit;

    memset (&se, 0 , sizeof se);
    name = strtok(param, "\t\n ");
    if (!name) {
	config_perror("missing NAME parameter");
	return;
    }
    source = strtok(NULL, "\t\n ");
    if (!source) {
	config_perror("missing SOURCE parameter");
	return;
    }
    community = strtok(NULL, "\t\n ");
    if (!community) {
	config_perror("missing COMMUNITY parameter");
	return;
    }
    cp = strchr(source, '/');
    if (cp == NULL) cp = "";
    else *cp++ = 0;
    mask = cp;
    if (strcmp("default", source) == 0 || strcmp("0.0.0.0", source) == 0) {
	memset(&se.sourceIp.sin_addr, 0, sizeof(struct in_addr));
	mask = "0.0.0.0";
    }
    else if ((se.sourceIp.sin_addr.s_addr = inet_addr (source)) == (unsigned) -1) {
	struct hostent *hp = gethostbyname(source);
	if (hp != NULL) {
	    memcpy(&se.sourceIp.sin_addr , hp->h_addr, 4);
	}
	else {
	    config_perror ("bad source address");
	    return;
	}
    }
    if (*mask == 0) memset (&se.sourceMask.sin_addr, 0xff, sizeof(struct in_addr));
    else {
	if (strchr(mask, '.')) {
	    if ((se.sourceMask.sin_addr.s_addr = inet_addr(mask)) == (unsigned)-1) {
		config_perror("bad mask");
		return;
	    }
	}
	else {
	    maskLength = atoi(mask);
	    if (maskLength <= 0 || maskLength > 32) {
		config_perror("bad mask length");
		return;
	    }
	    maskBit = 0x80000000L;
	    se.sourceMask.sin_addr.s_addr = 0;
	    while (maskLength--) {
		se.sourceMask.sin_addr.s_addr |= maskBit;
		maskBit >>= 1;
	    }
	    se.sourceMask.sin_addr.s_addr = htonl(se.sourceMask.sin_addr.s_addr);
	}
    }
    if ((se.sourceIp.sin_addr.s_addr & ~se.sourceMask.sin_addr.s_addr) != 0) {
	config_perror("source/mask mismatch");
	return;
    }
    strcpy(se.securityName, name);
    strcpy(se.community, community);
    sp = (struct vacm_securityEntry *)malloc (sizeof *sp);
    *sp = se;
    if (securityFirst != NULL) {
	securityLast->next = sp;
	securityLast = sp;
    }
    else {
	securityFirst = securityLast = sp;
    }
}

void vacm_free_security (void)
{
    struct vacm_securityEntry *sp;
    while ((sp = securityFirst)) {
	securityFirst = sp->next;
	free(sp);
    }
}

void vacm_parse_group (char *token, 
		       char *param)
{
    char *group, *model, *security;
    int imodel;
    struct vacm_groupEntry *gp = NULL;

    group = strtok (param, " \t\n");
    model = strtok (NULL, " \t\n");
    security = strtok (NULL, " \t\n");

    if (group == NULL || *group == 0) {
	config_perror("missing GROUP parameter");
	return;
    }
    if (model == NULL || *model == 0) {
	config_perror("missing MODEL parameter");
	return;
    }
    if (security == NULL || *security == 0) {
	config_perror("missing SECURITY parameter");
	return;
    }
    if (strcasecmp(model, "any") == 0) imodel = SNMP_SEC_MODEL_ANY;
    else if (strcasecmp(model, "v1") == 0) imodel = SNMP_SEC_MODEL_SNMPv1;
    else if (strcasecmp(model, "v2c") == 0) imodel = SNMP_SEC_MODEL_SNMPv2c;
    else if (strcasecmp(model, "v2p") == 0) imodel = SNMP_SEC_MODEL_SNMPv2p;
    else if (strcasecmp(model, "usm") == 0) imodel = SNMP_SEC_MODEL_USM;
    else {
	config_perror("bad security model");
	return;
    }
    gp = vacm_createGroupEntry(imodel, security);
    strcpy (gp->groupName, group);
    gp->storageType = SNMP_STORAGE_PERMANENT;
    gp->status = SNMP_ROW_ACTIVE;
    free (gp->reserved);
    gp->reserved = NULL;
}

void vacm_free_group (void)
{
    vacm_destroyAllGroupEntries();
}

void vacm_parse_access (char *token, char *param)
{
    char *name, *context, *model, *level, *prefix, *readView, *writeView, *notify;
    int imodel, ilevel;
    struct vacm_accessEntry *ap;

    name = strtok(param, " \t\n");
    if (!name) {
	config_perror("missing NAME parameter");
	return;
    }
    context = strtok(NULL, " \t\n");
    if (!context) {
	config_perror("missing CONTEXT parameter");
	return;
    }
    model = strtok(NULL, " \t\n");
    if (!model) {
	config_perror("missing MODEL parameter");
	return;
    }
    level = strtok(NULL, " \t\n");
    if (!level) {
	config_perror("missing LEVEL parameter");
	return;
    }
    prefix = strtok(NULL, " \t\n");
    if (!prefix) {
	config_perror("missing PREFIX parameter");
	return;
    }
    readView = strtok(NULL, " \t\n");
    if (!readView) {
	config_perror("missing readView parameter");
	return;
    }
    writeView = strtok(NULL, " \t\n");
    if (!writeView) {
	config_perror("missing writeView parameter");
	return;
    }
    notify = strtok(NULL, " \t\n");
    if (!notify) {
	config_perror("missing notifyView parameter");
	return;
    }
    if (strcmp(context, "\"\"") == 0) *context = 0;
    if (strcasecmp(model, "any") == 0) imodel = SNMP_SEC_MODEL_ANY;
    else if (strcasecmp(model, "v1") == 0) imodel = SNMP_SEC_MODEL_SNMPv1;
    else if (strcasecmp(model, "v2c") == 0) imodel = SNMP_SEC_MODEL_SNMPv2c;
    else if (strcasecmp(model, "v2p") == 0) imodel = SNMP_SEC_MODEL_SNMPv2p;
    else if (strcasecmp(model, "usm") == 0) imodel = SNMP_SEC_MODEL_USM;
    else {
	config_perror("bad security model");
	return;
    }
    if (strcasecmp(level, "noauth") == 0) ilevel = SNMP_SEC_LEVEL_NOAUTH;
    else if (strcasecmp(level, "noauthnopriv") == 0) ilevel = SNMP_SEC_LEVEL_NOAUTH;
    else if (strcasecmp(level, "auth") == 0) ilevel = SNMP_SEC_LEVEL_AUTHNOPRIV;
    else if (strcasecmp(level, "authnopriv") == 0) ilevel = SNMP_SEC_LEVEL_AUTHNOPRIV;
    else if (strcasecmp(model, "priv") == 0) ilevel = SNMP_SEC_LEVEL_AUTHPRIV;
    else if (strcasecmp(model, "authpriv") == 0) ilevel = SNMP_SEC_LEVEL_AUTHPRIV;
    else {
	config_perror("bad security level");
	return;
    }
    ap = vacm_createAccessEntry (name, context, imodel, ilevel);
    strcpy(ap->readView, readView);
    strcpy(ap->writeView, writeView);
    strcpy(ap->notifyView, notify);
    ap->storageType = SNMP_STORAGE_PERMANENT;
    ap->status = SNMP_ROW_ACTIVE;
    free (ap->reserved);
    ap->reserved = NULL;
}

void vacm_free_access (void)
{
    vacm_destroyAllAccessEntries();
}

void vacm_parse_view (char *token, 
		      char *param)
{
    char *name, *type, *subtree, *mask;
    int inclexcl;
    struct vacm_viewEntry *vp;
    oid suboid[MAX_OID_LEN];
    int suboid_len = 0;
    u_char viewMask[sizeof (vp->viewMask)];
    int i;

    init_mib();
    name = strtok (param, " \t\n");
    if (!name) {
	config_perror("missing NAME parameter");
	return;
    }
    type = strtok (NULL, " \n\t");
    if (!type) {
	config_perror("missing TYPE parameter");
	return;
    }
    subtree = strtok(NULL, " \t\n");
    if (!subtree) {
	config_perror("missing SUBTREE parameter");
	return;
    }
    mask = strtok(NULL, " \t\n");

    if (strcmp(type, "included") == 0) inclexcl = SNMP_VIEW_INCLUDED;
    else if (strcmp(type, "excluded") == 0) inclexcl = SNMP_VIEW_EXCLUDED;
    else {
	config_perror("TYPE must be included/excluded?");
	return;
    }
    suboid_len = MAX_OID_LEN;
    if (!read_objid(subtree, suboid, &suboid_len)) {
	config_perror("bad SUBTREE object id");
	return;
    }
    if (mask) {
	int val;
	i = 0;
	for (mask = strtok(mask, "."); mask; mask = strtok(NULL, ":")) {
	    if (i >= sizeof(viewMask)) {
		config_perror("MASK too long");
		return;
	    }
	    if (sscanf(mask, "%x", &val) == 0) {
		config_perror("invalid MASK");
		return;
	    }
	    viewMask[i] = val;
	    i++;
	}
    }
    else {
	for (i = 0; i < sizeof(viewMask); i++)
	    viewMask[i] = 0xff;
    }
    vp = vacm_createViewEntry(name, suboid, suboid_len);
    memcpy(vp->viewMask, viewMask, sizeof(viewMask));
    vp->viewType = inclexcl;
    vp->viewStorageType = SNMP_STORAGE_PERMANENT;
    vp->viewStatus = SNMP_ROW_ACTIVE;
    free (vp->reserved);
    vp->reserved = NULL;
}

void vacm_free_view (void)
{
    vacm_destroyAllViewEntries();
}

int vacm_in_view (struct packet_info *pi,
		  oid *name,
		  int namelen)
{
    struct vacm_securityEntry *sp = securityFirst;
    struct vacm_accessEntry *ap;
    struct vacm_groupEntry *gp;
    struct vacm_viewEntry *vp;
    char *vn;
    char *sn;

    if (pi->sec_model == SNMP_SEC_MODEL_SNMPv1 || pi->sec_model == SNMP_SEC_MODEL_SNMPv2c) {
	DEBUGMSGTL(("mibII/vacm_vars", "vacm_in_view: ver=%d, source=%.8x, community=%s\n", pi->version, pi->source.sin_addr.s_addr, pi->community));

	/* allow running without snmpd.conf */
	if (sp == NULL) {
	    DEBUGMSGTL(("mibII/vacm_vars", "vacm_in_view: accepted with no com2sec entries\n"));
	    switch (pi->pdutype) {
	    case SNMP_MSG_GET:
	    case SNMP_MSG_GETNEXT:
	    case SNMP_MSG_GETBULK:
		return 1;
	    default:
		return 0;
	    }
	}
	while (sp) {
	    if ((pi->source.sin_addr.s_addr & sp->sourceMask.sin_addr.s_addr)
		    == sp->sourceIp.sin_addr.s_addr
		&& strcmp(sp->community, (char *)pi->community) == 0)
		break;
	    sp = sp->next;
	}
	if (sp == NULL) return 0;
	sn = sp->securityName;
    }
    else {
	sn = NULL;
    }

    if (sn == NULL) return 0;
    DEBUGMSGTL(("mibII/vacm_vars", "vacm_in_view: securityName == %s\n", sn));
    gp = vacm_getGroupEntry(pi->sec_model, sn);
    if (gp == NULL) return 0;
    DEBUGMSGTL(("mibII/vacm_vars", "vacm_in_view: groupName == %s\n", gp->groupName));
    ap = vacm_getAccessEntry(gp->groupName, "", pi->sec_model, pi->sec_level);
    if (ap == NULL) return 0;
    switch (pi->pdutype) {
    case SNMP_MSG_GET:
    case SNMP_MSG_GETNEXT:
    case SNMP_MSG_GETBULK:
	vn = ap->readView;
	break;
    case SNMP_MSG_SET:
	vn = ap->writeView;
	break;
    case SNMP_MSG_TRAP:
    case SNMP_MSG_TRAP2:
    case SNMP_MSG_INFORM:
	vn = ap->notifyView;
	break;
    default:
	fprintf(stderr,"bad msg type in vacm_in_view: %d\n", pi->pdutype);
	vn = ap->readView;
    }
    DEBUGMSGTL(("mibII/vacm_vars", "vacm_in_view: viewName == %s\n", vn));
    vp = vacm_getViewEntry (vn, name, namelen);
    if (vp == NULL) return 0;
    DEBUGMSGTL(("mibII/vacm_vars", "vacm_in_view: viewType == %d\n", vp->viewType));
    if (vp->viewType == SNMP_VIEW_EXCLUDED) return 0;
    return 1;
}

u_char *var_vacm_sec2group(struct variable *vp,
			   oid *name,
			   int *length,
			   int exact,
			   int *var_len,
			   WriteMethod **write_method)
{
    struct vacm_groupEntry *gp;
    oid *groupSubtree;
    int groupSubtreeLen;
    int secmodel;
    char secname[32], *cp;

    write_method = NULL;
    if (memcmp(name, vp->name, sizeof(oid)*vp->namelen) != 0) {
	memcpy(name, vp->name, sizeof(oid)*vp->namelen);
	*length = vp->namelen;
    }
    if (exact) {
	if (*length < 13) return NULL;

	secmodel = name[11];
	groupSubtree = name+13;
	groupSubtreeLen = *length - 13;
	cp = secname;
	while (groupSubtreeLen-- > 0) {
	    *cp++ = *groupSubtree++;
	}
	*cp = 0;

	gp = vacm_getGroupEntry(secmodel, secname);
    }
    else {
	secmodel = *length > 11 ? name[11] : 0;
	groupSubtree = name+12;
	groupSubtreeLen = *length - 12;
	cp = secname;
	while (groupSubtreeLen-- > 0) {
	    *cp++ = *groupSubtree++;
	}
	*cp = 0;
	vacm_scanGroupInit();
	while ((gp = vacm_scanGroupNext()) != NULL) {
	    if (gp->securityModel > secmodel ||
		(gp->securityModel == secmodel && strcmp(gp->securityName, secname) > 0))
		break;
	}
	if (gp) {
	    name[11] = gp->securityModel;
	    *length = 12;
	    cp = gp->securityName;
	    while (*cp) {
		name[(*length)++] = *cp++;
	    }
	}
    }

    if (!gp) return NULL;

    *var_len =sizeof(long_return);
    switch (vp->magic) {
    case SECURITYMODEL:
	long_return = gp->securityModel;
	return (u_char *)&long_return;
    case SECURITYNAME:
	*var_len = gp->securityName[0];
	return (u_char *)&gp->securityName[1];
    case SECURITYGROUP:
	*var_len = strlen(gp->groupName);
	return (u_char *)gp->groupName;
    case SECURITYSTORAGE:
	long_return = gp->storageType;
	return (u_char *)&long_return;
    case SECURITYSTATUS:
	long_return = gp->status;
	return (u_char *)&long_return;
    }
    return NULL;
}

u_char *var_vacm_access(struct variable *vp,
			oid *name,
			int *length,
			int exact,
			int *var_len,
			WriteMethod **write_method)
{
    struct vacm_accessEntry *gp;
    int secmodel;
    int seclevel;
    char groupName[32];
    char contextPrefix[32];
    oid *op;
    int len;
    char *cp;
    int cmp;

    write_method = NULL;
    if (memcmp(name, vp->name, sizeof(oid)*vp->namelen) != 0) {
	memcpy(name, vp->name, sizeof(oid)*vp->namelen);
	*length = vp->namelen;
    }
    if (exact) {
	if (*length < 15) return NULL;

	op = name+11;
	len = *op++;
	cp = groupName;
	while (len-- > 0) {
	    *cp++ = *op++;
	}
	*cp = 0;
	len = *op++;
	cp = contextPrefix;
	while (len-- > 0) {
	    *cp++ = *op++;
	}
	*cp = 0;
	secmodel = *op++;
	seclevel = *op++;
	if (op != name + *length) {
	    return NULL;
	}

	gp = vacm_getAccessEntry(groupName, contextPrefix, secmodel, seclevel);
    }
    else {
	secmodel = seclevel = 0;
	groupName[0] = 0;
	contextPrefix[0] = 0;
	op = name+11;
	if (op >= name + *length) {
	}
	else {
	    len = *op;
	    cp = groupName;
	    while (len-- >= 0) {
		*cp++ = *op++;
	    }
	    *cp = 0;
	}
	if (op >= name + *length) {
	}
	else {
	    len = *op;
	    cp = contextPrefix;
	    while (len-- >= 0) {
		*cp++ = *op++;
	    }
	    *cp = 0;
	}
	if (op >= name + *length) {
	}
	else {
	    secmodel = *op++;
	}
	if (op >= name + *length) {
	}
	else {
	    seclevel = *op++;
	}
	vacm_scanAccessInit();
	while ((gp = vacm_scanAccessNext()) != NULL) {
	    cmp = strcmp(gp->groupName, groupName);
	    if (cmp > 0) break;
	    if (cmp < 0) continue;
	    cmp = strcmp(gp->contextPrefix, contextPrefix);
	    if (cmp > 0) break;
	    if (cmp < 0) continue;
	    if (gp->securityModel > secmodel) break;
	    if (gp->securityModel < secmodel) continue;
	    if (gp->securityLevel > seclevel) break;
	}
	if (gp) {
	    *length = 11;
	    cp = gp->groupName;
	    do {
		name[(*length)++] = *cp++;
	    } while (*cp);
	    cp = gp->contextPrefix;
	    do {
		name[(*length)++] = *cp++;
	    } while (*cp);
	    name[(*length)++] = gp->securityModel;
	    name[(*length)++] = gp->securityLevel;
	}
    }

    if (!gp) return NULL;

    *var_len =sizeof(long_return);
    switch (vp->magic) {
    case ACCESSMATCH:
	long_return = gp->contextMatch;
	return (u_char *)&long_return;
    case ACCESSLEVEL:
	long_return = gp->securityLevel;
	return (u_char *)&long_return;
    case ACCESSMODEL:
	long_return = gp->securityModel;
	return (u_char *)&long_return;
    case ACCESSPREFIX:
	*var_len = *gp->contextPrefix;
	return (u_char *)&gp->contextPrefix[1];
    case ACCESSREAD:
	*var_len = strlen(gp->readView);
	return (u_char *)gp->readView;
    case ACCESSWRITE:
	*var_len = strlen(gp->writeView);
	return (u_char *)gp->writeView;
    case ACCESSNOTIFY:
	*var_len = strlen(gp->notifyView);
	return (u_char *)gp->notifyView;
    case ACCESSSTORAGE:
	long_return = gp->storageType;
	return (u_char *)&long_return;
    case ACCESSSTATUS:
	long_return = gp->status;
	return (u_char *)&long_return;
    }
    return NULL;
}

u_char *var_vacm_view(struct variable *vp,
		      oid *name,
		      int *length,
		      int exact,
		      int *var_len,
		      WriteMethod **write_method)
{
    struct vacm_viewEntry *gp;
    char viewName[32];
    oid subtree[MAX_OID_LEN];
    int subtreeLen = 0;
    oid *op, *op1;
    int len;
    char *cp;
    int cmp;

    write_method = NULL;
    if (memcmp(name, vp->name, sizeof(oid)*vp->namelen) != 0) {
	memcpy(name, vp->name, sizeof(oid)*vp->namelen);
	*length = vp->namelen;
    }
    if (exact) {
	if (*length < 15) return NULL;

	op = name+12;
	len = *op++;
	cp = viewName;
	while (len-- > 0) {
	    *cp++ = *op++;
	}
	*cp = 0;
	len = *length - (op - name);
	op1 = subtree;
	while (len-- > 0) {
	    *op1++ = *op++;
	    subtreeLen++;
	}
	if (op != name + *length) {
	    return NULL;
	}

	gp = vacm_getViewEntry(viewName, subtree, subtreeLen);
    }
    else {
	viewName[0] = 0;
	op = name+12;
	if (op >= name + *length) {
	}
	else {
	    len = *op;
	    cp = viewName;
	    while (len-- >= 0) {
		*cp++ = *op++;
	    }
	    *cp = 0;
	}
	if (op >= name + *length) {
	}
	else {
	    len = *length - (op - name);
	    op1 = subtree;
	    while (len-- >= 0) {
		*op1++ = *op++;
		subtreeLen++;
	    }
	}
	vacm_scanViewInit();
	while ((gp = vacm_scanViewNext()) != NULL) {
	    cmp = strcmp(gp->viewName, viewName);
	    if (cmp > 0) break;
	    if (cmp < 0) continue;
	}
	if (gp) {
	    *length = 12;
	    cp = gp->viewName;
	    do {
		name[(*length)++] = *cp++;
	    } while (*cp);
	    op1 = gp->viewSubtree;
	    len = gp->viewSubtreeLen;
	    do {
		name[(*length)++] = *op1++;
	    } while (len-- > 0);
	}
    }

    if (!gp) return NULL;

    *var_len =sizeof(long_return);
    switch (vp->magic) {
    case VIEWNAME:
	*var_len = gp->viewName[0];
	return (u_char *)&gp->viewName[1];
    case VIEWSUBTREE:
	*var_len = gp->viewSubtreeLen*sizeof(oid);
	return (u_char *)gp->viewSubtree;
    case VIEWMASK:
	*var_len = (gp->viewSubtreeLen + 7) / 8;
	return (u_char *)gp->viewMask;
    case VIEWTYPE:
	long_return = gp->viewType;
	return (u_char *)&long_return;
    case VIEWSTORAGE:
	long_return = gp->viewStorageType;
	return (u_char *)&long_return;
    case VIEWSTATUS:
	long_return = gp->viewStatus;
	return (u_char *)&long_return;
    }
    return NULL;
}
