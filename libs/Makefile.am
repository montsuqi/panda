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

lib_LTLIBRARIES = libpanda.la

noinst_PROGRAMS		=		\
	testnet					\
	testblob

libpanda_la_CFLAGS = $(AM_CFLAGS)
libpanda_la_LDFLAGS	=			\
	-version-info $(LTVERSION)	\
	-export-dynamic

libpanda_la_SOURCES	=				\
	BDparser.c		BDparser.h		\
	DBDparser.c		DBDparser.h		\
	DBparser.c		DBparser.h		\
	DDparser.c		DDparser.h		\
	DIparser.c		DIparser.h		\
	Dlex.h							\
	LDparser.c		LDparser.h		\
	SQLlex.c		SQLlex.h		\
	SQLparser.c		SQLparser.h		\
	auth.c			auth.h			\
	authstub.c		authstub.h		\
	blob.c			blob.h			\
	blobreq.c		blobreq.h		\
	comm.c			comm.h			\
	comms.c			comms.h			\
	debug.h							\
	directory.c		directory.h		\
	load.c			load.h			\
	message.c		message.h		\
	mhandler.c		mhandler.h		\
	monapi.c		monapi.h		\
	net.c			net.h			\
	option.c		option.h		\
	panda.h							\
	port.c			port.h			\
	queue.c			queue.h			\
	socket.c		socket.h		\
	sysdatacom.h					\
	sysdbreq.c		sysdbreq.h		\
	term.c			term.h			\
	wfcio.c			wfcio.h

libpanda_la_LIBADD		=	\
	-lm									\
	$(SSL_LIBS)							\
	$(P11_LIBS)							\
	$(COMMON_LIBS)


libpandaincludedir = $(pkgincludedir)
libpandainclude_HEADERS =	\
	BDparser.h			\
	DBDparser.h			\
	DBparser.h			\
	DDparser.h			\
	DIparser.h			\
	Dlex.h				\
	LDparser.h			\
	SQLlex.h			\
	SQLparser.h			\
	auth.h				\
	authstub.h			\
	blob.h				\
	blobreq.h			\
	comm.h				\
	comms.h				\
	debug.h				\
	directory.h			\
	load.h				\
	message.h			\
	mhandler.h			\
	monapi.h			\
	net.h				\
	option.h			\
	panda.h				\
	port.h				\
	queue.h				\
	socket.h			\
	sysdatacom.h		\
	sysdbreq.h			\
	term.h				\
	wfcio.h

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	$(P11_CFLAGS)			\
	$(SSL_CFLAGS)			\
	$(COMMON_CFLAGS)		\
	-DBIN_DIR=\"$(bindir)\"

testnet_LDADD	= 	\
	-lm									\
	$(top_builddir)/libs/libpanda.la	\
	$(COMMON_LIBS)						\
	$(P11_LIBS)							\
	$(SSL_LIBS)

testnet_DEPENDENCIES	=				\
	$(top_builddir)/libs/libpanda.la

testblob_LDADD	=						\
	-lm									\
	$(top_builddir)/libs/libpanda.la	\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)

testblob_DEPENDENCIES	=

AM_CPPFLAGS = 					\
	-DLOCALEDIR=\"$(localedir)\"

EXTRA_DIST	=					\
	option_parser.txt
