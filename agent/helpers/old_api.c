#include <net-snmp/net-snmp-config.h>

#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "mibincl.h"
#include <net-snmp/tools.h>
#include <net-snmp/agent/snmp_agent.h>
#include <net-snmp/agent/old_api.h>
#include <net-snmp/agent/agent_callbacks.h>

#if HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#define MIB_CLIENTS_ARE_EVIL 1

/** @defgroup old_api old_api: Calls mib module code written in the old style of code.
 *  @ingroup handler
 *  This is a backwards compatilibity module that allows code written
 *  in the old API to be run under the new handler based architecture.
 *  Use it by calling register_old_api().
 *  @{
 */

/** returns a old_api handler that should be the final calling
 * handler.  Don't use this function.  Use the register_old_api()
 * function instead.
 */
mib_handler *
get_old_api_handler(void) {
    return create_handler("old_api", old_api_helper);
}
    

/** Registers an old API set into the mib tree.  Functionally this
 * mimics the old register_mib_context() function (and in fact the new
 * register_mib_context() function merely calls this new old_api one).
 */
int
register_old_api(const char *moduleName,
                 struct variable *var,
                 size_t varsize,
                 size_t numvars,
                 oid *mibloc,
                 size_t mibloclen,
                 int priority,
                 int range_subid,
                 oid range_ubound,
                 struct snmp_session *ss,
                 const char *context,
                 int timeout,
                 int flags) {
    
    old_api_info *old_info = SNMP_MALLOC_TYPEDEF(old_api_info);
    int i;
    
    old_info->var = var;
    old_info->varsize = varsize;
    old_info->numvars = numvars;
    old_info->ss = ss;
    old_info->flags = flags;
    
    /* register all subtree nodes */
    for(i = 0; i < numvars; i++) {
        struct variable *vp;
        handler_registration *reginfo =
            SNMP_MALLOC_TYPEDEF(handler_registration);

        memdup((void *) &vp,
               (void *) (struct variable *) ((char *) var + varsize*i),
               varsize);
 
        reginfo->handler = get_old_api_handler();
        reginfo->handlerName = strdup(moduleName);
        reginfo->rootoid_len = (mibloclen + vp->namelen);
        reginfo->rootoid =
            (oid *) malloc(reginfo->rootoid_len * sizeof(oid));
        
        memcpy(reginfo->rootoid, mibloc, mibloclen*sizeof(oid));
        memcpy(reginfo->rootoid + mibloclen, vp->name, vp->namelen
               * sizeof(oid));
        reginfo->handler->myvoid = (void *) vp;

        reginfo->priority = priority;
        reginfo->range_subid = range_subid;
        
        reginfo->range_ubound = range_ubound;
        reginfo->timeout = timeout;
        reginfo->contextName = (context) ? strdup(context) : NULL;
        reginfo->modes = HANDLER_CAN_RWRITE;

        /* register ourselves in the mib tree */
        if (register_handler(reginfo) != MIB_REGISTERED_OK) {
	    snmp_handler_registration_free(reginfo);
	    SNMP_FREE(vp);
	}
    }
    SNMP_FREE(old_info);
    return SNMPERR_SUCCESS;
}

