## Process this file with automake to produce Makefile.in.
## MX-TCE: mxtce-sw/src/geni/Makefile.am

# -- Programs
bin_PROGRAMS = 

# -- No-install Programs
#check_PROGRAMS  = geni_test

# -- No-install Libraries
noinst_LIBRARIES = libgeni.a

# -- Libraries and library flags to link the binary programs
LDADD = $(noinst_LIBRARIES)

# -- Including headers
INCLUDES = -I. -I/usr/include -I/usr/local/include -I/usr/include/libxml2 -I/usr/local/include/libxml2
INCLUDES += -I$(top_srcdir)/src/api -I$(top_srcdir)/src/common -I$(top_srcdir)/src/compute -I$(top_srcdir)/src/main -I$(top_srcdir)/src/tedbManager -I$(top_srcdir)/src/resvManager -I$(top_srcdir)/src/policyManager -I$(top_srcdir)/src/geni


DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\"

INSTALL_SDATA=@INSTALL@ -m 600

# -- No-install Program Sources
#geni_test_SOURCES = geni_test.cc 

# -- Library Sources
libgeni_a_SOURCES  = xmlrpc_apiserver.hh rspec.hh
libgeni_a_SOURCES  += xmlrpc_apiserver.cc rspec.cc
