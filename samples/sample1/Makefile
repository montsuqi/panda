.EXPORT_ALL_VARIABLES:

TOP			=	$(shell pwd)
TOOLDIR		= 	~/MONTSUQI/panda/tools

SUBDIRS		=	demo1 demo2

DIRECTORY	=	$(TOP)/directory
LINKREC		=	$(TOP)/link.rec
METAREC		=	$(TOP)/metadb.db

DBGEN = $(TOOLDIR)/dbgen -record $(TOP)
COPYGEN = $(TOOLDIR)/copygen -dir $(DIRECTORY) -lddir $(TOP) -record $(TOP)

all: suball

clean: subclean
	rm -f *~

suball:
	set -e; for i in $(SUBDIRS); do $(MAKE) -f Makefile -C $$i all; done
subclean:
	set -e; for i in $(SUBDIRS); do $(MAKE) -f Makefile -C $$i clean; done