/** registers a row within a mib table */
int 
register_mib_table_row(const char *moduleName,
                       struct variable *var,
                       size_t varsize,
                       size_t numvars,
                       oid *mibloc,
                       size_t mibloclen,
                       int priority,
                       int var_subid,
                       struct snmp_session *ss,
                       const char *context,
                       int timeout,
                       int flags)
{
    int i = 0, rc = 0;
    oid ubound = 0;

    for (i = 0; i < numvars; i++) {
	struct variable *vr = (struct variable *)((char *)var+(i*varsize));
	handler_registration *r = SNMP_MALLOC_TYPEDEF(handler_registration);
	
	if (r == NULL) {
	    /*  Unregister whatever we have registered so far, and
		return an error.  */
	    rc = MIB_REGISTRATION_FAILED;
	    break;
	}
	memset(r, 0, sizeof(handler_registration));
	
        r->handler = get_old_api_handler();
        r->handlerName = strdup(moduleName);

	if (r->handlerName == NULL) {
	    snmp_handler_registration_free(r);
	    break;
	}

        r->rootoid_len = mibloclen;
        r->rootoid = (oid *)malloc(r->rootoid_len * sizeof(oid));

        if (r->rootoid == NULL) {
	    snmp_handler_registration_free(r);
	    rc = MIB_REGISTRATION_FAILED;
	    break;
	}
        memcpy(r->rootoid, mibloc, mibloclen*sizeof(oid));
	memcpy((u_char *)(r->rootoid + (var_subid - 1)), vr->name, 
	       vr->namelen * sizeof(oid));
	DEBUGMSGTL(("register_mib_table_row", "rootoid "));
	DEBUGMSGOID(("register_mib_table_row", r->rootoid, r->rootoid_len));
	DEBUGMSG(("register_mib_table_row", "\n"));
        r->handler->myvoid = (void *)malloc(varsize);
	
	if (r->handler->myvoid == NULL) {
	    snmp_handler_registration_free(r);
	    rc = MIB_REGISTRATION_FAILED;
	    break;
	}
	memcpy((char *)r->handler->myvoid, vr, varsize);

        r->contextName = (context) ? strdup(context) : NULL;

	if (context != NULL && r->contextName == NULL) {
	    snmp_handler_registration_free(r);
	    rc = MIB_REGISTRATION_FAILED;
	    break;
	}

        r->priority = priority;
        r->range_subid = 0; // var_subid;
        r->range_ubound = 0; // range_ubound;
        r->timeout = timeout;
        r->modes = HANDLER_CAN_RWRITE;

        /*  Register this column and row  */
        if ((rc = register_handler_nocallback(r)) != MIB_REGISTERED_OK) {
	    DEBUGMSGTL(("register_mib_table_row", "register failed %d\n", rc));
	    snmp_handler_registration_free(r);
	    break;
	}

	if (vr->namelen > 0) {
	    if (vr->name[vr->namelen - 1] > ubound) {
		ubound = vr->name[vr->namelen - 1];
	    }
	}
    }

    if (rc == MIB_REGISTERED_OK) {
	struct register_parameters reg_parms;

	reg_parms.name = mibloc;
	reg_parms.namelen = mibloclen;
	reg_parms.priority = priority;
	reg_parms.flags = (u_char)flags;
	reg_parms.range_subid  = var_subid;
	reg_parms.range_ubound = ubound;
	reg_parms.timeout = timeout;
	rc = snmp_call_callbacks(SNMP_CALLBACK_APPLICATION,
				 SNMPD_CALLBACK_REGISTER_OID, &reg_parms);
    }

    return rc;
}

