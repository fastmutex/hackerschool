// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: packet.c,v 1.5 2003/05/19 15:09:07 dev Exp $

#include <ntddk.h>
#include <ndis.h>

#include "filter.h"
#include "net.h"
#include "sock.h"

static int		process_transp(int direction, int iface, UCHAR proto, struct ether_hdr *eth_hdr,
							   struct ip_hdr *ip_hdr, char *pointer, UINT buffer_len,
                               struct filter_nfo *parent);

static int		process_ether(int direction, int iface, struct ether_hdr *ether_hdr, struct filter_nfo *parent);
static int		process_ip(int direction, int iface, struct ip_hdr *ip_hdr, struct filter_nfo *parent);
static int		process_tcp(int direction, int iface, struct ether_hdr *eth_hdr,
							struct ip_hdr *ip_hdr, struct tcp_hdr *tcp_hdr, struct filter_nfo *parent);
static int		process_udp(int direction, int iface, struct ip_hdr *ip_hdr, struct udp_hdr *udp_hdr,
							struct filter_nfo *parent);
static int		process_icmp(int direction, int iface, struct ip_hdr *ip_hdr, struct icmp_hdr *icmp_hdr,
							 struct filter_nfo *parent);

static USHORT	in_cksum(USHORT *buffer, int size);

static void		send_packet(int direction, int iface, char *packet_data, ULONG packet_size,
							struct filter_nfo *parent);

BOOLEAN
filter_packet(int direction, int iface, PNDIS_PACKET packet, struct filter_nfo *self,
	BOOLEAN packet_unchanged)
{
	struct filter_nfo *parent, *child;
	int result = FILTER_UNKNOWN;
	PNDIS_BUFFER buffer;
	UINT packet_len, buffer_len, buffer_offset, hdr_len;
	void *pointer;
	struct ether_hdr *ether_hdr;
	struct ip_hdr *ip_hdr;

	// get parent and child in filter stack
	if (direction == DIRECTION_IN) {
		// in - from top to bottom in stack: parent is upper and the child is lower
		parent = self->upper;
		child = self->lower;
	} else {
		// out - from bottom to top in stack: parent is lower and the child is upper
		parent = self->lower;
		child = self->upper;
	}

	// parse packet

	NdisQueryPacket(packet, NULL, NULL, &buffer, &packet_len);

	if (packet_len < sizeof(struct ether_hdr)) {
		KdPrint(("[ndis_flt] filter_packet: too small packet for ether_hdr! (%u)\n", packet_len));
		goto done;
	}

	/* process ether_hdr */

	NdisQueryBuffer(buffer, &ether_hdr, &buffer_len);

	if (buffer_len < sizeof(struct ether_hdr)) {
		KdPrint(("[ndis_flt] filter_packet: too small buffer for ether_hdr! (%u)\n", buffer_len));
		goto done;
	}
	buffer_offset = 0;

	result = process_ether(direction, iface, ether_hdr, parent);
	if (result != FILTER_ALLOW)
		goto done;

	result = FILTER_UNKNOWN;	// default value

	// go to the next header
	if (buffer_len > sizeof(struct ether_hdr)) {

		pointer = (char *)ether_hdr + sizeof(struct ether_hdr);
		buffer_offset += sizeof(struct ether_hdr);

		buffer_len -= sizeof(struct ether_hdr);

	} else {
		// use next buffer in chain
		NdisGetNextBuffer(buffer, &buffer);
		NdisQueryBuffer(buffer, &pointer, &buffer_len);
		buffer_offset = 0;
	}

	if (ntohs(ether_hdr->ether_type) == ETHERTYPE_IP) {

		/* process ip_hdr */

		if (buffer_len < sizeof(struct ip_hdr)) {
			KdPrint(("[ndis_flt] filter_packet: too small buffer for ip_hdr! (%u)\n",
				buffer_len));
			goto done;
		}

		ip_hdr = (struct ip_hdr *)pointer;
		hdr_len = ip_hdr->ip_hl * 4;

		if (buffer_len < hdr_len) {
			KdPrint(("[ndis_flt] filter_packet: too small buffer for ip_hdr! (%u vs. %u)\n",
				buffer_len, hdr_len));
			goto done;
		}

   		// check we've got the first fragment (doesn't work with another!)
		if ((ntohs(ip_hdr->ip_off) & IP_OFFMASK) != 0 && (ip_hdr->ip_off & IP_DF) == 0) {
			result = FILTER_ALLOW;
			goto done;
		}

		result = process_ip(direction, iface, ip_hdr, parent);
		if (result != FILTER_ALLOW)
			goto done;

		result = FILTER_UNKNOWN;	// default value

		// go to the next header
		if (buffer_len > hdr_len) {

			pointer = (char *)ip_hdr + hdr_len;
			buffer_offset += hdr_len;
					
			buffer_len -= hdr_len;
			
		} else {
			// use next buffer in chain
			NdisGetNextBuffer(buffer, &buffer);
			NdisQueryBuffer(buffer, &pointer, &buffer_len);
			buffer_offset = 0;
		}

		result = process_transp(direction, iface, ip_hdr->ip_p, ether_hdr, ip_hdr, pointer, buffer_len, parent);
		if (result != FILTER_ALLOW)
			goto done;
	}

	result = FILTER_ALLOW;

done:
	if (result == FILTER_ALLOW)		// give packet to child filter
		return child->process_packet(direction, iface, packet, child, packet_unchanged);
	else
		return FALSE;				// don't send original packet
}

