// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: filter.h,v 1.4 2003/05/13 12:48:00 dev Exp $

#ifndef _filter_h_
#define _filter_h_

#include "ndis_hk_ioctl.h"
#include "net.h"

NTSTATUS	filter_init(void);
void		filter_free(void);

NTSTATUS	filter_reload(void);

// callbacks
int		filter_ip(int direction, int iface, struct ip_hdr *ip_hdr);
int		filter_tcp(int direction, int iface, struct ip_hdr *ip_hdr, struct tcp_hdr *tcp_hdr);
int		filter_udp(int direction, int iface, struct ip_hdr *ip_hdr, struct udp_hdr *udp_hdr);
int		filter_icmp(int direction, int iface, struct ip_hdr *ip_hdr, struct icmp_hdr *icmp_hdr);

// ip part of rule
struct ip_rule {
	ULONG	src_addr;
	ULONG	src_mask;

	ULONG	dst_addr;
	ULONG	dst_mask;

	BOOLEAN	use_ip_proto;
	UCHAR	ip_proto;
};

// tcp part of rule
struct tcp_rule {
	struct	ip_rule ip;		// don't change or move! the same as in udp_rule

	USHORT	src_port1;		// don't change or move! the same as in udp_rule
	USHORT	src_port2;		// don't change or move! the same as in udp_rule

	USHORT	dst_port1;		// don't change or move! the same as in udp_rule
	USHORT	dst_port2;		// don't change or move! the same as in udp_rule

	UCHAR	flags_set;
	UCHAR	flags_unset;
};

// udp part of rule
struct udp_rule {
	struct	ip_rule ip;		// don't change or move! ip

	USHORT	src_port1;
	USHORT	src_port2;

	USHORT	dst_port1;
	USHORT	dst_port2;
};

// icmp part of rule
struct icmp_rule {
	struct	ip_rule ip;		// don't change or move! ip

	BOOLEAN	use_type;
	UCHAR	type;

	BOOLEAN	use_code;
	UCHAR	code;
};

// type of flt_rule
enum rule_type {
	IP_RULE,
	TCP_RULE,
	UDP_RULE,
	ICMP_RULE
};

// result of flt_rule
enum {
	FILTER_UNKNOWN,
	FILTER_ALLOW,
	FILTER_DROP,
	FILTER_DENY
};

// rule
struct flt_rule {
	struct	flt_rule *next;

	int		rule_line;
	int		direction;
	int		iface;
	int		result;
	BOOLEAN	log;
	
	enum	rule_type rule_type;
	union {
		struct	ip_rule ip;
		struct	tcp_rule tcp;
		struct	udp_rule udp;
		struct	icmp_rule icmp;
	} rule;
};

// filter callback
BOOLEAN		filter_packet(int direction, int iface, PNDIS_PACKET packet, struct filter_nfo *self,
	BOOLEAN packet_unchanged);

#endif
