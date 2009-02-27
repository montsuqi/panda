# PANDA -- a simple transaction monitor
# Copyright (C) 2005-2008  ogochan
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA
## Process this file with automake to produce Makefile.in

sbin_PROGRAMS = wfc
bin_PROGRAMS = wfccontrol

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	$(SSL_CFLAGS)			\
	$(GLIB_CFLAGS)			\
	$(MONDAI_CFLAGS)

LDADD 			=						\
	-lm									\
	$(top_builddir)/libs/libpanda.a		\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a	\
	$(top_builddir)/libs/libpandablob.a	\
	$(SSL_LIBS)							\
	$(GLIB_LIBS)						\
	$(MONDAI_LIBS)						
DEPENDENCIES	=						\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/libs/libpanda.a		\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a	\
	$(top_builddir)/libs/libpandablob.a


wfc_LDFLAGS	=			\
	-export-dynamic
wfc_SOURCES	=							\
	wfc.c		wfc.h						\
	mqthread.c		mqthread.h			\
	corethread.c	corethread.h		\
	blobthread.c	blobthread.h		\
	controlthread.c	controlthread.h		\
	blobserv.c		blobserv.h			\
	termthread.c	termthread.h

wfccontrol_LDFLAGS	=			\
	-export-dynamic
wfccontrol_SOURCES	=					\
	wfccontrol.c						\
	controlthread.h

