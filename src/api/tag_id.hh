#ifndef _tag_id_hh_
#define _tag_id_hh_

#define PCE_USERCONSTRAINT 0xF1
#define PCE_RESERVEDCONSTRAINT 0xF2
#define PCE_RESVCONSTRAINT 0xF2
#define PCE_OPTICONSTRAINT_COSCHEDULEREQ 0xF3
#define PCE_MULTIPLE_PATH 0xF4

#define PCE_REPLY 0xF8
#define PCE_REGU_REPLY 0xF9
#define PCE_OPTI_REPLY 0xFA

	
#define PCE_GRI   0x81
#define PCE_LOGIN 0x82
#define PCE_LAYER 0x83
#define PCE_SOURCE 0x84
#define PCE_DESTINATION 0x85
#define PCE_BANDWIDTH 0x86
#define PCE_DESCRIPTION 0x87
#define PCE_STARTTIME 0x88
#define PCE_ENDTIME 0x89
#define PCE_PATHSETUPMODEL 0x8A
#define PCE_PATHTYPE 0x8B
#define PCE_SRCVLAN 0x8C
#define PCE_DESTVLAN 0x8D
#define PCE_PATH 0x8E
#define PCE_SRCIPPORT 0x8F
#define PCE_DESTIPPORT 0x90
#define PCE_L3_PROTOCOL 0x91
#define PCE_L3_DSCP 0x92
#define PCE_MPLS_BURSTLIMIT 0x93
#define PCE_MPLS_LSPCLASS 0x94

//#define PCE_HOP 0x95

#define PCE_PATH_ID 0x95
#define PCE_HOP_ID 0x96
#define PCE_LINK_ID 0x97
#define PCE_SWITCHINGCAPTYPE 0x98
#define PCE_SWITCHINGENCTYPE 0x99
#define PCE_SWITCHINGVLANRANGEAVAI 0x9A
#define PCE_PATH_LENGTH 0x9B
#define PCE_SWITCHINGVLANRANGESUGG 0x9C
#define PCE_VLANTRANSLATION 0x9D
#define PCE_SWITCHINGVLANRANGEASSI 0x9E
#define PCE_CAPACITY 0x9F
#define PCE_MTU 0xA0
#define PCE_COMPUTE_ERROR 0xA1
#define PCE_REMOTE_LINK 0xA2
#define PCE_MAXRESVCAPACITY 0xA3
#define PCE_MINRESVCAPACITY 0xA4
#define PCE_GRANULARITY 0xA5
#define PCE_TE_METRIC 0xA6


#define PCE_OPT_COSCHEDREQID 0xA7
#define PCE_OPT_COSCHREQ_STARTTIME 0xA8
#define PCE_OPT_COSCHREQ_ENDTIME 0xA9
#define PCE_OPT_COSCHREQ_MINBANDWIDTH 0xAA
#define PCE_OPT_COSCHREQ_MAXNUMOFALTPATHS 0xAB
#define PCE_OPT_COSCHREQ_BANDWIDTHAVAIGRAPH 0xAC
#define PCE_OPT_COSCHREQ_CONTIGUOUSVLAN 0xAD
#define PCE_OPT_COSCHREQ_MAXDURATION 0xAE
#define PCE_OPT_COSCHREQ_MAXBANDWIDTH 0xAF
#define PCE_OPT_COSCHREQ_DATASIZEBYTES 0xB0

#define PCE_HOP_END_TAG 0xB1
#define PCE_ALT_PATH_NUM 0xB2
#define PCE_OPT_BAG_BANDWIDTH 0xB3
#define PCE_OPT_BAG_STARTTIME 0xB4
#define PCE_OPT_BAG_ENDTIME 0xB5
#define PCE_PATH_END_TAG 0xB6

#define PCE_CHANNELREPRESENTATION 0xB7
#define PCE_SWITCHINGWAVELENAVAI 0xB8
#define PCE_SWITCHINGWAVELENASSI 0xB9
#define PCE_SWITCHINGWAVELENSUGG 0xBA
#define PCE_WAVELENGTHCONVERSION 0xBB

#define PCE_CONCATENATIONTYPE 0xBC
#define PCE_SWITCHINGTIMESLOTAVAI 0xBD
#define PCE_SWITCHINGTIMESLOTASSI 0xBE
#define PCE_SWITCHINGTIMESLOTSUGG 0xBF
#define PCE_TSIENABLED 0xC0
#define PCE_VCATENABLED 0xC1

#define PCE_VENDORSPECIFICINFO 0xC2
#define PCE_IACD_START 0xC3
#define PCE_LOWERLAYERSWITCHINGTYPE 0xC4
#define PCE_LOWERLAYERENCODINGTYPE 0xC5
#define PCE_UPPERLAYERSWITCHINGTYPE 0xC6
#define PCE_UPPERLAYERENCODINGTYPE 0xC7
#define PCE_MAXADAPTBANDWIDTH 0xC8

#define PCE_NEXTHOP_LENGTH 0xC9
#define PCE_NEXTHOP 0xCA
#define PCE_LIFETIME_NUM 0xCB
#define PCE_LIFETIME 0xCC
#define PCE_LIFETIME_START 0xCD
#define PCE_LIFETIME_END 0xCE
#define PCE_LIFETIME_DUR 0xCF

#define PCE_FLEX_ALT_PATH_NUM 0xD0
#define PCE_OPT_BAG_INFO_NUM 0xD1
#define PCE_OPT_BAG_SEG_NUM	0xD2

#define PCE_OPT_REQ_LINK_BAG 0xD3
#define PCE_OPT_BAG_TYPE 0xD4
#define PCE_OPT_BAG_ID 0xD5


#define PCE_TEST1 0xE0
#define PCE_TEST2 0xE1
#define PCE_TEST3 0xE2
#define PCE_TEST4 0xE3
#define PCE_TEST5 0xE4
	
#define ASN_LONG_LEN 0x80


#define BOOLEAN 0x01
#define INTEGER_NUM 0x02
#define ENUMERATED_NUM 0x03
#define FLOAT_NUM 0x04
#define DOUBLE_NUM 0x05
#define CHARACTER_STRING 0x06

#endif
