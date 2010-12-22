## Process this file with automake to produce Makefile.in.
## MX-TCE: mxtce-sw/src/policyManager/Makefile.am

# -- Programs
bin_PROGRAMS = 

# -- No-install Programs
check_PROGRAMS  = policyman_test

# -- No-install Libraries
noinst_LIBRARIES = libpm.a

# -- Libraries and library flags to link the binary programs
LDADD = $(noinst_LIBRARIES)

# -- Including headers
INCLUDES = -I. -I/usr/include -I/usr/local/include -I/usr/include/libxml2 -I/usr/local/include/libxml2
INCLUDES += -I$(top_srcdir)/src/api -I$(top_srcdir)/src/common -I$(top_srcdir)/src/main -I$(top_srcdir)/src/tedbManager -I$(top_srcdir)/src/resvManager -I$(top_srcdir)/src/policyManager



DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\"

INSTALL_SDATA=@INSTALL@ -m 600

# -- No-install Program Sources
policyman_test_SOURCES = policyman_test.cc 

# -- Library Sources
libpm_a_SOURCES  = policyman.hh
libpm_a_SOURCES  += policyman.cc
