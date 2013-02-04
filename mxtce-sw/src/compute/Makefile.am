## Process this file with automake to produce Makefile.in.
## MX-TCE: mxtce-sw/src/compute/Makefile.am

# -- Programs
bin_PROGRAMS = 

# -- No-install Libraries
noinst_LIBRARIES = libcompute.a

# -- Libraries and library flags to link the binary programs
LDADD = $(noinst_LIBRARIES)

# -- Including headers
INCLUDES = -I. -I/usr/include -I/usr/local/include -I/usr/include/libxml2 -I/usr/local/include/libxml2
INCLUDES += -I$(top_srcdir)/src/api -I$(top_srcdir)/src/common -I$(top_srcdir)/src/geni -I$(top_srcdir)/src/compute -I$(top_srcdir)/src/main -I$(top_srcdir)/src/tedbManager -I$(top_srcdir)/src/resvManager -I$(top_srcdir)/src/policyManager

DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\"

INSTALL_SDATA=@INSTALL@ -m 600

# -- Library Sources
libcompute_a_SOURCES  = action.hh scheduling.hh compute_actions.hh compute_worker.hh simple_worker.hh schedule_worker.hh multip2p_worker.hh
libcompute_a_SOURCES  += action.cc scheduling.cc compute_actions.cc compute_worker.cc simple_worker.cc schedule_worker.cc multip2p_worker.cc