int
process_ether(int direction, int iface, struct ether_hdr *ether_hdr, struct filter_nfo *parent)
{
#define PRINT_ETH_ADDR(addr) \
	(addr)[0], (addr)[1], (addr)[2], (addr)[3], (addr)[4], (addr)[5]

	KdPrint(("[ndis_flt] eth %02x-%02x-%02x-%02x-%02x-%02x -> %02x-%02x-%02x-%02x-%02x-%02x (0x%x)\n",
		PRINT_ETH_ADDR(ether_hdr->ether_shost),
		PRINT_ETH_ADDR(ether_hdr->ether_dhost),
		ether_hdr->ether_type));

	// TODO: add ethernet-level filtering here

	return FILTER_ALLOW;
}

int
process_ip(int direction, int iface, struct ip_hdr *ip_hdr, struct filter_nfo *parent)
{
#define PRINT_IP_ADDR(addr) \
	((UCHAR *)&(addr))[0], ((UCHAR *)&(addr))[1], ((UCHAR *)&(addr))[2], ((UCHAR *)&(addr))[3]

	KdPrint(("[ndis_flt] ip %d.%d.%d.%d -> %d.%d.%d.%d (proto = %d)\n",
		PRINT_IP_ADDR(ip_hdr->ip_src),
		PRINT_IP_ADDR(ip_hdr->ip_dst),
		ip_hdr->ip_p));

	// TODO: add ICMP destination host unreachable sending on FILTER_DENY

	return filter_ip(direction, iface, ip_hdr);
}

int
process_tcp(int direction, int iface, struct ether_hdr *eth_hdr, struct ip_hdr *ip_hdr, struct tcp_hdr *tcp_hdr,
			struct filter_nfo *parent)
{
	int result;

#if DBG
	char flags[7];

	flags[0] = (tcp_hdr->th_flags & TH_FIN) ? 'F' : ' ';
	flags[1] = (tcp_hdr->th_flags & TH_SYN) ? 'S' : ' ';
	flags[2] = (tcp_hdr->th_flags & TH_RST) ? 'R' : ' ';
	flags[3] = (tcp_hdr->th_flags & TH_PUSH) ? 'P' : ' ';
	flags[4] = (tcp_hdr->th_flags & TH_ACK) ? 'A' : ' ';
	flags[5] = (tcp_hdr->th_flags & TH_URG) ? 'U' : ' ';
	flags[6] = '\0';

