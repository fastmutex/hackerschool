// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: log.c,v 1.2 2003/05/13 12:47:59 dev Exp $

#include <ntddk.h>
#include <stdarg.h>
#include <stdio.h>

#include "config.h"
#include "log.h"
#include "memtrack.h"
#include "nt.h"
#include "time.h"

static HANDLE g_log_file = NULL;

/* async logging queue */

#define ASYNC_STRING_SIZE	128

struct queue_entry {
	char	string[ASYNC_STRING_SIZE];
	ULONG	log_skipped;
};

static struct {
	struct		queue_entry *data;
	KSPIN_LOCK	guard;
	ULONG		head;	/* write to head */
	ULONG		tail;	/* read from tail */
	HANDLE		file;
	HANDLE		write_thread;
	KEVENT		write_event;
	BOOLEAN		b_exit;
} g_queue;

static void		logger_thread(PVOID param);


NTSTATUS
open_log(void)
{
	static char begin_str[] = "--- begin ---\r\n";

	wchar_t log_name[1024];
	UNICODE_STRING name;
	OBJECT_ATTRIBUTES oa;
	LARGE_INTEGER time1, time2, offset;
	SYSTEMTIME time3;
	NTSTATUS status;
	IO_STATUS_BLOCK isb;

	KeQuerySystemTime(&time1);
	ExSystemTimeToLocalTime(&time1, &time2);
	KernelTimeToSystemTime(&time2, &time3);

	if (_snwprintf(log_name, sizeof(log_name), L"%s%04d%02d%02d.log", LOG_NAME,
		time3.wYear, time3.wMonth, time3.wDay) == -1) {
		KdPrint(("[ndis_flt] log_str: name is too long!\n"));
		return STATUS_BUFFER_OVERFLOW;
	}

	RtlInitUnicodeString(&name, log_name);
	InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE, NULL, NULL);

	status = ZwCreateFile(&g_log_file,  FILE_APPEND_DATA, &oa, &isb, 0, FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ, FILE_OPEN_IF, 0, NULL, 0);
	if (status != STATUS_SUCCESS) {
		KdPrint(("[ndis_flt] log_str: ZwCreateFile: 0x%x\n", status));
		return status;
	}

	// write begin of log
	offset.QuadPart = 0;

	ZwWriteFile(g_log_file, NULL, NULL, NULL, &isb, begin_str, sizeof(begin_str) - 1, &offset, NULL);

	/* init async logging queue */

	KeInitializeSpinLock(&g_queue.guard);
	KeInitializeEvent(&g_queue.write_event, SynchronizationEvent, FALSE);

	g_queue.data = (struct queue_entry *)malloc_np(sizeof(struct queue_entry) * LOG_QUEUE_SIZE);
	if (g_queue.data == NULL) {
		KdPrint(("[ndis_flt] filter_init: malloc_np!\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	memset(g_queue.data, 0, sizeof(struct queue_entry) * LOG_QUEUE_SIZE);

	g_queue.head = g_queue.tail = 0;
	g_queue.file = NULL;

	// create worker thread
	status = PsCreateSystemThread(&g_queue.write_thread, THREAD_ALL_ACCESS, NULL, NULL, NULL,
		logger_thread, NULL);
	if (status != STATUS_SUCCESS) {
		KdPrint(("[ndis_flt] filter_init: PsCreateSystemThread!\n"));

		free(g_queue.data);
		return status;
	}

	return STATUS_SUCCESS;
}

void
close_log(void)
{
	static char end_str[] = "--- end ---\r\n";

	IO_STATUS_BLOCK isb;
	LARGE_INTEGER offset;
	HANDLE log_file;
	KIRQL irql;

	if (g_log_file == NULL)
		return;

	/* terminate logger_thread */
	g_queue.b_exit = TRUE;
	KeSetEvent(&g_queue.write_event, 0, FALSE);
	ZwWaitForSingleObject(g_queue.write_thread, FALSE, NULL);
	ZwClose(g_queue.write_thread);

	/* clear logger queue */
	KeAcquireSpinLock(&g_queue.guard, &irql);
	free(g_queue.data);
	KeReleaseSpinLock(&g_queue.guard, irql);

	log_file = g_log_file;
	g_log_file = NULL;

	// write end of log
	offset.QuadPart = 0;
	ZwWriteFile(log_file, NULL, NULL, NULL, &isb, end_str, sizeof(end_str) - 1, &offset, NULL);

	// close log file

	ZwClose(log_file);
}

void
log_str(const char *fmt, ...)
{
	static ULONG event_number = 1;

	char msg1[256], msg2[256 + sizeof("#4294967295 yyyy.mm.dd hh:mm:ss.mss\r\n")];
	va_list ap;
	NTSTATUS status;
	LARGE_INTEGER time1, time2;
	SYSTEMTIME time3;

	va_start(ap, fmt);

	if (_vsnprintf(msg1, sizeof(msg1), fmt, ap) == -1)
		msg1[sizeof(msg1) - 1] = '\0';
	
	va_end(ap);

	// prepare msg2 with event number and timestamp

	KeQuerySystemTime(&time1);
	ExSystemTimeToLocalTime(&time1, &time2);
	KernelTimeToSystemTime(&time2, &time3);

	if (_snprintf(msg2, sizeof(msg2), "%010u %04d.%02d.%02d %02d:%02d:%02d.%03d\t%s\r\n", 
		event_number++,
		time3.wYear, time3.wMonth, time3.wDay,
		time3.wHour, time3.wMinute, time3.wSecond, time3.wMilliseconds,
		msg1) == -1)
		msg2[sizeof(msg2) - 1] = '\0';

	KdPrint(("[ndis_flt] log_str: %s", msg2));

	if (g_log_file != NULL) {
		LARGE_INTEGER offset;
		IO_STATUS_BLOCK isb;
	
		offset.QuadPart = 0;

		status = ZwWriteFile(g_log_file, NULL, NULL, NULL, &isb, msg2, strlen(msg2), &offset, NULL);
		if (status != STATUS_SUCCESS)
			KdPrint(("[ndis_flt] log_str: ZwWriteFile: 0x%x\n", status));
	}
}

void
log_str_dispatch(const char *fmt, ...)
{
	struct queue_entry entry;
	va_list ap;
	KIRQL irql;
	ULONG next_head;

	// prepare string

	va_start(ap, fmt);

	if (_vsnprintf(entry.string, sizeof(entry.string), fmt, ap) == -1)
		entry.string[sizeof(entry.string) - 1] = '\0';
	
	va_end(ap);

	// write it to queue

	KeAcquireSpinLock(&g_queue.guard, &irql);

	next_head = (g_queue.head + 1) % LOG_QUEUE_SIZE;
	
	if (next_head == g_queue.tail) {
		// queue overflow: reject one entry from tail
		entry.log_skipped = g_queue.data[g_queue.tail].log_skipped + 1;
		g_queue.tail = (g_queue.tail + 1) % LOG_QUEUE_SIZE;
	} else
		entry.log_skipped = 0;

	memcpy(&g_queue.data[g_queue.head], &entry, sizeof(struct queue_entry));

	g_queue.head = next_head;

	KeReleaseSpinLock(&g_queue.guard, irql);

	KeSetEvent(&g_queue.write_event, IO_NO_INCREMENT, FALSE);
}

void
logger_thread(PVOID param)
{
	KIRQL irql;
	struct queue_entry entry;
	int has_request;

	for (;;) {
		KeWaitForSingleObject(&g_queue.write_event, Executive, KernelMode, FALSE, NULL);

		if (g_queue.b_exit)
			break;

		for (;;) {
			has_request = 0;

			// enter DISPATCH level
			KeAcquireSpinLock(&g_queue.guard, &irql);

			if (g_queue.head != g_queue.tail) {
				memcpy(&entry, &g_queue.data[g_queue.tail], sizeof(entry));
				has_request = 1;

				g_queue.tail = (g_queue.tail + 1) % LOG_QUEUE_SIZE;
			}

			KeReleaseSpinLock(&g_queue.guard, irql);
			// we're on PASSIVE level

			if (!has_request)
				break;

			if (entry.log_skipped != 0)
				log_str("SKIP\t%u", entry.log_skipped);


			// write entry to file
			log_str("%s", entry.string);
		}
	}
}
