// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: flt_rule.c,v 1.6 2003/05/13 12:47:59 dev Exp $

#include <ntddk.h>
#include <ndis.h>
#include <stdlib.h>

#include "flt_rule.h"
#include "filter.h"
#include "log.h"
#include "net.h"
#include "sock.h"

struct str_value {
	const	char *str;
	int		value;
};

static int	str_value(const struct str_value *sv, const char *str);
static char	*_strpbrk(const char *string, const char *strCharSet);
static int	get_amp(char *str, u_long *addr, u_long *mask, u_short *port, u_short *port2);

static ULONG	hex_toi(const char *str);

static struct str_value filter_sv[] = {
	{"allow",	FILTER_ALLOW},
	{"deny",	FILTER_DENY},
	{NULL,		0}
};

static struct str_value proto_sv[] = {
	{"ip",		IP_RULE},
	{"tcp",		TCP_RULE},
	{"udp",		UDP_RULE},
	{"icmp",	ICMP_RULE},
	{NULL,		0}
};

static struct str_value direction_sv[] = {
	{"in",		DIRECTION_IN},
	{"out",		DIRECTION_OUT},
	{NULL,		0}
};

static struct str_value flags_sv[] = {
	{"F",		TH_FIN},
	{"S",		TH_SYN},
	{"R",		TH_RST},
	{"P",		TH_PUSH},
	{"A",		TH_ACK},
	{"U",		TH_URG},
	{NULL,		0}
};

