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

lib_LTLIBRARIES = panda.la libpanda.la

noinst_LIBRARIES	=		\
	libpanda.a				\
	libpandas.a				\
	libpandacom.a			\
	libpandagl.a			\
	libpandamisc.a			\
	libpandablob.a

BUILT_SOURCES = enum.h

noinst_PROGRAMS		=		\
	enum					\
	testnet					\
	testblob

panda_la_LDFLAGS	=			\
	-version-info $(LTVERSION)	\
	-module						\
	-export-dynamic

panda_la_DEPENDENCIES =				\
	enum.h
panda_la_SOURCES	=				\
	Dlex.h							\
	mhandler.c	mhandler.h			\
	DDparser.c	DDparser.h			\
	BDparser.c	BDparser.h			\
	LDparser.c	LDparser.h			\
	DBDparser.c	DBDparser.h			\
	DIparser.c	DIparser.h			\
	DBparser.c	DBparser.h			\
	SQLlex.c	SQLlex.h			\
	SQLparser.c	SQLparser.h			\
	blobreq.c	blobreq.h			\
	blobcom.h						\
	keyvaluereq.c					\
	keyvaluereq.h					\
	keyvaluecom.h					\
	directory.c	directory.h			\
	driver.c	driver.h	enum.h	\
	load.c		load.h				\
	applications.c applications.h	\
	comm.c		comm.h				\
	comms.c		comms.h				\
	net.c		net.h				\
	term.c		term.h				\
	socket.c	socket.h			\
	port.c		port.h				\
	pty.c		pty.h				\
	wfcio.c		wfcio.h				\
	panda.c


panda_la_LIBADD		=	\
	-lm									\
	$(SSL_LIBS)							\
	$(GLIB_LIBS)						\
	$(MONDAI_LIBS)

libpandaincludedir = $(includedir)/panda
libpandainclude_HEADERS =	\
	enum.h
libpanda_a_DEPENDENCIES =	\
	enum.h
INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	$(GTK_CFLAGS)			\
	$(GLIB_CFLAGS)			\
	$(MONDAI_CFLAGS)		\
	-DBIN_DIR=\"$(bindir)\"

enum_LDADD	= 	\
	 -lm	\
	$(GLADE_LIBS)			\
	$(GLIB_LIBS)			\
	$(MONDAI_LIBS)

testnet_LDADD	= 	\
	-lm		\
	$(GLIB_LIBS) \
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a	\
	$(MONDAI_LIBS)			\
	$(SSL_LIBS)

testnet_DEPENDENCIES	=		\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a

libpanda_la_CFLAGS = $(AM_CFLAGS)
libpanda_la_LDFLAGS	=	\
	-version-info $(LTVERSION)	\
	-export-dynamic
libpanda_la_SOURCES	=		\
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
	blobcom.h						\
	keyvaluereq.c					\
	keyvaluereq.h					\
	keyvaluecom.h					\
	directory.c		directory.h		\
	driver.c		driver.h	enum.h	\
	load.c			load.h			\
	debug.h							\
	applications.c	applications.h	\
	front.c			front.h			\
	comm.c			comm.h			\
	comms.c			comms.h			\
	auth.c			auth.h			\
	authstub.c		authstub.h		\
	net.c			net.h			\
	socket.c		socket.h		\
	port.c			port.h			\
	pty.c			pty.h			\
	option.c		option.h		\
	message.c		message.h		\
	queue.c			queue.h

libpanda_a_CFLAGS = $(AM_CFLAGS)
libpanda_a_SOURCES	=				\
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
	blobcom.h						\
	keyvaluereq.c	keyvaluereq.h	\
	keyvaluecom.h					\
	directory.c		directory.h		\
	driver.c		driver.h		\
	enum.h							\
	load.c			load.h			\
	debug.h

libpandas_a_CFLAGS = $(AM_CFLAGS)
libpandas_a_SOURCES	=			\
	applications.c applications.h	\
	DDparser.c		DDparser.h		\
	driver.c		driver.h		\
	enum.h							\
	term.c			term.h			\
	front.c			front.h			\
	comm.c			comm.h			\
	comms.c			comms.h			\
	glcomm.c		glcomm.h		\
	auth.c			auth.h			\
	authstub.c		authstub.h		\
	blobreq.c		blobreq.h		\
	keyvaluereq.c	keyvaluereq.h	\
	keyvaluecom.h					\
	blobcom.h						\
	monapi.c	monapi.h			\
	load.c		load.h

libpandas_a_DEPENDENCIES	=		\
	$(top_builddir)/defaults.h

#libpandacominclude_HEADERS =	\
#	comm.h			\
#	comms.h			\
#	socket.h		\
#	pty.h			\
#	queue.h

libpandacom_a_CFLAGS = $(AM_CFLAGS)
libpandacom_a_SOURCES	=			\
	auth.c			auth.h			\
	authstub.c		authstub.h		\
	net.c			net.h			\
	comm.c			comm.h			\
	comms.c			comms.h			\
	socket.c		socket.h		\
	port.c			port.h			\
	pty.c			pty.h			\
	queue.c			queue.h

libpandagl_a_CFLAGS = $(AM_CFLAGS)
libpandagl_a_SOURCES	=			\
	net.c			net.h			\
	socket.c		socket.h		\
	port.c			port.h			\
	queue.c			queue.h

enum.h:	enum enum.c
	./enum > enum.h

libpandablobincludedir = $(includedir)/panda
libpandablobinclude_HEADERS =	\
	blob_v1.h					\
	blob_v2.h					\
	blob.h

libpandablob_a_CFLAGS = $(AM_CFLAGS)
libpandablob_a_SOURCES	=			\
	blob_v1.c		blob_v1.h		\
					blob_v2.h		\
	table.c			table.h			\
	blob.c			blob.h			\
	keyvalue.c		keyvalue.h

libpandablob_a_DEPENDENCIES	=		\
	$(top_builddir)/defaults.h

libpandamisc_a_CFLAGS = $(AM_CFLAGS)
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
	$(GLIB_LIBS)				\
	$(SSL_LIBS)					\
	$(MONDAI_LIBS)				

testblob_DEPENDENCIES	=

AM_CPPFLAGS = 					\
	-DLOCALEDIR=\"$(localedir)\"

DISTCLEANFILES = enum.h
EXTRA_DIST	=					\
	blobup.c					\
	option.doc
