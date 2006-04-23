TARGET		=	mainColor.so main.so
SOURCES		=	mainColor.c main.c
LDNAME		=	web
DBFILES		=	
SCRREC		=	login.rec main.rec
DIRECTORY	=	directory
SPAREC		=	webspa.rec
LDFILE		=	$(LDNAME).ld

PANDA_DIR	=	/home/ogochan/MONTSUQI/panda-current

#

GLIB_CFLAGS	:=	$(shell glib-config --cflags)
MONDAI_CFLAGS:=	$(shell libmondai-config --cflags)

DBGEN = $(TOOLDIR)/dbgen -record $(TOP)
COPYGEN = $(TOOLDIR)/copygen -dir $(DIRECTORY) -lddir $(TOP) -record $(TOP)

INCLUDES 			= 		\
	-I$(PANDA_DIR)/include	\
	-I$(PANDA_DIR)/libs		\
	-I$(PANDA_DIR)/aps		\
	$(GLIB_CFLAGS)			\
	$(MONDAI_CFLAGS)

CC			=	gcc
CFLAGS		=	-g -O2 -Wall -Wunused 
LDFLAGS		=	-module -export-dynamic

all: $(TARGET)

clean:
	rm -f *.so *~ *.P
	rm -f LINKAREA SPAAREA DB* SCRAREA.SE

%.o: %.c
	$(CC) -fPIC $(CFLAGS) $(INCLUDES) -c $<

%.so: %.o
	$(CC) -shared -o $*.so $<
%.so	: %.CBL
	cobc -m -dynamic $<

DBPATH:	$(DIRECTORY) $(DBFILES)
	$(COPYGEN) -dbpath -ld $(LDNAME) > DBPATH

LINKAREA:	$(LINKREC) $(DIRECTORY)
	$(COPYGEN) -linkage -prefix lnk- -ld $(LDNAME) > LINKAREA

SPAAREA:	$(SPAREC)
	$(COPYGEN) $(SPAREC) -name SPAAREA -prefix spa- -ld $(LDNAME) > SPAAREA

SCRAREA.SE:	$(DIRECTORY) $(SCRREC) $(LDFILE)
	$(COPYGEN) -ld $(LDNAME) -screen -wprefix -name scrarea > SCRAREA.SE

MCPAREA: $(DIRECTORY)
	$(COPYGEN) -mcp  > MCPAREA
