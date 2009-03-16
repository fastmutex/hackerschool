/*

  FwHookDrv.H

  Author: Jes? O.
  Last Updated : 12/09/03 
  
*/


//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//
// Device type
#define FILE_DEVICE_FWHOOKDRV  0x00692322


#define FWHOOKDRV_IOCTL_INDEX  0x830


// IOCTLs
#define START_IP_HOOK CTL_CODE(FILE_DEVICE_FWHOOKDRV, FWHOOKDRV_IOCTL_INDEX,METHOD_BUFFERED, FILE_ANY_ACCESS)

#define STOP_IP_HOOK CTL_CODE(FILE_DEVICE_FWHOOKDRV, FWHOOKDRV_IOCTL_INDEX+1, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define ADD_FILTER CTL_CODE(FILE_DEVICE_FWHOOKDRV, FWHOOKDRV_IOCTL_INDEX+2, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define CLEAR_FILTER CTL_CODE(FILE_DEVICE_FWHOOKDRV, FWHOOKDRV_IOCTL_INDEX+3, METHOD_BUFFERED, FILE_ANY_ACCESS)



// Structure to define filter rules
typedef struct filter
{
	USHORT protocol;		// Protocol

	ULONG sourceIp;			// Source Ip
	ULONG destinationIp;	// Destination Ip

	ULONG sourceMask;		// Source Ip mask
	ULONG destinationMask;	// Destination Ip mask

	USHORT sourcePort;		// Source port
	USHORT destinationPort; // Destination port
	
	BOOLEAN drop;			// if TRUE, the packet will be dropped
}IPFilter, *PIPFilter;





