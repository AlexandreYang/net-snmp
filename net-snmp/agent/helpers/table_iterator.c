/* table_iterator.c */
/** @defgroup table_iterator table_iterator: The table iterator helper is designed to simplify the task of writing a table handler for the net-snmp agent when the data being accessed is not in an oid sorted form and must be accessed externally.
 *  @ingroup table
 *  Functionally, it is a specialized version of the more
 *  generic table helper but easies the burden of GETNEXT processing by
 *  manually looping through all the data indexes retrieved through
 *  function calls which should be supplied by the module that wishes
 *  help.  The module the table_iterator helps should, afterwards,
 *  never be called for the case of "MODE_GETNEXT" and only for the GET
 *  and SET related modes instead.
 */

#include <net-snmp/net-snmp-config.h>

#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <net-snmp/agent/table.h>
#include <net-snmp/agent/serialize.h>
#include <net-snmp/agent/table_iterator.h>

#if HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

/* doesn't work yet, but shouldn't be serialized (for efficiency) */
#undef NOT_SERIALIZED
#define TABLE_ITERATOR_LAST_CONTEXT "ti_last_c"

/** returns a mib_handler object for the table_iterator helper */
mib_handler *
get_table_iterator_handler(iterator_info *iinfo) {
    mib_handler *me=
        create_handler(TABLE_ITERATOR_NAME, table_iterator_helper_handler);

    if (!me || !iinfo)
        return NULL;
    
    me->myvoid = iinfo; 
    return me;
}

    
/** registers a table after attaching it to a table_iterator helper */
int
register_table_iterator(handler_registration *reginfo,
                        iterator_info *iinfo) {
    inject_handler(reginfo, get_table_iterator_handler(iinfo));
#ifndef NOT_SERIALIZED
    inject_handler(reginfo, get_serialize_handler());
#endif
    return register_table(reginfo, iinfo->table_reginfo);
}

/** extracts the table_iterator specific data from a request */
inline void *
extract_iterator_context(request_info *request) 
{
    return request_get_list_data(request, TABLE_ITERATOR_NAME);
}

