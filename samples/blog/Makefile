.EXPORT_ALL_VARIABLES:

TOP			=	$(shell pwd)
PANDA_DIR	:=	/home/ogochan/MONTSUQI/panda
TOOLDIR		:=	$(PANDA_DIR)/tools

SUBDIRS		=	schedule

DIRECTORY	=	$(TOP)/directory
LINKREC		=	$(TOP)/link.rec

DBGEN = $(TOOLDIR)/dbgen -record $(TOP)
COPYGEN = $(TOOLDIR)/copygen -dir $(DIRECTORY) -ddir $(TOP) -record $(TOP)

TARGET		=	entry.so btest.so
SOURCES		=	entry.c
LDNAME		=	blog
DBFILES		=	blog_entry.db
SCRREC		=	entry.rec
DIRECTORY	=	directory
SPAREC		=	blogspa.rec
LDFILE		=	$(LDNAME).ld

#

GLIB_CFLAGS	:=	$(shell glib-config --cflags)
FIXED_CFLAGS:=	$(shell libfixed-config --cflags)
MONDAI_CFLAGS:=	$(shell libmondai-config --cflags)

DBGEN = $(TOOLDIR)/dbgen -record $(TOP)
COPYGEN = $(TOOLDIR)/copygen -dir $(DIRECTORY) -lddir $(TOP) -record $(TOP)

INCLUDES 			= 		\
	-I$(PANDA_DIR)/include	\
	-I$(PANDA_DIR)/libs		\
	-I$(PANDA_DIR)/aps		\
	$(GLIB_CFLAGS)			\
	$(FIXED_CFLAGS)			\
	$(MONDAI_CFLAGS)

CC			=	gcc
CFLAGS		=	-g -O2 -Wall -Wunused 
LDFLAGS		=	-module -export-dynamic

all: $(TARGET)

clean:
	rm -f *.so *~

%.o: %.c
	$(CC) -fPIC $(CFLAGS) $(INCLUDES) -c $<

%.so: %.o
	$(CC) -shared -o $*.so $<

