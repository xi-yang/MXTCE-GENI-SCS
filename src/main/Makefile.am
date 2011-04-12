## Process this file with automake to produce Makefile.in.
## MX-TCE: mxtce-sw/src/main/Makefile.am

# -- Programs
bin_PROGRAMS = mxtce mxtce_test

# -- Libraries and library flags to link the binary programs
LDADD = $(top_srcdir)/src/common/libcommon.a $(top_srcdir)/src/api/libapi.a $(top_srcdir)/src/compute/libcompute.a
LDADD += $(top_srcdir)/src/tedbManager/libtedb.a $(top_srcdir)/src/resvManager/librm.a $(top_srcdir)/src/policyManager/libpm.a

# -- Including headers
INCLUDES = -I. -I/usr/include -I/usr/local/include -I/usr/include/libxml2 -I/usr/local/include/libxml2
INCLUDES += -I$(top_srcdir)/src/api -I$(top_srcdir)/src/common -I$(top_srcdir)/src/compute -I$(top_srcdir)/src/main -I$(top_srcdir)/src/tedbManager -I$(top_srcdir)/src/resvManager -I$(top_srcdir)/src/policyManager


DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\"

INSTALL_SDATA=@INSTALL@ -m 600

# -- Program Sources
mxtce_SOURCES = mxtce.hh mxtce_config.hh
mxtce_SOURCES += mxtce.cc mxtce_config.cc mxtce_main.cc

# -- Program Sources
mxtce_test_SOURCES = mxtce.hh mxtce_config.hh
mxtce_test_SOURCES += mxtce.cc mxtce_config.cc mxtce_test.cc
