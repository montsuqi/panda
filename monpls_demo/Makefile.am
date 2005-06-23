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

lib_LTLIBRARIES	=	$(MONPLS_OBJECTS)
MONPLS_OBJECTS	=	\
	demo.la		\
	demo2.la	\
	demo3.la	\
	demo4.la	\
	demo5.la

demo_la_LDFLAGS		=	-module -export-dynamic
demo2_la_LDFLAGS	=	-module -export-dynamic
demo3_la_LDFLAGS	=	-module -export-dynamic
demo4_la_LDFLAGS	=	-module -export-dynamic
demo5_la_LDFLAGS	=	-module -export-dynamic

demo_la_SOURCES		=	demo.c
demo2_la_SOURCES	=	demo2.c
demo3_la_SOURCES	=	demo3.c
demo4_la_SOURCES	=	demo4.c
demo5_la_SOURCES	=	demo5.c

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	$(GTK_CFLAGS)			\
	$(GLIB_CFLAGS)			\
	$(MONDAI_CFLAGS)
