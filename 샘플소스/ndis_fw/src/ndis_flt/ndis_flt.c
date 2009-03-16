// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: ndis_flt.c,v 1.6 2003/05/21 12:24:57 dev Exp $

#include <ntddk.h>
#include <ndis.h>

#include "filter.h"
#include "log.h"
#include "memtrack.h"
#include "ndis_hk_ioctl.h"

/* prototypes */

static NTSTATUS	DeviceDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp);
static VOID		OnUnload(IN PDRIVER_OBJECT DriverObject);

static NTSTATUS	get_adapters_list(void);
static NTSTATUS	get_iface(void);

static NTSTATUS	log_description(wchar_t *device_name, int n);

/* globals */

static PDEVICE_OBJECT g_ndis_hk_devobj;
static PFILE_OBJECT g_ndis_hk_fileobj;

// interface of ndis_hk
struct ndis_hk_interface *g_ndis_hk;

static struct filter_nfo g_ndis_flt = {
	filter_packet,
	NULL,
	NULL,
	{NULL, NULL, NULL, NULL}
};

/* initialization */
NTSTATUS
DriverEntry(IN PDRIVER_OBJECT theDriverObject,
            IN PUNICODE_STRING theRegistryPath)
{
	NTSTATUS status;
	UNICODE_STRING devname;
	int i;

 	memtrack_init();

	status = open_log();
	if (status != STATUS_SUCCESS) {
		KdPrint(("[ndis_flt] DriverEntry: open_log: 0x%x\n", status));
		goto done;
	}
	
	status = filter_init();
	if (status != STATUS_SUCCESS) {
		KdPrint(("[ndis_flt] DriverEntry: filter_init: 0x%x\n", status));
		goto done;
	}

	// register UnLoad procedure
	theDriverObject->DriverUnload = OnUnload;

	// connect with ndis_hk
	RtlInitUnicodeString(&devname, L"\\Device\\ndis_hk");
	
	status = IoGetDeviceObjectPointer(
		&devname,
		STANDARD_RIGHTS_ALL,
		&g_ndis_hk_fileobj,
		&g_ndis_hk_devobj);
	if (status != STATUS_SUCCESS) {
		KdPrint(("[ndis_flt] DriverEntry: IoGetDeviceObjectPointer: 0x%x\n"));
		goto done;
	}

	status = get_iface();
	if (status != STATUS_SUCCESS)
		KdPrint(("[ndis_flt] DriverEntry: get_iface: 0x%x!\n", status));

	status = get_adapters_list();
	if (status != STATUS_SUCCESS) {
		KdPrint(("[ndis_flt] DriverEntry: get_adapters_list: 0x%x!\n", status));
		goto done;
	}

	// attach our filter!
	g_ndis_hk->attach_filter(&g_ndis_flt, TRUE, FALSE);	// to bottom of filter stack

done:
	// log starting (if we can)
	if (status == STATUS_SUCCESS)
		log_str("DRIVER\tstarted OK");
	else
		log_str("DRIVER\terror starting driver 0x%x", status);
	
	if (status != STATUS_SUCCESS) {
		// cleanup
		OnUnload(theDriverObject);
	}

	return status;
}

/* Unload */
VOID
OnUnload(IN PDRIVER_OBJECT DriverObject)
{
	if (g_ndis_hk_fileobj != NULL) {
		// detach our filter!
		if (g_ndis_hk != NULL)
			g_ndis_hk->attach_filter(&g_ndis_flt, FALSE, FALSE);

		ObDereferenceObject(g_ndis_hk_fileobj);
	}
	
	filter_free();
	close_log();

	memtrack_free();
}

