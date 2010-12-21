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
INCLUDES += -I$(top_srcdir)/src/api -I$(top_srcdir)/src/common -I$(top_srcdir)/src/main -I$(top_srcdir)/src/tedbManager -I$(top_srcdir)/src/resvManager -I$(top_srcdir)/src/policyManager

DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\"

INSTALL_SDATA=@INSTALL@ -m 600

# -- Libraries Sources
libapi_a_SOURCES = api.hh
libapi_a_SOURCES += api.cc

distclean:
	-rm -rf Makefile Makefile.in


