// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: filter.c,v 1.5 2003/05/19 15:09:26 dev Exp $

#include <ntddk.h>
#include <ndis.h>

#include "config.h"
#include "filter.h"
#include "flt_rule.h"
#include "log.h"
#include "memtrack.h"
#include "nt.h"
#include "sock.h"

/* globals */

// rules chain
static struct {
	struct		flt_rule *head;
	struct		flt_rule *tail;
	KSPIN_LOCK	guard;
	HANDLE		reload_thread;
	KEVENT		reload_event;
	int			b_exit;
} g_rules;

/* prototypes */

static void		reload_thread(PVOID param);

static NTSTATUS	add_flt_rule(const struct flt_rule *rule);
static void		clear_flt_chain(void);

// init
NTSTATUS
filter_init(void)
{
	NTSTATUS status;

	/* rules chain */
	
	KeInitializeSpinLock(&g_rules.guard);
	KeInitializeEvent(&g_rules.reload_event, NotificationEvent, FALSE);
	g_rules.head = g_rules.tail = NULL;

	// load rules
	filter_reload();

	// create watching rules changes thread
	status = PsCreateSystemThread(&g_rules.reload_thread, THREAD_ALL_ACCESS, NULL, NULL, NULL,
		reload_thread, NULL);
	if (status != STATUS_SUCCESS)
		KdPrint(("[ndis_flt] filter_init: PsCreateSystemThread!\n"));

	return status;
}

void
filter_free(void)
{
	KIRQL irql;

	g_rules.b_exit = 1;

	/* terminate rules reloading thread */
	KeSetEvent(&g_rules.reload_event, 0, FALSE);
	ZwWaitForSingleObject(g_rules.reload_thread, FALSE, NULL);
	ZwClose(g_rules.reload_thread);

	clear_flt_chain();
}

NTSTATUS
filter_reload(void)
{
	NTSTATUS status;
	UNICODE_STRING name;
	OBJECT_ATTRIBUTES oa;
	IO_STATUS_BLOCK isb;
	HANDLE rules_file = NULL;
	char *buf, *p, *p2;
	LARGE_INTEGER offset;
	int n_str;

	clear_flt_chain();

	buf = (char *)ExAllocatePool(PagedPool, MAX_RULES_SIZE);	// PagedPool!
	if (buf == NULL) {
		KdPrint(("[ndis_flt] filter_reload: ExAllocatePool!\n"));
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto done;
	}
	
	// load rules from file
	
	RtlInitUnicodeString(&name, RULES_NAME);
	InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE, NULL, NULL);

	status = ZwCreateFile(&rules_file, FILE_READ_DATA | SYNCHRONIZE, &oa, &isb, NULL, 0,
		FILE_SHARE_READ, FILE_OPEN, 0, NULL, 0);
	if (status != STATUS_SUCCESS)
		goto done;

	offset.QuadPart = 0;

	status = ZwReadFile(rules_file, NULL, NULL, NULL, &isb, buf, MAX_RULES_SIZE - 1, &offset, NULL);
	if (status == STATUS_PENDING) {
		ZwWaitForSingleObject(rules_file, FALSE, NULL);
		status = isb.Status;
	}
	if (status != STATUS_SUCCESS && status != STATUS_END_OF_FILE)
		goto done;

	buf[isb.Information] = '\0';

	n_str = 1;
	p = buf;
	p2 = NULL;

	do {
		struct flt_rule rule;
		
		// find \n
		p2 = strchr(p, '\n');
		if (p2 != NULL) {
			*p2 = '\0';

			// process \r\n
			if (p2 != p && p2[-1] == '\r')
				p2[-1] = '\0';

			p2++;
		}

		if (*p != 0 && *p != '#') {
			// process p string and generate rule

			if (!parse_rule(p, &rule)) {
				log_str("PARSE\terror in line %d", n_str);
	
				// ignore error
			} else {
				// save rule line
				rule.rule_line = n_str;
				// and add rule
				add_flt_rule(&rule);
			}
		}

		p = p2;
		n_str++;
	
	} while(p != NULL);
		
	log_str("RULES\tapplied");
	status = STATUS_SUCCESS;

done:
	if (rules_file != NULL)
		ZwClose(rules_file);
	if (buf != NULL)
		ExFreePool(buf);

	return status;
}