int
parse_rule(char *str, struct flt_rule *rule)
{
	static char delim[] = " \t";
	char *p = str, *p2, tmp[2];
	int v;
	ULONG a;
	BOOLEAN has_code;

	memset(rule, 0, sizeof(*rule));

	// by default log using of all rules!
	rule->log = 1;

	/* allow|deny */

	if (*p == '\0') {
		log_str("PARSE\tparse_rule: filter \"allow\" or \"deny\" is missing");
		return 0;
	}

	p2 = _strpbrk(p, delim);
	if (p2 != NULL)
		*(p2++) = '\0';

	v = str_value(filter_sv, p);
	if (v == -1) {
		log_str("PARSE\tparse_rule: \"%s\" is not \"allow\" or \"deny\" filter", p);
		return 0;
	}
	rule->result = v;

	p = p2;

	/* ip|tcp|udp|icmp */

	if (*p == '\0') {
		log_str("PARSE\tparse_rule: protocol ip|tcp|udp|icmp is missing");
		return 0;
	}

	p2 = _strpbrk(p, delim);
	if (p2 != NULL)
		*(p2++) = '\0';

	v = str_value(proto_sv, p);
	if (v == -1) {
		log_str("PARSE\tparse_rule: \"%s\" is not ip|tcp|udp|icmp protocol", p);
		return 0;
	}
	rule->rule_type = v;

	p = p2;

	/* in|out */

	if (p == NULL) {
		log_str("PARSE\tparse_rule: direction \"in\" or \"out\" is missing");
		return 0;
	}

	p2 = _strpbrk(p, delim);
	if (p2 != NULL)
		*(p2++) = '\0';

	v = str_value(direction_sv, p);
	if (v == -1) {
		log_str("PARSE\tparse_rule: \"%s\" is not \"in\" or \"out\" direction", p);
		return 0;
	}
	rule->direction = v;
	
	p = p2;

	/* [if] */

	if (p != NULL && *p == 'i') {

		p2 = _strpbrk(p, delim);
		if (p2 != NULL)
			*(p2++) = '\0';

		if (strcmp(p, "if") != 0) {
			log_str("PARSE\tparse_rule: \"%s\" is not \"if\" keyword", p);
			return 0;
		}
		
		p = p2;

		if (p == NULL) {
			log_str("PARSE\tparse_rule: if keyword: interface index is missing");
			return 0;
		}

		p2 = _strpbrk(p, delim);
		if (p2 != NULL)
			*(p2++) = '\0';

		a = atoi(p);
		if (a == 0 || a > 100) {	// 100 interfaces is cool!
			log_str("PARSE\tparse_rule: if keyword: invalid interface index: %s", p);
			return 0;
		}

		rule->iface = (int)a;

		p = p2;
	}

	/* from */

	if (p == NULL) {
		log_str("PARSE\tparse_rule: keyword \"from\" is missing");
		return 0;
	}

	p2 = _strpbrk(p, delim);
	if (p2 != NULL)
		*(p2++) = '\0';

	if (strcmp(p, "from") != 0) {
		log_str("PARSE\tparse_rule: \"%s\" is not \"from\" keyword");
		return 0;
	}

	p = p2;

	/* <addr-from> */

	if (p == NULL) {
		log_str("PARSE\tparse_rule: from address is missing");
		return 0;
	}

	p2 = _strpbrk(p, delim);
	if (p2 != NULL)
		*(p2++) = '\0';

	switch(rule->rule_type) {
	case IP_RULE:
	case ICMP_RULE:
		// read IP-address/mask
		if (!get_amp(p, &rule->rule.ip.src_addr, &rule->rule.ip.src_mask, NULL, NULL)) {
			log_str("PARSE\tparse_rule: invalid from address \"%s\"", p);
			return 0;
		}
		break;

	case UDP_RULE:
	case TCP_RULE:
		// read IP-address/mask:port
		if (!get_amp(p, &rule->rule.udp.ip.src_addr, &rule->rule.udp.ip.src_mask,
			&rule->rule.udp.src_port1, &rule->rule.udp.src_port2)) {

			log_str("PARSE\tparse_rule: invalid from address \"%s\"", p);
			return 0;
		}
	}

	p = p2;

	/* to */

	if (p == NULL) {
		log_str("PARSE\tparse_rule: keyword \"to\" is missing");
		return 0;
	}

	p2 = _strpbrk(p, delim);
	if (p2 != NULL)
		*(p2++) = '\0';

	if (strcmp(p, "to") != 0) {
		log_str("PARSE\tparse_rule: \"%s\" is not \"to\" keyword");
		return 0;
	}

	p = p2;

	/* <addr-to> */

	if (p == NULL) {
		log_str("PARSE\tparse_rule: to address is missing");
		return 0;
	}

	p2 = _strpbrk(p, delim);
	if (p2 != NULL)
		*(p2++) = '\0';

	if (strcmp(p, "any") != 0) {
		// not any

		switch(rule->rule_type) {
		case IP_RULE:
		case ICMP_RULE:
			// read IP-address/mask
			if (!get_amp(p, &rule->rule.ip.dst_addr, &rule->rule.ip.dst_mask, NULL, NULL)) {
				log_str("PARSE\tparse_rule: invalid to address \"%s\"", p);
				return 0;
			}
			break;

		case UDP_RULE:
		case TCP_RULE:
			// read IP-address/mask:port
			if (!get_amp(p, &rule->rule.udp.ip.dst_addr, &rule->rule.udp.ip.dst_mask,
				&rule->rule.udp.dst_port1, &rule->rule.udp.dst_port2)) {

				log_str("PARSE\tparse_rule: invalid to address \"%s\"", p);
				return 0;
			}

		}
	}

	p = p2;

	switch (rule->rule_type) {
	case IP_RULE:

		/* [ipproto <ipproto>] */

		if (p == NULL)
			break;

		if (*p != 'i')
			break;

		p2 = _strpbrk(p, delim);
		if (p2 != NULL)
			*(p2++) = '\0';

		if (strcmp(p, "ipproto") != 0) {
			log_str("PARSE\tparse_rule: \"%s\" is not \"ipproto\" keyword", p);
			return 0;
		}

		p = p2;

		if (p == NULL) {
			log_str("PARSE\tparse_rule: \"ipproto\" keyword: missing argument");
			return 0;
		}

		p2 = _strpbrk(p, delim);
		if (p2 != NULL)
			*(p2++) = '\0';

		a = atoi(p);
		if (a < 1 || a > 0xff) {
			log_str("PARSE\tparse_rule: \"ipproto\" keyword: invalid argument: %s", p);
			return 0;
		}

		rule->rule.ip.ip_proto = (UCHAR)a;
		rule->rule.ip.use_ip_proto = TRUE;

		p = p2;
		break;

	case TCP_RULE:

		/* [flags <flags>] */

		if (p == NULL)
			break;

		if (*p != 'f')
			break;

		p2 = _strpbrk(p, delim);
		if (p2 != NULL)
			*(p2++) = '\0';

		if (strcmp(p, "flags") != 0) {
			log_str("PARSE\tparse_rule: \"%s\" is not \"flags\" keyword", p);
			return 0;
		}

		p = p2;

		if (p == NULL) {
			log_str("PARSE\tparse_rule: \"flags\" keyword: missing argument");
			return 0;
		}

		p2 = _strpbrk(p, delim);
		if (p2 != NULL)
			*(p2++) = '\0';

		for (; *p != '\0'; p++) {
			tmp[0] = *p;
			tmp[1] = '\0';

			v = str_value(flags_sv, tmp);
			if (v == -1) {
				log_str("PARSE\tparse_rule: invalid flag: %s", tmp);
				return 0;
			}

			p++;
			if (*p == '\0' || (*p != '+' && *p != '-')) {
				log_str("PARSE\tparse_rule: missing '+' or '-' after flag %s", tmp);
				return 0;
			}

			if (*p == '+')
				rule->rule.tcp.flags_set |= v;
			else
				rule->rule.tcp.flags_unset |= v;
		}

		p = p2;

		break;

	case ICMP_RULE:

		/* type type[.code] */

		if (p == NULL)
			break;

		if (*p != 't')
			break;

		p2 = _strpbrk(p, delim);
		if (p2 != NULL)
			*(p2++) = '\0';

		if (strcmp(p, "type") != 0) {
			log_str("PARSE\tparse_rule: \"%s\" is not \"type\" keyword", p);
			return 0;
		}

		p = p2;

		if (p == NULL) {
			log_str("PARSE\tparse_rule: \"type\" keyword: missing argument");
			return 0;
		}

		p2 = strchr(p, '.');
		if (p2 != NULL) {
			*(p2++) = '\0';
			
			has_code = TRUE;
		
		} else {
			p2 = _strpbrk(p, delim);
			if (p2 != NULL)
				*(p2++) = '\0';
			
			has_code = FALSE;
		}

		a = atoi(p);
		if ((a == 0 && strcmp(p, "0") != 0) || a < 0 || a > 0xff) {
			log_str("PARSE\tparse_rule: \"type\" keyword: invalid type: %s", p);
			return 0;
		}
		rule->rule.icmp.type = (UCHAR)a;
		rule->rule.icmp.use_type = TRUE;

		p = p2;

		if (has_code) {
			
			p2 = _strpbrk(p, delim);
			if (p2 != NULL)
				*(p2++) = '\0';

			a = atoi(p);
			if (a == 0 && strcmp(p, "0") != 0) {
				log_str("PARSE\tparse_rule: \"type\" keyword: invalid code: %s", p);
				return 0;
			}
			rule->rule.icmp.code = (UCHAR)a;
			rule->rule.icmp.use_code = TRUE;
		
			p = p2;
		}

		break;
	}

	/* [nolog] */

	if (p != NULL) {

		if (strcmp(p, "nolog") == 0)
			rule->log = 0;
		else {
			log_str("PARSE\tparse_rule: \"%s\" is not \"nolog\"", p);
			return 0;
		}

	}

	return 1;
}