	KdPrint(("[ndis_flt] tcp %d -> %d (%s)\n",
		ntohs(tcp_hdr->th_sport),
		ntohs(tcp_hdr->th_dport),
		flags));
#endif

	result = filter_tcp(direction, iface, ip_hdr, tcp_hdr);
	if (result == FILTER_DENY && !(tcp_hdr->th_flags & TH_RST)) {
		/* send TCP RST */
		
		char rst_packet[sizeof(struct ether_hdr) + sizeof(struct ip_hdr) + sizeof(struct tcp_hdr)];
		struct ether_hdr *eth_rst = (struct ether_hdr *)rst_packet;
		struct ip_hdr *ip_rst = (struct ip_hdr *)(eth_rst + 1);
		struct tcp_hdr *tcp_rst = (struct tcp_hdr *)(ip_rst + 1);
		struct pseudo_tcp_hdr *pseudo_tcp_rst = (struct pseudo_tcp_hdr *)tcp_rst - 1;

		memset(rst_packet, 0, sizeof(rst_packet));

		// make TCP header (source <-> destination)
		tcp_rst->th_sport = tcp_hdr->th_dport;
		tcp_rst->th_dport = tcp_hdr->th_sport;
		tcp_rst->th_seq = 0;
		tcp_rst->th_ack = htonl(ntohl(tcp_hdr->th_seq) + 1);
		tcp_rst->th_off = sizeof(*tcp_rst) / 4;
		tcp_rst->th_flags = TH_ACK | TH_RST;
		tcp_rst->th_win = 0;
		tcp_rst->th_sum = 0;
		tcp_rst->th_urp = 0;

		// make TCP pseudopacket to calculate checksum
		pseudo_tcp_rst->pth_src = ip_hdr->ip_dst;
		pseudo_tcp_rst->pth_dst = ip_hdr->ip_src;
		pseudo_tcp_rst->pth_zero = 0;
		pseudo_tcp_rst->pth_protocol = IPPROTO_TCP;
		pseudo_tcp_rst->pth_length = htons(sizeof(*tcp_rst));

		// calculate TCP checksum
		tcp_rst->th_sum = in_cksum((USHORT *)pseudo_tcp_rst, sizeof(*pseudo_tcp_rst) + sizeof(*tcp_rst));

		// make IP header (source <-> destination)
		ip_rst->ip_v = 4;
		ip_rst->ip_hl = sizeof(*ip_rst) / 4;
		ip_rst->ip_tos = ip_hdr->ip_tos;
		ip_rst->ip_len = htons(sizeof(struct ip_hdr) + sizeof(struct tcp_hdr));
		ip_rst->ip_id = 0;			// XXX make it something meaningful
		ip_rst->ip_off = 0;
		ip_rst->ip_ttl = 128;		// XXX make it equal to tcpip TTL from registry
		ip_rst->ip_p = IPPROTO_TCP;
		ip_rst->ip_sum = 0;			// for checksum calculation
		ip_rst->ip_src = ip_hdr->ip_dst;
		ip_rst->ip_dst = ip_hdr->ip_src;

		// calculate IP checksum
		ip_rst->ip_sum = in_cksum((USHORT *)ip_rst, sizeof(*ip_rst));

		// make ethernet header (source <-> destination)
		memcpy(eth_rst->ether_dhost, eth_hdr->ether_shost, sizeof(eth_rst->ether_dhost));
		memcpy(eth_rst->ether_shost, eth_hdr->ether_dhost, sizeof(eth_rst->ether_shost));
		eth_rst->ether_type = eth_hdr->ether_type;

		// send this packet back!

		send_packet(
			direction == DIRECTION_IN ? DIRECTION_OUT : DIRECTION_IN,
			iface,
			rst_packet,
			sizeof(rst_packet),
			parent);

	}
	
	return result;
}

int
process_udp(int direction, int iface, struct ip_hdr *ip_hdr, struct udp_hdr *udp_hdr,
			struct filter_nfo *parent)
{
	KdPrint(("[ndis_flt] udp %d -> %d\n", ntohs(udp_hdr->uh_sport), ntohs(udp_hdr->uh_dport)));

