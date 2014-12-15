## Process this file with automake to produce Makefile.in.
## MX-TCE: mxtce-sw/src/api/Makefile.am

# -- Programs
bin_PROGRAMS = 


# -- No-install Libraries
noinst_LIBRARIES  = libapi.a

# -- Libraries and library flags to link the binary programs
LDADD = $(noinst_LIBRARIES)

# -- Including headers
INCLUDES = -I. -I/usr/include -I/usr/local/include -I/usr/include/libxml2 -I/usr/local/include/libxml2 
INCLUDES += -I$(top_srcdir)/src/api -I$(top_srcdir)/src/common -I$(top_srcdir)/src/geni -I$(top_srcdir)/src/compute -I$(top_srcdir)/src/main -I$(top_srcdir)/src/tedbManager -I$(top_srcdir)/src/resvManager -I$(top_srcdir)/src/policyManager

DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\"

INSTALL_SDATA=@INSTALL@ -m 600

# -- Libraries Sources
libapi_a_SOURCES = api.hh apiserver.hh api_constraint.hh decode_pri.hh reserved_constraint.hh tag_id.hh user_constraint.hh encode_pri.hh reply_encoder.hh request_decoder.hh request_encoder.hh compute_result.hh reply_decoder.hh optional_constraint.hh stornet_constraint.hh 
libapi_a_SOURCES += api.cc apiserver.cc decode_pri.cc request_decoder.cc encode_pri.cc reply_encoder.cc request_encoder.cc reply_decoder.cc