/** implements the old_api handler */
int
old_api_helper(mib_handler               *handler,
               handler_registration      *reginfo,
               agent_request_info        *reqinfo,
               request_info              *requests) {

#if MIB_CLIENTS_ARE_EVIL
    oid			save[MAX_OID_LEN];
    size_t		savelen = 0;
#endif
    struct variable	compat_var, *cvp = &compat_var;
    int exact = 1;
    int status;
    int have_delegated = 0;
    
    struct variable *vp;
    WriteMethod *write_method = NULL;
    size_t len;
    u_char *access = NULL;
    old_api_cache *cacheptr;
    AddVarMethod *add_method;
    struct agent_snmp_session  *oldasp = NULL;

    vp = (struct variable *) handler->myvoid;

    /* create old variable structure with right information */
    memcpy(cvp->name, reginfo->rootoid, reginfo->rootoid_len * sizeof(oid));
    cvp->namelen = reginfo->rootoid_len;
    cvp->type = vp->type;
    cvp->magic = vp->magic;
    cvp->acl = vp->acl;
    cvp->findVar = vp->findVar;

    switch(reqinfo->mode) {
        case MODE_GETNEXT:
        case MODE_GETBULK:
            exact = 0;
    }

    while(requests) {
        
#if MIB_CLIENTS_ARE_EVIL
        savelen = requests->requestvb->name_length;
        memcpy(save, requests->requestvb->name,
               savelen*sizeof(oid));
#endif

        switch(reqinfo->mode) {
            case MODE_GET:
            case MODE_GETNEXT:
            case MODE_SET_RESERVE1:
                /* Actually call the old mib-module function */
                if (vp && vp->findVar)
                    access = (*(vp->findVar))(cvp, requests->requestvb->name,
                                              &(requests->requestvb->name_length),
                                              exact, &len, &write_method);
                else
                    access = NULL;

#ifdef WWW_FIX
                if (IS_DELEGATED(cvp->type)) {
                    add_method = (AddVarMethod*)statP;
                    requests->delayed = 1;
                    have_delegated = 1;
                    continue; /* WWW: This may not get to the right place */
                }
#endif

                /* WWW: end range checking */
                if (access) {
                    /* result returned */
                    if (reqinfo->mode != MODE_SET_RESERVE1)
                        snmp_set_var_typed_value(requests->requestvb,
                                                 cvp->type, access, len);
                } else {
                    /* no result returned */
#if MIB_CLIENTS_ARE_EVIL
                    if (access == NULL) {
                        if (snmp_oid_compare(requests->requestvb->name,
                                             requests->requestvb->name_length,
                                             save, savelen) != 0) {
                            DEBUGMSGTL(("old_api", "evil_client: %s\n",
                                        reginfo->handlerName));
                            memcpy(requests->requestvb->name, save,
                                   savelen*sizeof(oid));
                            requests->requestvb->name_length = savelen;
                        }
                    }
#endif
                }

                /* AAA: fall through for everything that is a set (see BBB) */
                if (reqinfo->mode != MODE_SET_RESERVE1)
                    break;

                cacheptr = SNMP_MALLOC_TYPEDEF(old_api_cache);
                if (!cacheptr)
                    return set_request_error(reqinfo, requests,
                                             SNMP_ERR_RESOURCEUNAVAILABLE);
                cacheptr->data = access;
                cacheptr->write_method = write_method;
                request_add_list_data(requests,
                                      create_data_list(OLD_API_NAME, cacheptr,
                                                       free));
                /* BBB: fall through for everything that is a set (see AAA) */
                
            default:
                /* WWW: explicitly list the SET conditions */
                /* (the rest of the) SET contions */
                cacheptr =
                    (old_api_cache *) request_get_list_data(requests,
                                                            OLD_API_NAME);

                if (cacheptr == NULL || cacheptr->write_method == NULL) {
                    /* WWW: try to set ourselves if possible? */
                    return set_request_error(reqinfo, requests,
                                             SNMP_ERR_NOTWRITABLE);
                }

                oldasp = get_current_agent_session();
                set_current_agent_session(reqinfo->asp);
                status =
                    (*(cacheptr->write_method))(reqinfo->mode,
                                                requests->requestvb->val.string,
                                                requests->requestvb->type,
                                                requests->requestvb->val_len,
                                                cacheptr->data,
                                                requests->requestvb->name,
                                                requests->requestvb->name_length);
                set_current_agent_session(oldasp);

                if (status != SNMP_ERR_NOERROR) {
                    set_request_error(reqinfo, requests, status);
		}

                /* clean up is done by the automatic freeing of the
                   cache stored in the request. */

                break;
        }
        requests = requests->next;
    }
    return SNMP_ERR_NOERROR;
}

/** @} */

/* don't use this! */
static struct agent_snmp_session *current_agent_session = NULL;
struct agent_snmp_session  *
get_current_agent_session() {
    return current_agent_session;
}

/* don't use this! */
void
set_current_agent_session(struct agent_snmp_session  *asp) {
    current_agent_session = asp;
}

