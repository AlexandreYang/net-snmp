/*
 *  Template MIB group interface - party_vars.h
 *
 */
#ifndef _MIBGROUP_PARTY_VARS_H
#define _MIBGROUP_PARTY_VARS_H

extern FindVarMethod var_party;
extern WriteMethod write_party;
struct partyEntry *party_rowCreate (oid *, int);
void party_rowDelete (oid *, int);

#include "party.h"

#define PARTYTABLE	PARTYMIB, 2, 1, 1, 1

#ifdef IN_SNMP_VARS_C

struct variable2 party_variables[] = {
    {PARTYINDEX, ASN_INTEGER, RONLY, var_party, 1, {2}},
    {PARTYTDOMAIN, ASN_OBJECT_ID, RWRITE, var_party, 1, {3}},
    {PARTYTADDRESS, ASN_OCTET_STR, RWRITE, var_party, 1, {4}},
    {PARTYMAXMESSAGESIZE, ASN_INTEGER, RWRITE, var_party, 1, {5}},
    {PARTYLOCAL, ASN_INTEGER, RWRITE, var_party, 1, {6}},
    {PARTYAUTHPROTOCOL, ASN_OBJECT_ID, RWRITE, var_party, 1, {7}},
    {PARTYAUTHCLOCK, ASN_UINTEGER, RWRITE, var_party, 1, {8}},
    {PARTYAUTHPRIVATE, ASN_OCTET_STR, RWRITE, var_party, 1, {9}},
    {PARTYAUTHPUBLIC, ASN_OCTET_STR, RWRITE, var_party, 1, {10}},
    {PARTYAUTHLIFETIME, ASN_INTEGER, RWRITE, var_party, 1, {11}},
    {PARTYPRIVPROTOCOL, ASN_OBJECT_ID, RWRITE, var_party, 1, {12}},
    {PARTYPRIVPRIVATE, ASN_OCTET_STR, RWRITE, var_party, 1, {13}},
    {PARTYPRIVPUBLIC, ASN_OCTET_STR, RWRITE, var_party, 1, {14}},
    {PARTYCLONEFROM, ASN_OBJECT_ID, RONLY, var_party, 1, {15}},
    {PARTYSTORAGETYPE, ASN_INTEGER, RWRITE, var_party, 1, {16}},
    {PARTYSTATUS, ASN_INTEGER, RWRITE, var_party, 1, {17}}
};

config_load_mib( PARTYTABLE, 11, party_variables)

#endif
#endif /* _MIBGROUP_PARTY_VARS_H */
