/*
 *  Host Resources MIB - proc processor group implementation - hr_proc.c
 *
 */

#include <config.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "host_res.h"
#include "hr_proc.h"
#include "auto_nlist.h"
#include "agent_read_config.h"
#include "ucd-snmp/loadave.h"

#define HRPROC_MONOTONICALLY_INCREASING

	/*********************
	 *
	 *  Kernel & interface information,
	 *   and internal forward declarations
	 *
	 *********************/

extern void  Init_HR_Proc (void);
extern int   Get_Next_HR_Proc (void);
int header_hrproc (struct variable *,oid *, int *, int, int *,  WriteMethod **);

	/*********************
	 *
	 *  Initialisation & common implementation functions
	 *
	 *********************/


void init_hr_proc(void)
{
    init_device[ HRDEV_PROC ] = Init_HR_Proc;	
    next_device[ HRDEV_PROC ] = Get_Next_HR_Proc;
#ifdef HRPROC_MONOTONICALLY_INCREASING
    dev_idx_inc[ HRDEV_PROC ] = 1;
#endif
}

/*
  header_hrproc(...
  Arguments:
  vp	  IN      - pointer to variable entry that points here
  name    IN/OUT  - IN/name requested, OUT/name found
  length  IN/OUT  - length of IN/OUT oid's 
  exact   IN      - TRUE if an exact match was requested
  var_len OUT     - length of variable or 0 if function returned
  write_method
  
*/

int
header_hrproc(struct variable *vp,
		  oid *name,
		  int *length,
		  int exact,
		  int *var_len,
		  WriteMethod **write_method)
{
#define HRPROC_ENTRY_NAME_LENGTH	11
    oid newname[MAX_OID_LEN];
    int proc_idx, LowIndex=-1;
    int result;
    char c_oid[SPRINT_MAX_LEN];

    if (snmp_get_do_debugging()) {
      sprint_objid (c_oid, name, *length);
      DEBUGMSGTL(("host/hr_proc", "var_hrproc: %s %d\n", c_oid, exact));
    }

    memcpy( (char *)newname,(char *)vp->name, (int)vp->namelen * sizeof(oid));
	/* Find "next" proc entry */

    Init_HR_Proc();
    for ( ;; ) {
        proc_idx = Get_Next_HR_Proc();
        if ( proc_idx == -1 )
	    break;
	newname[HRPROC_ENTRY_NAME_LENGTH] = proc_idx;
        result = snmp_oid_compare(name, *length, newname, (int)vp->namelen + 1);
        if (exact && (result == 0)) {
	    LowIndex = proc_idx;
	    /* Save processor status information */
            break;
	}
	if ((!exact && (result < 0)) &&
		( LowIndex == -1 || proc_idx < LowIndex )) {
	    LowIndex = proc_idx;
	    /* Save processor status information */
#ifdef HRPROC_MONOTONICALLY_INCREASING
            break;
#endif
	}
    }

    if ( LowIndex == -1 ) {
        DEBUGMSGTL(("host/hr_proc", "... index out of range\n"));
        return(MATCH_FAILED);
    }

    memcpy( (char *)name,(char *)newname, ((int)vp->namelen + 1) * sizeof(oid));
    *length = vp->namelen + 1;
    *write_method = 0;
    *var_len = sizeof(long);	/* default to 'long' results */

    if (snmp_get_do_debugging()) {
      sprint_objid (c_oid, name, *length);
      DEBUGMSGTL(("host/hr_proc", "... get proc stats %s\n", c_oid));
    }
    return LowIndex;
}


	/*********************
	 *
	 *  System specific implementation functions
	 *
	 *********************/


u_char	*
var_hrproc(struct variable *vp,
	   oid *name,
	   int *length,
	   int exact,
	   int *var_len,
	   WriteMethod **write_method)
{
    int  proc_idx;
    double avenrun[3];

    proc_idx = header_hrproc(vp, name, length, exact, var_len, write_method);
    if ( proc_idx == MATCH_FAILED )
	return NULL;
    if (try_getloadavg(&avenrun[0], sizeof(avenrun) / sizeof(avenrun[0])) == -1)
	return NULL;

    switch (vp->magic){
	case HRPROC_ID:
            *var_len = nullOidLen;
	    return (u_char *) nullOid;
	case HRPROC_LOAD:
	    long_return = avenrun[0] * 100;	/* 1 minute average */
	    if ( long_return > 100 )
		long_return=100;
	    return (u_char *)&long_return;
	default:
	    ERROR_MSG("");
    }
    return NULL;
}


	/*********************
	 *
	 *  Internal implementation functions
	 *
	 *********************/

static int HRP_index;

void
Init_HR_Proc (void)
{
   HRP_index = 1;
}

int
Get_Next_HR_Proc (void)
{
		/*
		 * Silly question time:
		 *   How do you detect processors?
		 *   Assume we've just got one.
		 */

    if ( HRP_index < 2 ) 
        return ( HRDEV_PROC << HRDEV_TYPE_SHIFT ) + HRP_index++;
    else
        return -1;
}