/** implements the table_iterator helper */
int
table_iterator_helper_handler(
    mib_handler               *handler,
    handler_registration      *reginfo,
    agent_request_info        *reqinfo,
    request_info              *requests) {
  
    table_registration_info   *tbl_info;
    oid coloid[MAX_OID_LEN];
    size_t coloid_len;
    int ret;
    static oid myname[MAX_OID_LEN];
    static int myname_len;
    int oldmode;
    iterator_info *iinfo;
    
    iinfo = (iterator_info *) handler->myvoid;
    tbl_info = iinfo->table_reginfo;

    /* copy in the table registration oid for later use */
    coloid_len = reginfo->rootoid_len+2;
    memcpy(coloid, reginfo->rootoid, reginfo->rootoid_len * sizeof(oid));
    coloid[reginfo->rootoid_len] = 1; /* table.entry node */
    
    /* illegally got here if these functions aren't defined */
    if (iinfo->get_first_data_point == NULL ||
        iinfo->get_next_data_point == NULL) {
        snmp_log(LOG_ERR, "table_iterator helper called without data accessor functions\n");
        return SNMP_ERR_GENERR;
    }

    /* XXXWWW: deal with SET caching */

#ifdef NOT_SERIALIZED
    while(requests) /* XXX: currently only serialized */
#endif
        {
        /* XXXWWW: optimize by reversing loops (look through data only once) */
        struct variable_list *results = NULL;
        struct variable_list *index_search = NULL; /* WWW: move up? */
        struct variable_list *free_this_index_search = NULL;
        table_request_info *table_info =
            extract_table_info(requests);
        void *callback_loop_context = NULL;
        void *callback_data_context = NULL;
        void *callback_data_keep = NULL;
        
        if (requests->processed != 0) {
#ifdef NOT_SERIALIZED
            continue;
#else
            return SNMP_ERR_NOERROR;
#endif
        }

        if (reqinfo->mode != MODE_GET &&
            reqinfo->mode != MODE_GETNEXT &&
            reqinfo->mode != MODE_GETBULK && /* XXX */
            reqinfo->mode != MODE_SET_RESERVE1) {
            goto skip_processing;
        }
            

        if (table_info->colnum > tbl_info->max_column) {
            requests->processed = 1;
#ifdef NOT_SERIALIZED
            break;
#else
            return SNMP_ERR_NOERROR;
#endif
        }
        
        /* XXX: if loop through everything, these are never free'd
           since iterator returns NULL and thus we forget about
           these */
        
        index_search = snmp_clone_varbind(table_info->indexes);
        free_this_index_search = index_search;

        /* below our minimum column? */
        if (table_info->colnum < tbl_info->min_column) {
            results = (iinfo->get_first_data_point)(&callback_loop_context,
                                                    &callback_data_context,
                                                    index_search,
                                                    iinfo);
            if (iinfo->free_loop_context)
                (iinfo->free_loop_context)(callback_loop_context,
                                           iinfo);
            goto got_results;
        }

        /* XXX: do "only got some indexes" */
        
        /* find the next legal result to return */
        /* find the first node */
        index_search = (iinfo->get_first_data_point)(&callback_loop_context,
                                                     &callback_data_context,
                                                     index_search,
                                                     iinfo);

        /* table.entry.column node */
        coloid[reginfo->rootoid_len+1] = table_info->colnum;

        switch(reqinfo->mode) {
            case MODE_GETNEXT:
            case MODE_GETBULK: /* XXXWWW */
                /* loop through all data and find next one */
                while(index_search) {
                    /* compare the node with previous results */
                    if (check_getnext_reply(requests, coloid, coloid_len,
                                            index_search, &results)) {

                        /* result is our current choice, so keep a pointer to
                           the data that the lower handler wants us to
                           remember (possibly freeing the last known "good"
                           result data pointer) */
                        if (callback_data_keep &&
                            iinfo->free_data_context) {
                            (iinfo->free_data_context)(callback_data_keep,
                                                       iinfo);
                        }
                        if (iinfo->make_data_context &&
                            !callback_data_context) {
                            callback_data_context = 
                                (iinfo->make_data_context)(callback_loop_context, iinfo);
                            
                        }
                        callback_data_keep = callback_data_context;
                        callback_data_context = NULL;
                    } else {
                        if (callback_data_context && iinfo->free_data_context)
                            (iinfo->free_data_context)(callback_data_context,
                                                       iinfo);
                    }

                    /* get the next node in the data chain */
                    index_search =
                        (iinfo->get_next_data_point)(&callback_loop_context,
                                                     &callback_data_context,
                                                     index_search,
                                                     iinfo);

                    if (!index_search && !results &&
                        tbl_info->max_column > table_info->colnum) {
                        /* restart loop.  XXX: Should cache this better */
                        table_info->colnum++;
                        coloid[reginfo->rootoid_len+1] = table_info->colnum;
                        /* XXX: free old contexts first? */
                        index_search = snmp_clone_varbind(table_info->indexes);
                        index_search =
                            (iinfo->get_first_data_point)(&callback_loop_context,
                                                          &callback_data_context,
                                                          index_search,
                                                          iinfo);
                    }
                }

                break;

            case MODE_GET:
            case MODE_SET_RESERVE1:
                /* loop through all data till exact results are found */
    
                while(index_search) {
                    build_oid_noalloc(myname, MAX_OID_LEN, &myname_len,
                                      coloid, coloid_len, index_search);
                    if (snmp_oid_compare(myname, myname_len,
                                         requests->requestvb->name,
                                         requests->requestvb->name_length) == 0) {
                        /* found the exact match, so we're done */
                        if (iinfo->make_data_context &&
                            !callback_data_context) {
                            callback_data_context = 
                                (iinfo->make_data_context)(callback_loop_context, iinfo);
                            
                        }
                        callback_data_keep = callback_data_context;
                        callback_data_context = NULL;
                        results = snmp_clone_varbind(index_search);
                        snmp_set_var_objid(results, myname, myname_len);
                        goto got_results;
                    } else {
                        /* free not-needed data context */
                        if (callback_data_context && iinfo->free_data_context)
                            (iinfo->free_data_context)(callback_data_context,
                                                       iinfo);
                    }
                    
                    /* get the next node in the data chain */
                    index_search =
                        (iinfo->get_next_data_point)(&callback_loop_context,
                                                     &callback_data_context,
                                                     index_search,
                                                     iinfo);
                }
                break;

            default:
                /* the rest of the set states have been dealt with already */
                goto got_results;
        }
        
        /* XXX: free index_search? */
        if (callback_loop_context && iinfo->free_loop_context) {
            callback_loop_context = NULL;
            (iinfo->free_loop_context)(callback_loop_context,
                                       iinfo);
        }

          got_results: /* not milk */
        
        if (!results && !MODE_IS_SET(reqinfo->mode)) {
            /* no results found. */
            /* XXX: check for at least one entry at the very top */
#ifdef NOT_SERIALIZED
            break;
#else
            if (callback_loop_context && iinfo->free_loop_context_at_end)
                (iinfo->free_loop_context_at_end)(callback_loop_context, iinfo);
	    if (free_this_index_search != NULL) {
	      snmp_free_varbind(free_this_index_search);
	    }
            return SNMP_ERR_NOERROR;
#endif
        }
        
          skip_processing:
        /* OK, here results should be a pointer to the data that we
           actually need to GET */
        oldmode = reqinfo->mode;
        if (reqinfo->mode == MODE_GETNEXT ||
            reqinfo->mode == MODE_GETBULK) { /* XXX */
            snmp_set_var_objid(requests->requestvb, results->name,
                               results->name_length);
            reqinfo->mode = MODE_GET;
        }
        if (reqinfo->mode == MODE_GET ||
            reqinfo->mode == MODE_GETNEXT ||
            reqinfo->mode == MODE_GETBULK || /* XXX */
            reqinfo->mode == MODE_SET_RESERVE1) {
            /* first (or only) pass stuff */
            /* let set requsets use previously constructed data */
            snmp_free_var(results);
            if (callback_data_keep)
                request_add_list_data(requests, create_data_list(TABLE_ITERATOR_NAME, callback_data_keep, NULL));
            request_add_list_data(requests, create_data_list(TABLE_ITERATOR_LAST_CONTEXT, callback_loop_context, NULL));
        }
        
        DEBUGMSGTL(("table_iterator", "doing mode: %s\n",
                    se_find_label_in_slist("agent_mode", oldmode)));
        ret = call_next_handler(handler, reginfo, reqinfo, requests);
        if (oldmode == MODE_GETNEXT ||
            oldmode == MODE_GETBULK) { /* XXX */
            if (requests->requestvb->type == ASN_NULL) {
                /* get next skipped this value for this column, we
                   need to keep searching forward */
                requests->requestvb->type = ASN_PRIV_RETRY;
            }
            reqinfo->mode = oldmode;
        }

        callback_data_context =
            request_get_list_data(requests, TABLE_ITERATOR_NAME);
        callback_loop_context =
            request_get_list_data(requests, TABLE_ITERATOR_LAST_CONTEXT);

        if (reqinfo->mode == MODE_GET ||
            reqinfo->mode == MODE_GETNEXT ||
            reqinfo->mode == MODE_GETBULK || /* XXX */
            reqinfo->mode == MODE_SET_FREE ||
            reqinfo->mode == MODE_SET_UNDO ||
            reqinfo->mode == MODE_SET_COMMIT) {
            if (callback_data_keep && iinfo->free_data_context)
                (iinfo->free_data_context)(callback_data_keep,
                                           iinfo);
            if (free_this_index_search)
                snmp_free_varbind(free_this_index_search);
#ifdef NOT_SERIALIZED
            if (callback_loop_context && iinfo->free_loop_context_at_end)
                (iinfo->free_loop_context_at_end)(callback_loop_context, iinfo);
#endif
        }
#ifdef NOT_SERIALIZED
        return ret;
#else
        requests = requests->next;
#endif
        }
#ifdef NOT_SERIALIZED
    if (reqinfo->mode == MODE_GET ||
        reqinfo->mode == MODE_GETNEXT ||
        reqinfo->mode == MODE_GETBULK || /* XXX */
        reqinfo->mode == MODE_SET_FREE ||
        reqinfo->mode == MODE_SET_UNDO ||
        reqinfo->mode == MODE_SET_COMMIT) {
        if (callback_loop_context && iinfo->free_loop_context_at_end)
            (iinfo->free_loop_context_at_end)(callback_loop_context, iinfo);
    }
#endif
    return SNMP_ERR_NOERROR;
}

