/*
 * snmpbulkwalk.c - send SNMPv2 Bulk requests to a network entity, walking a
 * subtree.
 *
 */
/*********************************************************************
	Copyright 1988, 1989, 1991, 1992 by Carnegie Mellon University

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of CMU not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
**********************************************************************/
#include <net-snmp/net-snmp-config.h>

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
#include <sys/types.h>
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if TIME_WITH_SYS_TIME
# ifdef WIN32
#  include <sys/timeb.h>
# else
#  include <sys/time.h>
# endif
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <stdio.h>
#include <ctype.h>
#if HAVE_WINSOCK_H
#include <winsock.h>
#endif
#if HAVE_NETDB_H
#include <netdb.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include <net-snmp/asn1.h>
#include <net-snmp/snmp_api.h>
#include <net-snmp/snmp_client.h>
#include <net-snmp/mib.h>
#include <net-snmp/snmp.h>
#include <net-snmp/snmp_impl.h>
#include <net-snmp/system.h>
#include <net-snmp/default_store.h>
#include <net-snmp/snmp_parse_args.h>
#include <net-snmp/getopt.h>

#define DS_WALK_INCLUDE_REQUESTED		1
#define DS_WALK_PRINT_STATISTICS		2
#define DS_WALK_DONT_CHECK_LEXICOGRAPHIC	3

oid objid_mib[] = {1, 3, 6, 1, 2, 1};
int numprinted = 0;
int reps = 10, non_reps = 0;

void
usage (void)
{
  fprintf(stderr,"Usage: snmpbulkwalk ");
  snmp_parse_args_usage(stderr);
  fprintf(stderr," [<objectID>]\n\n");
  snmp_parse_args_descriptions(stderr);
  fprintf(stderr, "  -C <APPOPTS>  Set various application specific behaviours:\n");
  fprintf(stderr, "\t\t  APPOPTS values:\n");
  fprintf(stderr,"\t\t      c:       do not check that returned OIDs are increasing\n");
  fprintf(stderr,"\t\t      i:       include given OIDs in the search range\n");
  fprintf(stderr,"\t\t      n<NUM>:  set non-repeaters to <NUM>\n");
  fprintf(stderr,"\t\t      p:       print the number of variables found\n");
  fprintf(stderr,"\t\t      r<NUM>:  set max-repeaters to <NUM>\n");
}

static void
snmp_get_and_print(struct snmp_session *ss, oid *theoid, size_t theoid_len)
{
    struct snmp_pdu *pdu, *response;
    struct variable_list *vars;
    int status;
    
    pdu = snmp_pdu_create(SNMP_MSG_GET);
    snmp_add_null_var(pdu, theoid, theoid_len);

    status = snmp_synch_response(ss, pdu, &response);
    if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
        for (vars = response->variables; vars; vars = vars->next_variable) {
            numprinted++;
            print_variable(vars->name, vars->name_length, vars);
        }
    }
    if (response) {
        snmp_free_pdu(response);
    }
}

static
void optProc(int argc, char *const *argv, int opt)
{
    char *endptr = NULL;

    switch (opt) {
    case 'C':
	while (*optarg) {
	    switch (*optarg++) {
	    case 'c':
		ds_toggle_boolean(DS_APPLICATION_ID,
				  DS_WALK_DONT_CHECK_LEXICOGRAPHIC);
		break;

	    case 'i':
		ds_toggle_boolean(DS_APPLICATION_ID,
				  DS_WALK_INCLUDE_REQUESTED);
		break;

	    case 'n':
	    case 'r':
		if (*(optarg - 1) == 'r') {
		    reps     = strtol(optarg, &endptr, 0);
		} else {
		    non_reps = strtol(optarg, &endptr, 0);
		}

		if (endptr == optarg) {
		    /*  No number given -- error.  */
		    usage();
		    exit(1);
		} else {
		    optarg = endptr;
		    if (isspace(*optarg)) {
			return;
		    }
		}
		break;

	    case 'p':
		ds_toggle_boolean(DS_APPLICATION_ID,
				  DS_WALK_PRINT_STATISTICS);
		break;

	    default:
		fprintf(stderr, "Unknown flag passed to -C: %c\n", optarg[-1]);
		exit(1);
	    }
	}
	break;
    }
}

