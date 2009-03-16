// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: ndis_hk_ioctl.h,v 1.4 2003/05/13 12:47:39 dev Exp $

/**
 * @file ndis_hk_ioctl.h
 * I/O controls and related definitions for ndis_hk control device
 */

#ifndef _ndis_hk_ioctl_h_
#define _ndis_hk_ioctl_h_

/* device ioctls (for kernel mode use only; internal) */

/** MAGIC number for ndis_hk device (useless) */
#define FILE_DEVICE_NDIS_HK		0x0000c501

/**
 * Get kernel-mode interface of hooking driver.
 * output buffer: struct ndis_hk
 */
#define IOCTL_CMD_GET_KM_IFACE		CTL_CODE(FILE_DEVICE_NDIS_HK, 0x801, METHOD_BUFFERED, FILE_READ_DATA)

/* ndis_hk kernel-mode interface */

enum {
	DIRECTION_IN = 0,		/**< input direction of packets (from network to protocol driver) */
	DIRECTION_OUT = 1		/**< output direction of packets (from protocol driver to network) */
};

/**
 * Main filter function to process input (from upper filter) or output (from lower filter) packet.
 * 
 * return value: FALSE don't pass unchanged packet to receiver
 * if (packet_unchanged == FALSE) function MUST return FALSE
 *
 * if function want to pass the packet to next filter in stack:
 *  for DIRECTION_IN:  self->lower->process_packet(direction, iface, packet, self->lower, packet_unchanged);
 *  for DIRECTION_OUT: self->upper->process_packet(direction, iface, packet, self->upper, packet_unchanged);
 */
typedef BOOLEAN	process_packet_t(
	int direction, int iface, PNDIS_PACKET packet, struct filter_nfo *self,
	BOOLEAN packet_unchanged);

/** Filter information for attach_filter */
struct filter_nfo {
	process_packet_t	*process_packet;	/**< function to process packet */

	struct filter_nfo	*upper;		/**< upper filter (process input) */
	struct filter_nfo	*lower;		/**< lower filter (process output) */

	void				*params[4];		/**< place for caller's static params */
};

#ifndef NDIS_HK_INTERFACE_VER
/** the latest NDIS_HK_INTERFACE_VER */
#	define	NDIS_HK_INTERFACE_VER		1
#endif

/** ndis_hk kernel-mode interface */
struct ndis_hk_interface {
	ULONG		version;			/**< should be NDIS_HK_INTERFACE_VER */

	/**
	 * Get list of adapters
	 * @param	buf			output buffer for adapter names
	 * @param	buf_size	size in wchar_t of buf (can be 0)
	 * @return				number of wchar_t has to be in buffer
	 *						if greater than buf_size only partial information has been copied
	 * @see get_adapter_list
	 */
	int			(*get_adapter_list)(wchar_t *buf, int buf_size);

	/**
	 * Attach or remove filter
	 * @param	flt		information about packet filter
	 * @param	add		TRUE - attach filter; FALSE - remove filter
	 * @param	to_top	TRUE - for attaching to top of stack (useless with add == FALSE)
	 * @see attach_filter
	 */
	void		(*attach_filter)(struct filter_nfo *flt, BOOLEAN add, BOOLEAN to_top);

#if NDIS_HK_INTERFACE_VER > 1
	// add future definitions here...
#endif
};

#endif
