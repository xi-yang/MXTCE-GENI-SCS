package net.es.oscars.pce.tce;

public class CodeNumber {
	public static final byte ASN_LONG_LEN = (byte)0x80;
	
	public static final byte BOOLEAN = (byte)0x01;
	public static final byte INTEGER_NUM = (byte)0x02;
	public static final byte ENUMERATED_NUM = (byte)0x03;
	public static final byte FLOAT_NUM = (byte)0x04;
	public static final byte DOUBLE_NUM = (byte)0x05;
	public static final byte CHARACTER_STRING = (byte)0x06;
	
    public static final byte PCE_USERCONSTRAINT = (byte)0xF1;
    public static final byte PCE_RESVCONSTRAINT = (byte)0xF2;
    
    public static final byte PCE_REPLY = (byte)0xF8;
	
	public static final byte PCE_GRI = (byte)0x81;
	public static final byte PCE_LOGIN = (byte)0x82;
	public static final byte PCE_LAYER = (byte)0x83;
	public static final byte PCE_SOURCE = (byte)0x84;
	public static final byte PCE_DESTINATION = (byte)0x85;
	public static final byte PCE_BANDWIDTH = (byte)0x86;
	public static final byte PCE_DESCRIPTION = (byte)0x87;
	public static final byte PCE_STARTTIME = (byte)0x88;
	public static final byte PCE_ENDTIME = (byte)0x89;
	public static final byte PCE_PATHSETUPMODEL = (byte)0x8A;
	public static final byte PCE_PATHTYPE = (byte)0x8B;
	public static final byte PCE_SRCVLAN = (byte)0x8C;
	public static final byte PCE_DESTVLAN = (byte)0x8D;
	public static final byte PCE_PATH = (byte)0x8E;
	public static final byte PCE_SRCIPPORT = (byte)0x8F;
	public static final byte PCE_DESTIPPORT = (byte)0x90;
	public static final byte PCE_L3_PROTOCOL = (byte)0x91;
	public static final byte PCE_L3_DSCP = (byte)0x92;
	public static final byte PCE_MPLS_BURSTLIMIT = (byte)0x93;
	public static final byte PCE_MPLS_LSPCLASS = (byte)0x94;

	
	public static final byte PCE_PATH_ID = (byte)0x95;
	public static final byte PCE_HOP_ID = (byte)0x96;
	public static final byte PCE_LINK_ID = (byte)0x97;
	public static final byte PCE_SWITCHINGCAPTYPE = (byte)0x98;
	public static final byte PCE_SWITCHINGENCTYPE = (byte)0x99;
	public static final byte PCE_SWITCHINGVLANRANGEAVAI = (byte)0x9A;
	public static final byte PCE_PATH_LENGTH = (byte)0x9B;
	public static final byte PCE_SWITCHINGVLANRANGESUGG = (byte)0x9C;
	public static final byte PCE_VLANTRANSLATION = (byte)0x9D;
	public static final byte PCE_SWITCHINGVLANRANGEASSI = (byte)0x9E;
	public static final byte PCE_CAPACITY = (byte)0x9F;
	public static final byte PCE_MTU = (byte)0xA0;
	public static final byte PCE_COMPUTE_ERROR = (byte)0xA1;
	public static final byte PCE_REMOTE_LINK = (byte)0xA2;
	public static final byte PCE_MAXRESVCAPACITY = (byte)0xA3;
	public static final byte PCE_MINRESVCAPACITY = (byte)0xA4;
	public static final byte PCE_GRANULARITY = (byte)0xA5;
	public static final byte PCE_TE_METRIC = (byte)0xA6;

	

}
