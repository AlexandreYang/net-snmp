/*
 * Note: this file originally auto-generated by mib2c using
 *  : generic-table-constants.m2c,v 1.5 2005/07/15 22:41:16 rstory Exp $
 *
 * $Id$
 */
#ifndef SNMPNOTIFYFILTERTABLE_CONSTANTS_H
#define SNMPNOTIFYFILTERTABLE_CONSTANTS_H

#ifdef __cplusplus
extern          "C" {
#endif

#include <snmp-notification-mib/snmpNotifyFilterTable/snmpNotifyFilterTable_data_storage.h>

    /*
     * column number definitions for table snmpNotifyFilterTable 
     */
#define SNMPNOTIFYFILTERTABLE_OID              1,3,6,1,6,3,13,1,3

#define COLUMN_SNMPNOTIFYFILTERSUBTREE         1

#define COLUMN_SNMPNOTIFYFILTERMASK         2
#define COLUMN_SNMPNOTIFYFILTERMASK_FLAG    (0x1 << 1)

#define COLUMN_SNMPNOTIFYFILTERTYPE         3
#define COLUMN_SNMPNOTIFYFILTERTYPE_FLAG    (0x1 << 2)

#define COLUMN_SNMPNOTIFYFILTERSTORAGETYPE         4
#define COLUMN_SNMPNOTIFYFILTERSTORAGETYPE_FLAG    (0x1 << 3)

#define COLUMN_SNMPNOTIFYFILTERROWSTATUS         5
#define COLUMN_SNMPNOTIFYFILTERROWSTATUS_FLAG    (0x1 << 4)


#define SNMPNOTIFYFILTERTABLE_MIN_COL   COLUMN_SNMPNOTIFYFILTERMASK
#define SNMPNOTIFYFILTERTABLE_MAX_COL   COLUMN_SNMPNOTIFYFILTERROWSTATUS


    /*
     * TODO:405:r: Review SNMPNOTIFYFILTERTABLE_SETTABLE_COLS macro.
     * OR together all the writable cols.
     */
#define SNMPNOTIFYFILTERTABLE_SETTABLE_COLS (COLUMN_SNMPNOTIFYFILTERMASK_FLAG | COLUMN_SNMPNOTIFYFILTERTYPE_FLAG | COLUMN_SNMPNOTIFYFILTERSTORAGETYPE_FLAG | COLUMN_SNMPNOTIFYFILTERROWSTATUS_FLAG)
    /*
     * TODO:405:r: Review SNMPNOTIFYFILTERTABLE_REQUIRED_COLS macro.
     * OR together all the required rows for row creation.
     * default is writable cols w/out defaults.
     */
#define SNMPNOTIFYFILTERTABLE_REQUIRED_COLS (COLUMN_SNMPNOTIFYFILTERROWSTATUS_FLAG)

    /*
     * NOTES on enums
     * ==============
     *
     * Value Mapping
     * -------------
     * If the values for your data type don't exactly match the
     * possible values defined by the mib, you should map them
     * below. For example, a boolean flag (1/0) is usually represented
     * as a TruthValue in a MIB, which maps to the values (1/2).
     *
     */
/*************************************************************************
 *************************************************************************
 *
 * enum definitions for table snmpNotifyFilterTable
 *
 *************************************************************************
 *************************************************************************/

/*************************************************************
 * constants for enums for the MIB node
 * snmpNotifyFilterType (INTEGER / ASN_INTEGER)
 *
 * since a Textual Convention may be referenced more than once in a
 * MIB, protect againt redefinitions of the enum values.
 */


/*************************************************************
 * constants for enums for the MIB node
 * snmpNotifyFilterStorageType (StorageType / ASN_INTEGER)
 *
 * since a Textual Convention may be referenced more than once in a
 * MIB, protect againt redefinitions of the enum values.
 */
#ifndef STORAGETYPE_ENUMS
#define STORAGETYPE_ENUMS

#define STORAGETYPE_OTHER  1
#define STORAGETYPE_VOLATILE  2
#define STORAGETYPE_NONVOLATILE  3
#define STORAGETYPE_PERMANENT  4
#define STORAGETYPE_READONLY  5

#endif                          /* STORAGETYPE_ENUMS */


/*************************************************************
 * constants for enums for the MIB node
 * snmpNotifyFilterRowStatus (RowStatus / ASN_INTEGER)
 *
 * since a Textual Convention may be referenced more than once in a
 * MIB, protect againt redefinitions of the enum values.
 */
#ifndef ROWSTATUS_ENUMS
#define ROWSTATUS_ENUMS

#define ROWSTATUS_ACTIVE  1
#define ROWSTATUS_NOTINSERVICE  2
#define ROWSTATUS_NOTREADY  3
#define ROWSTATUS_CREATEANDGO  4
#define ROWSTATUS_CREATEANDWAIT  5
#define ROWSTATUS_DESTROY  6

#endif                          /* ROWSTATUS_ENUMS */




#ifdef __cplusplus
}
#endif
#endif                          /* SNMPNOTIFYFILTERTABLE_OIDS_H */