// add rule to rules chain
NTSTATUS
add_flt_rule(const struct flt_rule *rule)
{
	NTSTATUS status;
	struct flt_rule *new_rule;
	KIRQL irql;
	
	KeAcquireSpinLock(&g_rules.guard, &irql);

	new_rule = (struct flt_rule *)malloc_np(sizeof(struct flt_rule));
	if (new_rule == NULL) {
		KdPrint(("[ndis_flt] add_flt_rule: malloc_np\n"));
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto done;
	}

	memcpy(new_rule, rule, sizeof(*new_rule));

	// append
	new_rule->next = NULL;

	if (g_rules.head == NULL) {
		g_rules.head = new_rule;
		g_rules.tail = new_rule;
	} else {
		g_rules.tail->next = new_rule;
		g_rules.tail = new_rule;
	}

	status = STATUS_SUCCESS;

done:
	KeReleaseSpinLock(&g_rules.guard, irql);
	return status;
}

void
clear_flt_chain(void)
{
	struct flt_rule *rule;
	KIRQL irql;
	
	/* rules chain */
	KeAcquireSpinLock(&g_rules.guard, &irql);

	for (rule = g_rules.head; rule;) {
		struct flt_rule *rule2 = rule->next;
		free(rule);
		rule = rule2;
	}

	g_rules.head = NULL;
	g_rules.tail = NULL;

	KeReleaseSpinLock(&g_rules.guard, irql);
}

void
reload_thread(PVOID param)
{
	HANDLE rules_file = NULL;
	UNICODE_STRING name;
	OBJECT_ATTRIBUTES oa;
	FILE_BASIC_INFORMATION info1, info2;
	LARGE_INTEGER timeout;
	NTSTATUS status;
	IO_STATUS_BLOCK isb;
	BOOLEAN b_first = TRUE;

	memset(&info2, 0, sizeof(info2));
	timeout.QuadPart = -5000 * 10000;

	RtlInitUnicodeString(&name, RULES_NAME);
	InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE, NULL, NULL);

	for (;;) {

		// wait 5 sec to update information about file
		KeWaitForSingleObject(&g_rules.reload_event, Executive, KernelMode, FALSE, &timeout);

		if (g_rules.b_exit)
			break;

		status = ZwCreateFile(&rules_file,  FILE_READ_ATTRIBUTES | SYNCHRONIZE, &oa, &isb, 0,
			FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, 0, NULL, 0);
		if (status != STATUS_SUCCESS) {
			KdPrint(("[ndis_flt] reload_thread: NtCreateFile: 0x%x\n"));
			continue;
		}

		status = ZwQueryInformationFile(rules_file, &isb, &info1, sizeof(info1), FileBasicInformation);
		ZwClose(rules_file);

		if (status != STATUS_SUCCESS)
			KdPrint(("[ndis_flt] log_str: ZwQueryInformationFile: 0x%x\n"));
		else {

			if (!b_first) {

				// compare new information with old
				if (memcmp(&info1.LastWriteTime, &info2.LastWriteTime, sizeof(LARGE_INTEGER)) != 0) {
					// not equal - reload rules
					filter_reload();
				}

			} else
				b_first = FALSE;

			// copy new information to old
			memcpy(&info2, &info1, sizeof(info2));
		}
	}
}

#define PRINT_IP_ADDR(addr) \
	((UCHAR *)&(addr))[0], ((UCHAR *)&(addr))[1], ((UCHAR *)&(addr))[2], ((UCHAR *)&(addr))[3]