/* get ndis_hk interface */
NTSTATUS
get_iface(void)
{
	PIRP irp;
	IO_STATUS_BLOCK isb;

	irp = IoBuildDeviceIoControlRequest(IOCTL_CMD_GET_KM_IFACE,
		g_ndis_hk_devobj,
		NULL, 0,
		&g_ndis_hk, sizeof(g_ndis_hk),
		TRUE, NULL, &isb);
	if (irp == NULL) {
		KdPrint(("[ndis_flt] get_iface: IoBuildDeviceIoControlRequest!\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	return IoCallDriver(g_ndis_hk_devobj, irp);
}

/* get adapters list from ndis_hk and log description for each of them */
NTSTATUS
get_adapters_list(void)
{
	wchar_t *buf = NULL, *p;
	int buf_size, i;
	NTSTATUS status;

	// first get buffer size

	buf_size = g_ndis_hk->get_adapter_list(NULL, 0);
	if (buf_size <= 1) {
		KdPrint(("[ndis_flt] get_adapters_list: adapters list is empty!"));

		status = STATUS_SUCCESS;
		goto done;
	}

	// allocate buffer for query
	
	buf = (wchar_t *)malloc_np(buf_size * sizeof(wchar_t));
	if (buf == NULL) {
		KdPrint(("[ndis_flt] get_adapters_list: malloc_np!\n"));
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto done;
	}

	// query it!

	g_ndis_hk->get_adapter_list(buf, buf_size);

	// and enumerate it
	for (i = 1, p = buf; *p != L'\0'; p += wcslen(p) + 1, i++) {
		KdPrint(("[ndis_flt] get_adapters_list: %S(%d)\n", p, i));

		log_description(p, i);
	
	}

	status = STATUS_SUCCESS;
done:
	// cleanup
	if (buf != NULL)
		free(buf);

	return status;
}

/* log adapter description */
NTSTATUS
log_description(wchar_t *device_name, int n)
{
	static wchar_t net_path[] =
		L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards";

	NTSTATUS status;
	UNICODE_STRING net_key, desc_key, desc_value;
	OBJECT_ATTRIBUTES oa;
	HANDLE hkey_net = NULL, hkey_desc = NULL;
	KEY_BASIC_INFORMATION *basic_info = NULL;
	KEY_VALUE_PARTIAL_INFORMATION *value_info = NULL;
	ULONG basic_len, value_len, len;
	int i;
	wchar_t *name, *desc_path = NULL;

	// skip \\Device\\ in device_name
	if (*device_name == '\\') {
		name = wcschr(device_name + 1, '\\');
		if (name == NULL)
			name = device_name + 1;
		else
			name++;
	} else
		name = device_name;

	// open key with NetworkCards

	RtlInitUnicodeString(&net_key, net_path);
	InitializeObjectAttributes(&oa, &net_key, OBJ_CASE_INSENSITIVE | OBJ_OPENLINK, NULL, NULL);

	status = ZwOpenKey(&hkey_net, KEY_ENUMERATE_SUB_KEYS, &oa);
	if (status != STATUS_SUCCESS) {
		KdPrint(("[ndis_flt] log_description: ZwOpenKey: 0x%x\n", status));
		goto done;
	}

	basic_len = sizeof(*basic_info) + 1024;
	basic_info = (KEY_BASIC_INFORMATION *)malloc_np(basic_len);
	if (basic_info == NULL) {
		KdPrint(("[ndis_flt] log_description: malloc_np!\n"));
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto done;
	}

	value_len = sizeof(*value_info) + 1024;
	value_info = (KEY_VALUE_PARTIAL_INFORMATION *)malloc_np(value_len);
	if (value_info == NULL) {
		KdPrint(("[ndis_flt] log_description: malloc_np!\n"));
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto done;
	}

	// default status: desctiption not found
	status = STATUS_OBJECT_NAME_NOT_FOUND;

	for (i = 0;; i++) {

		status = ZwEnumerateKey(hkey_net, i, KeyBasicInformation, basic_info,
			basic_len - sizeof(wchar_t), &len);
		if (status == STATUS_BUFFER_TOO_SMALL) {
			free(basic_info);

			basic_len = len + sizeof(wchar_t);
			basic_info = (KEY_BASIC_INFORMATION *)malloc_np(basic_len);
			if (basic_info == NULL) {
				KdPrint(("[ndis_flt] log_description: malloc_np!\n"));
				status = STATUS_INSUFFICIENT_RESOURCES;
				goto done;
			}

			status = ZwEnumerateKey(hkey_net, i, KeyBasicInformation, basic_info,
				basic_len - sizeof(wchar_t), &len);
		}
		if (status == STATUS_NO_MORE_ENTRIES)
            break;
		if (status != STATUS_SUCCESS) {
			KdPrint(("[ndis_flt] log_description: ZwEnumerateKey: 0x%x\n", status));
			goto done;
		}

		// terminate string with zero
		*(wchar_t *)(&basic_info->Name[basic_info->NameLength / 2]) = 0;

		KdPrint(("[ndis_flt] log_description: %S\n", basic_info->Name));

		// okay we have subname: prepare subkey and open it
		desc_path = (wchar_t *)malloc_np(sizeof(net_path) + basic_info->NameLength + sizeof(wchar_t));
		if (desc_path == NULL) {
			KdPrint(("[ndis_flt] log_description: malloc_np!\n"));
			status = STATUS_INSUFFICIENT_RESOURCES;
			goto done;
		}

		wcscpy(desc_path, net_path);
		wcscat(desc_path, L"\\");
		wcscat(desc_path, basic_info->Name);

		RtlInitUnicodeString(&desc_key, desc_path);
		InitializeObjectAttributes(&oa, &desc_key, OBJ_CASE_INSENSITIVE | OBJ_OPENLINK, NULL, NULL);

		status = ZwOpenKey(&hkey_desc, KEY_QUERY_VALUE, &oa);
		free(desc_path);

		if (status != STATUS_SUCCESS) {
			KdPrint(("[ndis_flt] log_description: ZwOpenKey: 0x%x\n", status));
			goto done;
		}

		// get ServiceName value

		RtlInitUnicodeString(&desc_value, L"ServiceName");

		status = ZwQueryValueKey(hkey_desc, &desc_value, KeyValuePartialInformation, value_info,
			value_len, &len);
		if (status == STATUS_BUFFER_TOO_SMALL) {
			free(value_info);

			value_len = len;
			value_info = (KEY_VALUE_PARTIAL_INFORMATION *)malloc_np(len);
			if (value_info == NULL) {
				KdPrint(("[ndis_flt] log_description: malloc_np\n"));
				status = STATUS_INSUFFICIENT_RESOURCES;
				goto done;
			}

			status = ZwQueryValueKey(hkey_desc, &desc_value, KeyValuePartialInformation, value_info,
				value_len, &len);
		}
		if (status != STATUS_SUCCESS) {
			KdPrint(("[ndis_flt] log_description: ZwQueryValueKey: 0x%x\n", status));
			goto done;
		}
		if (value_info->Type != REG_SZ) {
			KdPrint(("[ndis_flt] log_description: ServiceName is not REG_SZ\n"));
			goto done;
		}

		// compare ServiceName with name given
		if (wcscmp((wchar_t *)value_info->Data, name) == 0) {

			// get description
			RtlInitUnicodeString(&desc_value, L"Description");

			status = ZwQueryValueKey(hkey_desc, &desc_value, KeyValuePartialInformation, value_info,
				value_len, &len);
			if (status == STATUS_BUFFER_TOO_SMALL) {
				free(value_info);

				value_len = len;
				value_info = (KEY_VALUE_PARTIAL_INFORMATION *)malloc_np(len);
				if (value_info == NULL) {
					KdPrint(("[ndis_flt] log_description: malloc_np\n"));
					status = STATUS_INSUFFICIENT_RESOURCES;
					goto done;
				}

				status = ZwQueryValueKey(hkey_desc, &desc_value, KeyValuePartialInformation, value_info,
					value_len, &len);
			}
			if (status != STATUS_SUCCESS) {
				KdPrint(("[ndis_flt] log_description: ZwQueryValueKey: 0x%x\n", status));
				goto done;
			}
			if (value_info->Type != REG_SZ) {
				KdPrint(("[ndis_flt] log_description: ServiceName is not REG_SZ\n"));
				goto done;
			}

			log_str("ADAPTER\tif:%d\t%S", n, value_info->Data);

			status = STATUS_SUCCESS;
			break;

		}

		ZwClose(hkey_desc);
		hkey_desc = NULL;
	}

	if (status != STATUS_SUCCESS)
		log_str("ADAPTER\tif:%d\t%S", n, name);

	status = STATUS_SUCCESS;

done:
	if (hkey_desc != NULL)
		ZwClose(hkey_desc);
	if (hkey_net != NULL)
		ZwClose(hkey_net);
	if (value_info != NULL)
		free(value_info);
	if (basic_info != NULL)
		free(basic_info);

	return status;
}