	// TODO: add ICMP destination port unreachable sending on FILTER_DENY

	return filter_udp(direction, iface, ip_hdr, udp_hdr);
}

int
process_icmp(int direction, int iface, struct ip_hdr *ip_hdr, struct icmp_hdr *icmp_hdr,
			 struct filter_nfo *parent)
{
	int result;

	KdPrint(("[ndis_flt] icmp (%d.%d)\n", icmp_hdr->icmp_type, icmp_hdr->icmp_code));

	// TODO: what can we send on FILTER_DENY (host unreachable?)

	return filter_icmp(direction, iface, ip_hdr, icmp_hdr);
}

/* process TCP, UDP or ICMP header */
int
process_transp(int direction, int iface, UCHAR proto, struct ether_hdr *eth_hdr,
			   struct ip_hdr *ip_hdr, char *pointer, UINT buffer_len, struct filter_nfo *parent)
{
	struct tcp_hdr *tcp_hdr;
	struct udp_hdr *udp_hdr;
	struct icmp_hdr *icmp_hdr;

	switch (proto) {
	case IPPROTO_TCP:

		/* process tcp_hdr */

		if (buffer_len < sizeof(struct tcp_hdr)) {
			KdPrint(("[ndis_flt] filter_packet: too small buffer for tcp_hdr! (%u)\n",
				buffer_len));
			return FILTER_UNKNOWN;
		}

		tcp_hdr = (struct tcp_hdr *)pointer;

		return process_tcp(direction, iface, eth_hdr, ip_hdr, tcp_hdr, parent);

	case IPPROTO_UDP:

		/* process udp_hdr */

		if (buffer_len < sizeof(struct udp_hdr)) {
			KdPrint(("[ndis_flt] filter_packet: too small buffer for udp_hdr! (%u)\n",
				buffer_len));
			return FILTER_UNKNOWN;
		}

		udp_hdr = (struct udp_hdr *)pointer;

		return process_udp(direction, iface, ip_hdr, udp_hdr, parent);

	case IPPROTO_ICMP:
		
		/* process udp_hdr */

		if (buffer_len < sizeof(struct icmp_hdr)) {
			KdPrint(("[ndis_flt] filter_packet: too small buffer for icmp_hdr! (%u)\n",
				buffer_len));
			return FILTER_UNKNOWN;
		}

		icmp_hdr = (struct icmp_hdr *)pointer;

		return process_icmp(direction, iface, ip_hdr, icmp_hdr, parent);

	default:
		return FILTER_UNKNOWN;
	}

	/* UNREACHED */
}

// calculate internet checksum
USHORT
in_cksum(USHORT *buffer, int size)
{
    ULONG cksum = 0;

    while (size > 1) {
        cksum += *buffer++;
        size -= sizeof(USHORT);
    }
    if (size != 0)
		cksum += *(UCHAR *)buffer;

    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >> 16);

    return (USHORT)(~cksum);
}

// prepare NDIS_PACKET and send it to parent in filter stack
void
send_packet(int direction, int iface, char *packet_data, ULONG packet_size, struct filter_nfo *parent)
{
	NDIS_PACKET fake_packet;
	PMDL fake_buffer;			// NDIS_BUFFER is equal to MDL

	// create fake NDIS_PACKET and NDIS_BUFFER structures and call parent->process_packet

	fake_buffer = IoAllocateMdl(packet_data, packet_size, FALSE, FALSE, NULL);
	if (fake_buffer == NULL)
		return;

	MmBuildMdlForNonPagedPool(fake_buffer);		// is it correct for buffers in stack?

	memset(&fake_packet, 0, sizeof(fake_packet));
	fake_packet.Private.Head = fake_buffer;

	parent->process_packet(direction, iface, &fake_packet, parent, FALSE);

	// now we can safely free our fake_packet and fake_buffer
	IoFreeMdl(fake_buffer);
}
