#include "generic.h"

#undef IP_FORWARDING_SYMBOL
#define IP_FORWARDING_SYMBOL "ip_forwarding"

#undef TCP_TTL_SYMBOL
#define TCP_TTL_SYMBOL "tcp_ttl"

#define UTMP_HAS_NO_TYPE
#define UTMP_FILE "/etc/utmp"