int
str_value(const struct str_value *sv, const char *str)
{
	while (sv->str != NULL) {
		if (strcmp(sv->str, str) == 0)
			return sv->value;
		sv++;
	}
	return -1;
}

int
get_amp(char *string, u_long *addr, u_long *mask, u_short *port, u_short *port2)
{
	char *p, *addr_str = NULL, *mask_str = NULL, *port_str = NULL,
		*port2_str = NULL;
	int result = 0;
	
	addr_str = string;

	// "/mask"
	p = strchr(string, '/');
	if (p != NULL) {
		*(p++) = '\0';
		mask_str = p;
	}

	if (port != NULL) {
		// ":port"
		p = strchr(mask_str ? mask_str : string, ':');
		if (p != NULL) {
			*(p++) = '\0';
			port_str = p;
			
			// "-port2"
			p = strchr(port_str, '-');
			if (p != NULL) {
				*(p++) = '\0';
				port2_str = p;
			}
		}
	}

	// is it any?
	if (strcmp(addr_str, "any") == 0) {
		*addr = 0;
		*mask = 0;
	} else {
		*addr = inet_addr(addr_str);
		if (*addr == INADDR_NONE && strcmp(addr_str, "255.255.255.255") != 0) {
			log_str("PARSE\tInvalid address: %s", addr_str);
			return 0;
		}
		*mask = INADDR_ANY;	// default mask: 255.255.255.255

		if (mask_str != NULL) {
			int n = atoi(mask_str);
			if ((n == 0 && strcmp(mask_str, "0") != 0) || n < 0 || n > 32) {
				log_str("PARSE\tInvalid mask: %s", mask_str);
				return 0;
			}
			if (n == 0)
				*mask = 0;
			else {
				int i;
				for (i = 1, *mask = 0x80000000; i < n; i++)
					*mask |= *mask >> 1;
				*mask = htonl(*mask);
			}
		} else
			*mask = INADDR_NONE;
	}

	if (port != NULL) {
		if (port_str != NULL) {
			int n = atoi(port_str);
			if ((n == 0 && strcmp(port_str, "0") != 0) || n < 0 || n > 0xffff) {
				log_str("PARSE\tInvalid port: %s", port_str);
				return 0;
			}
			*port = htons((USHORT)n);
		} else
			*port = 0;
	}

	if (port2 != NULL) {
		if (port2_str) {
			int n = atoi(port2_str);
			if ((n == 0 && strcmp(port2_str, "0") != 0) || n < 0 || n > 0xffff) {
				log_str("PARSE\tInvalid port2: %s", port2_str);
				return 0;
			}
			*port2 = htons((USHORT)n);
		} else
			*port2 = 0;
	}

	return 1;
}

char *
_strpbrk(const char *string, const char *strCharSet)
{
	while (*string) {
		if (strchr(strCharSet, *string))
			return (char *)string;
		string++;
	}
	return NULL;
}

ULONG
hex_toi(const char *str)
{
	ULONG result = 0;
	for (; *str != '\0'; str++) {
		result <<= 4;
		if (*str >= '0' && *str <= '9')
			result |= *str - '0';
		else if (*str >= 'a' && * str <= 'f')
			result |= *str - 'a' + 0xa;
		else if (*str >= 'A' && * str <= 'F')
			result |= *str - 'A' + 0xa;
		else
			return (ULONG)-1;
	}
	return result;
}