int main(int argc, char *argv[])
{
    struct snmp_session  session, *ss;
    struct snmp_pdu *pdu, *response;
    struct variable_list *vars;
    int    arg;
    oid    name[MAX_OID_LEN];
    size_t name_length;
    oid    root[MAX_OID_LEN];
    size_t rootlen;
    int    count;
    int    running;
    int    status;
    int    check;
    int    exitval = 0;

    ds_register_config(ASN_BOOLEAN, "snmpwalk", "includeRequested",
                       DS_APPLICATION_ID, DS_WALK_INCLUDE_REQUESTED);
    ds_register_config(ASN_BOOLEAN, "snmpwalk", "printStatistics",
                       DS_APPLICATION_ID, DS_WALK_PRINT_STATISTICS);
    ds_register_config(ASN_BOOLEAN, "snmpwalk", "dontCheckOrdering",
                       DS_APPLICATION_ID, DS_WALK_DONT_CHECK_LEXICOGRAPHIC);

    /* get the common command line arguments */
    switch (arg = snmp_parse_args(argc, argv, &session, "C:", optProc)) {
    case -2:
    	exit(0);
    case -1:
        usage();
        exit(1);
    default:
        break;
    }

    /* get the initial object and subtree */
    if (arg < argc) {
      /* specified on the command line */
      rootlen = MAX_OID_LEN;
      if (snmp_parse_oid(argv[arg], root, &rootlen) == NULL) {
        snmp_perror(argv[arg]);
        exit(1);
      }
    } else {
      /* use default value */
      memmove(root, objid_mib, sizeof(objid_mib));
      rootlen = sizeof(objid_mib) / sizeof(oid);
    }

    SOCK_STARTUP;

    /* open an SNMP session */
    ss = snmp_open(&session);
    if (ss == NULL) {
      /* diagnose snmp_open errors with the input struct snmp_session pointer */
      snmp_sess_perror("snmpbulkwalk", &session);
      SOCK_CLEANUP;
      exit(1);
    }

    /* setup initial object name */
    memmove(name, root, rootlen * sizeof(oid));
    name_length = rootlen;

    running = 1;

    check =
        !ds_get_boolean(DS_APPLICATION_ID, DS_WALK_DONT_CHECK_LEXICOGRAPHIC);
    if (ds_get_boolean(DS_APPLICATION_ID, DS_WALK_INCLUDE_REQUESTED)) {
        snmp_get_and_print(ss, root, rootlen);
    }
    
    while (running) {
      /* create PDU for GETBULK request and add object name to request */
      pdu = snmp_pdu_create(SNMP_MSG_GETBULK);
      pdu->non_repeaters   = non_reps;
      pdu->max_repetitions = reps;  /* fill the packet */
      snmp_add_null_var(pdu, name, name_length);

      /* do the request */
      status = snmp_synch_response(ss, pdu, &response);
      if (status == STAT_SUCCESS){
        if (response->errstat == SNMP_ERR_NOERROR){
          /* check resulting variables */
          for(vars = response->variables; vars; vars = vars->next_variable){
            if ((vars->name_length < rootlen) ||
                (memcmp(root, vars->name, rootlen * sizeof(oid))!=0)) {
              /* not part of this subtree */
              running = 0;
              continue;
            }
            numprinted++;
            print_variable(vars->name, vars->name_length, vars);
            if ((vars->type != SNMP_ENDOFMIBVIEW) &&
                (vars->type != SNMP_NOSUCHOBJECT) &&
                (vars->type != SNMP_NOSUCHINSTANCE)){
              /* not an exception value */
	      if (check && snmp_oid_compare(name, name_length, vars->name,
	      				    vars->name_length) >= 0) {
		char name_buf[SPRINT_MAX_LEN], var_buf[SPRINT_MAX_LEN];
		sprint_objid(name_buf, name, name_length);
		sprint_objid(var_buf, vars->name, vars->name_length);
		fprintf(stderr, "Error: OID not increasing: %s >= %s\n",
			name_buf, var_buf);
	        running = 0;
		exitval = 1;
	      }
	      /*  Check if last variable, and if so, save for next request.  */
	      if (vars->next_variable == NULL) {
		  memmove(name, vars->name, vars->name_length * sizeof(oid));
		  name_length = vars->name_length;
	      }
            } else {
              /* an exception value, so stop */
              running = 0;
	    }
          }
        } else {
          /* error in response, print it */
          running = 0;
          if (response->errstat == SNMP_ERR_NOSUCHNAME){
            printf("End of MIB\n");
          } else {
            fprintf(stderr, "Error in packet.\nReason: %s\n",
		    snmp_errstring(response->errstat));
            if (response->errindex != 0){
              fprintf(stderr, "Failed object: ");
              for(count = 1, vars = response->variables;
                    vars && count != response->errindex;
                    vars = vars->next_variable, count++)
                /*EMPTY*/;
              if (vars)
                fprint_objid(stderr, vars->name, vars->name_length);
              fprintf(stderr, "\n");
            }
	    exitval = 2;
          }
        }
      } else if (status == STAT_TIMEOUT){
        fprintf(stderr, "Timeout: No Response from %s\n", session.peername);
        running = 0;
	exitval = 1;
      } else {    /* status == STAT_ERROR */
        snmp_sess_perror("snmpbulkwalk", ss);
        running = 0;
	exitval = 1;
      }
      if (response)
        snmp_free_pdu(response);
    }

    if (numprinted == 0 && status == STAT_SUCCESS) {
        /* no printed successful results, which may mean we were
           pointed at an only existing instance.  Attempt a GET, just
           for get measure. */
        snmp_get_and_print(ss, root, rootlen);
    }
    snmp_close(ss);

    if (ds_get_boolean(DS_APPLICATION_ID, DS_WALK_PRINT_STATISTICS)) {
        printf("Variables found: %d\n", numprinted);
    }

    SOCK_CLEANUP;
    return exitval;
}
