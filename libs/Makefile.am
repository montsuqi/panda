# PANDA -- a simple transaction monitor
# Copyright (C) 2005  ogochan
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

lib_LTLIBRARIES	=	libpanda.la libpandas.la libpandacom.la libpandagl.la libpandamisc.la panda.la

noinst_PROGRAMS	=	enum testnet

panda_la_LDFLAGS	=	-module -export-dynamic
panda_la_SOURCES	=		\
	wfcio.c		wfcio.h		\
	panda.c
panda_la_DEPENDENCIES = libpanda.la
panda_la_LIBADD		=	\
	-L. -lpanda

libpandaincludedir = $(includedir)/panda
libpandainclude_HEADERS =	\
	enum.h
libpanda_la_DEPENDENCIES =	\
	enum.h

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	-I$(top_srcdir)/blob	\
	$(GTK_CFLAGS)			\
	$(GLIB_CFLAGS)			\
	$(MONDAI_CFLAGS)

enum_LDADD	= 	\
	 -lm	\
	$(GLADE_LIBS)			\
	$(GLIB_LIBS)			\
	$(MONDAI_LIBS)

testnet_LDADD	= 	\
	-lm		\
	$(GLIB_LIBS) \
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la	\
	$(MONDAI_LIBS)			\
	$(SSL_LIBS)

testnet_DEPENDENCIES	=		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la

libpanda_la_SOURCES	=				\
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
	term.c		term.h				\
	blobreq.c	blobreq.h			\
	directory.c	directory.h			\
	load.c		load.h				\
	blobcom.h						\
	debug.h

libpandas_la_SOURCES	=			\
	applications.c applications.h	\
	DDparser.c	DDparser.h			\
	driver.c	driver.h	enum.h	\
	term.c		term.h				\
	front.c		front.h				\
	comm.c		comm.h				\
	comms.c		comms.h				\
	auth.c		auth.h				\
	authstub.c	authstub.h			\
	load.c		load.h

libpandas_la_DEPENDENCIES	=		\
	$(top_srcdir)/defaults.h

#libpandacominclude_HEADERS =	\
#	comm.h			\
#	comms.h			\
#	socket.h		\
#	pty.h			\
#	queue.h

libpandacom_la_SOURCES	=			\
	auth.c			auth.h			\
	authstub.c		authstub.h		\
	net.c			net.h			\
	comm.c			comm.h			\
	comms.c			comms.h			\
	socket.c		socket.h		\
	port.c			port.h			\
	pty.c			pty.h			\
	queue.c			queue.h
libpandagl_la_SOURCES	=			\
	net.c			net.h			\
	socket.c		socket.h		\
	port.c			port.h			\
	queue.c			queue.h

enum.h:	enum
	./enum > enum.h

libpandamisc_la_SOURCES	=		\
	option.c		option.h	\
	message.c		message.h
