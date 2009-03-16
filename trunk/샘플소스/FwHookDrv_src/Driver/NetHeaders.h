/*

  NetHeaders.h

  Autor: Jes? O.
  Last Updated: 15/02/2003


*/

#ifndef _NetHeaders_h_
#define _NetHeaders_h_


#pragma pack(1)


#define	IP_DF 0x4000			// dont fragment flag
#define	IP_MF 0x2000			// more fragments flag
#define	IP_OFFMASK 0x1fff		// mask for fragmenting bits

enum
  {
    IPPROTO_IP					= 0,		// Dummy protocol for TCP.
    IPPROTO_HOPOPTS				= 0,		// IPv6 Hop-by-Hop options.  */
    IPPROTO_ICMP				= 1,		// Internet Control Message Protocol.  */
    IPPROTO_IGMP				= 2,		// Internet Group Management Protocol. */
    IPPROTO_IPIP				= 4,		// IPIP tunnels (older KA9Q tunnels use 94).  */
    IPPROTO_TCP					= 6,		// Transmission Control Protocol.  */
    IPPROTO_EGP					= 8,		// Exterior Gateway Protocol.  */
    IPPROTO_PUP					= 12,		//	PUP protocol.  */
    IPPROTO_UDP					= 17,		//	User Datagram Protocol.  */
    IPPROTO_IDP					= 22,		//  XNS IDP protocol.  */
    IPPROTO_TP					= 29,		//  SO Transport Protocol Class 4.  */
    IPPROTO_IPV6				= 41,		//	IPv6 header.  */
    IPPROTO_ROUTING				= 43,		//	IPv6 routing header.  */
    IPPROTO_FRAGMENT			= 44,		//	IPv6 fragmentation header.  */
    IPPROTO_RSVP				= 46,		//  Reservation Protocol.  */
    IPPROTO_GRE					= 47,		//  General Routing Encapsulation.  */
    IPPROTO_ESP					= 50,		// encapsulating security payload.  */
    IPPROTO_AH					= 51,		// authentication header.  */
    IPPROTO_ICMPV6				= 58,		// ICMPv6.  */
    IPPROTO_NONE				= 59,		/* IPv6 no next header.  */
    IPPROTO_DSTOPTS				= 60,		/* IPv6 destination options.  */
    IPPROTO_MTP					= 92,		/* Multicast Transport Protocol.  */
    IPPROTO_ENCAP				= 98,		/* Encapsulation Header.  */
    IPPROTO_PIM					= 103,		/* Protocol Independent Multicast.  */
    IPPROTO_COMP				= 108,		/* Compression Header Protocol.  */
    IPPROTO_RAW					= 255,		/* Raw IP packets.  */
    IPPROTO_MAX
  };


typedef struct _IPHeader 
{
	UCHAR	headerLength:4;	// Header length 
	UCHAR	version:4;		// Version 
	UCHAR	tos;			// Type of service 
	USHORT	length;			// Total length 
	USHORT	id;				// Identification 
	USHORT	offset;			// Fragment offset field
	UCHAR	ttl;			// Time to live 
	UCHAR	protocol;		// Protocol 
	USHORT	checksum;		// Checksum 
	ULONG	source;			// Source address 
	ULONG	destination;	// Destination address 
}IPHeader, *PIPHeader;


typedef struct _ICMPHeader
{
	UCHAR	type;			// Type of message 
	UCHAR	code;			// Code
	USHORT	checksum;		// Checksum
}ICMPHeader, *PICMPHeader;


typedef struct _UDPHeader
{
	USHORT	sourcePort;			// Source port
	USHORT	destinationPort;	// Destination port
	USHORT	length;				// Length
	USHORT	checksum;			// Checksum
}UDPHeader, *PUDPHeader;


// TCP Flags
#define	TH_FIN	0x01
#define	TH_SYN	0x02
#define	TH_RST	0x04
#define	TH_PUSH	0x08
#define	TH_ACK	0x10
#define	TH_URG	0x20

typedef struct _TCPHeader
{
	USHORT	sourcePort;			// Source Port
	USHORT	destinationPort;	// Destination Port
	ULONG	nSequence;			// Sequence number
	ULONG	nAck;				// Acknowledgement number

	UCHAR	unused:4;			// Unused
	UCHAR	offset:4;			// Data offset
	UCHAR	flags;				// Flags

	USHORT	window;				// Window size
	USHORT	checksum;			// Checksum
	USHORT	urp;				// Urgent Pointer
}TCPHeader, *PTCPHeader;

#pragma pack()

#endif
