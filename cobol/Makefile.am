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

COPYCMD = $(top_srcdir)/tools/copy

all:	ENUM-VALUE MCPMAIN.CBL

ENUM-VALUE: ENUM-VALUE.C
	$(COPYCMD) $< > $@

MCPMAIN.CBL: MCPMAIN.CBL.C
	$(COPYCMD) $< > $@

EXTRA_DIST =					\
	ENUM-VALUE.C				\
	MCPMAIN.CBL.C				\
	MCPAREA						\
	MCPDCSUB.CBL				\
	MCPSUB.CBL

CLEANFILES	=					\
	ENUM-VALUE					\
	MCPMAIN.CBL