int
filter_ip(int direction, int iface, struct ip_hdr *ip_hdr)
{
	KIRQL irql;
	struct flt_rule *rule;
	int result = FILTER_ALLOW;

	KeAcquireSpinLock(&g_rules.guard, &irql);

	for (rule = g_rules.head; rule != NULL; rule = rule->next)
		if (rule->rule_type == IP_RULE && rule->direction == direction &&
			(rule->iface == 0 || rule->iface == iface) &&
			((rule->rule.ip.src_addr & rule->rule.ip.src_mask) == (ip_hdr->ip_src & rule->rule.ip.src_mask)) &&
			((rule->rule.ip.dst_addr & rule->rule.ip.dst_mask) == (ip_hdr->ip_dst & rule->rule.ip.dst_mask)) &&
			(!rule->rule.ip.use_ip_proto || rule->rule.ip.ip_proto == ip_hdr->ip_p)) {

			result = rule->result;

			if (rule->log) {
				// log it!
				log_str_dispatch("%s(%d)\tip\t%s\tif:%d\t%d.%d.%d.%d\t%d.%d.%d.%d\t%d",
					result == FILTER_ALLOW ? "ALLOW" : "DENY", rule->rule_line,
					direction == DIRECTION_IN ? "in" : "out",
					iface,
					PRINT_IP_ADDR(ip_hdr->ip_src),
					PRINT_IP_ADDR(ip_hdr->ip_dst),
					ip_hdr->ip_p);
			}

			break;
		}

	KeReleaseSpinLock(&g_rules.guard, irql);
	return result;
}

int
filter_tcp(int direction, int iface, struct ip_hdr *ip_hdr, struct tcp_hdr *tcp_hdr)
{
	KIRQL irql;
	struct flt_rule *rule;
	int result = FILTER_ALLOW;

	KeAcquireSpinLock(&g_rules.guard, &irql);

	for (rule = g_rules.head; rule != NULL; rule = rule->next)
		// can you undertand it? me too :-)
		if (rule->rule_type == TCP_RULE && rule->direction == direction &&
			(rule->iface == 0 || rule->iface == iface) &&
			((rule->rule.tcp.ip.src_addr & rule->rule.tcp.ip.src_mask) == (ip_hdr->ip_src & rule->rule.tcp.ip.src_mask)) &&
			(rule->rule.tcp.src_port1 == 0 || (rule->rule.tcp.src_port2 == 0 ?
				(tcp_hdr->th_sport == rule->rule.tcp.src_port1) : (ntohs(tcp_hdr->th_sport) >= ntohs(rule->rule.tcp.src_port1) && ntohs(tcp_hdr->th_sport) <= ntohs(rule->rule.tcp.src_port2)))) &&
			((rule->rule.ip.dst_addr & rule->rule.ip.dst_mask) == (ip_hdr->ip_dst & rule->rule.ip.dst_mask)) &&
			(rule->rule.tcp.dst_port1 == 0 || (rule->rule.tcp.dst_port2 == 0 ?
				(tcp_hdr->th_dport == rule->rule.tcp.dst_port1) : (ntohs(tcp_hdr->th_dport) >= ntohs(rule->rule.tcp.dst_port1) && ntohs(tcp_hdr->th_dport) <= ntohs(rule->rule.tcp.dst_port2)))) &&
			(rule->rule.tcp.flags_set == 0 || (tcp_hdr->th_flags & rule->rule.tcp.flags_set) != 0) &&
			(rule->rule.tcp.flags_unset == 0 || (tcp_hdr->th_flags & rule->rule.tcp.flags_unset) == 0)) {

			result = rule->result;

			if (rule->log) {
				char flags[7];

				flags[0] = (tcp_hdr->th_flags & TH_FIN) ? 'F' : '-';
				flags[1] = (tcp_hdr->th_flags & TH_SYN) ? 'S' : '-';
				flags[2] = (tcp_hdr->th_flags & TH_RST) ? 'R' : '-';
				flags[3] = (tcp_hdr->th_flags & TH_PUSH) ? 'P' : '-';
				flags[4] = (tcp_hdr->th_flags & TH_ACK) ? 'A' : '-';
				flags[5] = (tcp_hdr->th_flags & TH_URG) ? 'U' : '-';
				flags[6] = '\0';

				// log it!
				log_str_dispatch("%s(%d)\ttcp\t%s\tif:%d\t%d.%d.%d.%d:%d\t%d.%d.%d.%d:%d\t%s",
					result == FILTER_ALLOW ? "ALLOW" : "DENY", rule->rule_line,
					direction == DIRECTION_IN ? "in" : "out",
					iface,
					PRINT_IP_ADDR(ip_hdr->ip_src),
					ntohs(tcp_hdr->th_sport),
					PRINT_IP_ADDR(ip_hdr->ip_dst),
					ntohs(tcp_hdr->th_dport),
					flags);
			}

			break;
		}

	KeReleaseSpinLock(&g_rules.guard, irql);
	return result;
}

