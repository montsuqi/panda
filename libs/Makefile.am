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

noinst_LIBRARIES	=		\
	libpandamain.a			\
	libpandas.a				\
	libpandacom.a			\
	libpandagl.a			\
	libpandamisc.a			\
	libpandablob.a

noinst_PROGRAMS		=		\
	testnet					\
	testblob

libpanda_la_CFLAGS = $(AM_CFLAGS)
libpanda_la_LDFLAGS	=			\
	-version-info $(LTVERSION)	\
	-export-dynamic

libpanda_la_SOURCES	=				\
	Dlex.h							\
	mhandler.c		mhandler.h		\
	DDparser.c		DDparser.h		\
	BDparser.c		BDparser.h		\
	LDparser.c		LDparser.h		\
	DBDparser.c		DBDparser.h		\
	DIparser.c		DIparser.h		\
	DBparser.c		DBparser.h		\
	SQLlex.c		SQLlex.h		\
	SQLparser.c		SQLparser.h		\
	blobreq.c		blobreq.h		\
	sysdbreq.c		sysdbreq.h		\
	sysdatacom.h					\
	directory.c		directory.h		\
	driver.c		driver.h		\
	load.c			load.h			\
	comm.c			comm.h			\
	comms.c			comms.h			\
	net.c			net.h			\
	term.c			term.h			\
	socket.c		socket.h		\
	port.c			port.h			\
	queue.c			queue.h			\
	message.c		message.h		\
	debug.h							\
	panda.h

libpanda_la_LIBADD		=	\
	-lm									\
	$(SSL_LIBS)							\
	$(P11_LIBS)							\
	$(COMMON_LIBS)

libpandaincludedir = $(includedir)/panda

libpandainclude_HEADERS =	\
	Dlex.h				\
	mhandler.h			\
	DDparser.h			\
	BDparser.h			\
	LDparser.h			\
	DBDparser.h			\
	DIparser.h			\
	DBparser.h			\
	SQLlex.h			\
	SQLparser.h			\
	blobreq.h			\
	sysdbreq.h			\
	sysdatacom.h		\
	directory.h			\
	driver.h			\
	load.h				\
	comm.h				\
	comms.h				\
	net.h				\
	term.h				\
	socket.h			\
	port.h				\
	queue.h				\
	message.h			\
	debug.h				\
	panda.h

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
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a	\
	$(COMMON_LIBS)						\
	$(P11_LIBS)							\
	$(SSL_LIBS)

testnet_DEPENDENCIES	=		\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a

libpandamain_a_CFLAGS = $(AM_CFLAGS)	\
	-fPIC
libpandamain_a_SOURCES	=				\
	Dlex.h							\
	mhandler.c		mhandler.h		\
	DDparser.c		DDparser.h		\
	BDparser.c		BDparser.h		\
	LDparser.c		LDparser.h		\
	DBDparser.c		DBDparser.h		\
	DIparser.c		DIparser.h		\
	DBparser.c		DBparser.h		\
	SQLlex.c		SQLlex.h		\
	SQLparser.c		SQLparser.h		\
	term.c			term.h			\
	blobreq.c		blobreq.h		\
	sysdbreq.c		sysdbreq.h		\
	sysdatacom.h					\
	directory.c		directory.h		\
	driver.c		driver.h		\
	load.c			load.h			\
	debug.h

libpandas_a_CFLAGS = $(AM_CFLAGS)	\
	-fPIC
libpandas_a_SOURCES	=				\
	sessioncall.c 	sessioncall.h	\
	DDparser.c		DDparser.h		\
	driver.c		driver.h		\
	term.c			term.h			\
	front.c			front.h			\
	comm.c			comm.h			\
	comms.c			comms.h			\
	glcomm.c		glcomm.h		\
	auth.c			auth.h			\
	authstub.c		authstub.h		\
	blobreq.c		blobreq.h		\
	sysdbreq.c		sysdbreq.h		\
	sysdatacom.h					\
	monapi.c		monapi.h		\
	load.c			load.h			\
	wfcio.c			wfcio.h	

libpandas_a_DEPENDENCIES	=		\
	$(top_builddir)/defaults.h

libpandacom_a_CFLAGS = $(AM_CFLAGS)	\
	-fPIC
libpandacom_a_SOURCES	=			\
	DDparser.c		DDparser.h		\
	auth.c			auth.h			\
	authstub.c		authstub.h		\
	net.c			net.h			\
	comm.c			comm.h			\
	comms.c			comms.h			\
	socket.c		socket.h		\
	port.c			port.h			\
	monapi.c		monapi.h		\
	queue.c			queue.h

libpandagl_a_CFLAGS = $(AM_CFLAGS)	\
	-fPIC
libpandagl_a_SOURCES	=			\
	net.c			net.h			\
	socket.c		socket.h		\
	port.c			port.h			\
	queue.c			queue.h

libpandablobincludedir = $(includedir)/panda
libpandablobinclude_HEADERS =	\
	blob.h

libpandablob_a_CFLAGS = $(AM_CFLAGS)	\
	-fPIC
libpandablob_a_SOURCES	=			\
	blob.c			blob.h	

libpandablob_a_DEPENDENCIES	=		\
	$(top_builddir)/defaults.h

libpandamisc_a_CFLAGS = $(AM_CFLAGS)	\
	-fPIC
libpandamisc_a_SOURCES	=		\
	option.c		option.h	\
	net.c			net.h		\
	socket.c		socket.h	\
	port.c			port.h		\
	message.c		message.h

testblob_LDADD	=				\
	-lm							\
	libpandablob.a				\
	libpandacom.a				\
	libpandamisc.a				\
	$(P11_LIBS)					\
	$(SSL_LIBS)					\
	$(COMMON_LIBS)

testblob_DEPENDENCIES	=

AM_CPPFLAGS = 					\
	-DLOCALEDIR=\"$(localedir)\"

EXTRA_DIST	=					\
	option_parser.txt