int
filter_udp(int direction, int iface, struct ip_hdr *ip_hdr, struct udp_hdr *udp_hdr)
{
	KIRQL irql;
	struct flt_rule *rule;
	int result = FILTER_ALLOW;

	KeAcquireSpinLock(&g_rules.guard, &irql);

	for (rule = g_rules.head; rule != NULL; rule = rule->next)
		// can you undertand it? me too :-)
		if (rule->rule_type == UDP_RULE && rule->direction == direction &&
			(rule->iface == 0 || rule->iface == iface) &&
			((rule->rule.udp.ip.src_addr & rule->rule.udp.ip.src_mask) == (ip_hdr->ip_src & rule->rule.udp.ip.src_mask)) &&
			(rule->rule.udp.src_port1 == 0 || (rule->rule.udp.src_port2 == 0 ?
				(udp_hdr->uh_sport == rule->rule.udp.src_port1) : (ntohs(udp_hdr->uh_sport) >= ntohs(rule->rule.udp.src_port1) && ntohs(udp_hdr->uh_sport) <= ntohs(rule->rule.udp.src_port2)))) &&
			((rule->rule.ip.dst_addr & rule->rule.ip.dst_mask) == (ip_hdr->ip_dst & rule->rule.ip.dst_mask)) &&
			(rule->rule.udp.dst_port1 == 0 || (rule->rule.udp.dst_port2 == 0 ?
				(udp_hdr->uh_dport == rule->rule.udp.dst_port1) : (ntohs(udp_hdr->uh_dport) >= ntohs(rule->rule.udp.dst_port1) && ntohs(udp_hdr->uh_dport) <= ntohs(rule->rule.udp.dst_port2))))) {

			result = rule->result;

			if (rule->log) {
				// log it!
				log_str_dispatch("%s(%d)\tudp\t%s\tif:%d\t%d.%d.%d.%d:%d\t%d.%d.%d.%d:%d",
					result == FILTER_ALLOW ? "ALLOW" : "DENY", rule->rule_line,
					direction == DIRECTION_IN ? "in" : "out",
					iface,
					PRINT_IP_ADDR(ip_hdr->ip_src),
					ntohs(udp_hdr->uh_sport),
					PRINT_IP_ADDR(ip_hdr->ip_dst),
					ntohs(udp_hdr->uh_dport));
			}

			break;
		}

	KeReleaseSpinLock(&g_rules.guard, irql);
	return result;
}

int
filter_icmp(int direction, int iface, struct ip_hdr *ip_hdr, struct icmp_hdr *icmp_hdr)
{
	KIRQL irql;
	struct flt_rule *rule;
	int result = FILTER_ALLOW;

	KeAcquireSpinLock(&g_rules.guard, &irql);

	for (rule = g_rules.head; rule != NULL; rule = rule->next)
		if (rule->rule_type == ICMP_RULE && rule->direction == direction &&
			(rule->iface == 0 || rule->iface == iface) &&
			((rule->rule.icmp.ip.src_addr & rule->rule.icmp.ip.src_mask) == (ip_hdr->ip_src & rule->rule.icmp.ip.src_mask)) &&
			((rule->rule.icmp.ip.dst_addr & rule->rule.icmp.ip.dst_mask) == (ip_hdr->ip_dst & rule->rule.icmp.ip.dst_mask)) &&
			(!rule->rule.icmp.use_type || (icmp_hdr->icmp_type == rule->rule.icmp.type)) &&
			(!rule->rule.icmp.use_code || (icmp_hdr->icmp_code == rule->rule.icmp.code))) {

			result = rule->result;

			if (rule->log) {
				// log it!
				log_str_dispatch("%s(%d)\ticmp\t%s\tif:%d\t%d.%d.%d.%d\t%d.%d.%d.%d\t%d.%d",
					result == FILTER_ALLOW ? "ALLOW" : "DENY", rule->rule_line,
					direction == DIRECTION_IN ? "in" : "out",
					iface,
					PRINT_IP_ADDR(ip_hdr->ip_src),
					PRINT_IP_ADDR(ip_hdr->ip_dst),
					icmp_hdr->icmp_type, icmp_hdr->icmp_code);
			}

			break;
		}

	KeReleaseSpinLock(&g_rules.guard, irql);
	return result;
}
