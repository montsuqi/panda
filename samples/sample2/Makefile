all: sample adrs.create

clean:
	rm -f *.so *~ *.P
	rm -f LINKAREA SPAAREA DB* SCRAREA.SE

TOP			=	$(shell pwd)
TOOLDIR		= 	/home/ogochan/panda/tools

LDNAME	=	sample

DIRECTORY	=	$(TOP)/directory
LINKREC		=	$(TOP)/link.rec
METAREC		=	$(TOP)/metadb.db
SPAREC		=	$(TOP)/$(LDNAME)spa.rec
LDFILE		=	$(TOP)/$(LDNAME).ld
SCRREC		=	$(TOP)/list.rec

DBGEN = $(TOOLDIR)/dbgen -record $(TOP)
COPYGEN = $(TOOLDIR)/copygen -dir $(DIRECTORY) -lddir $(TOP) -record $(TOP)

SAMPLE_SRC =	SAMPLE1.CBL SAMPLEB.CBL

SAMPLE_CODE = $(SAMPLE_SRC:.CBL=.so)

%.so	: %.CBL
	cobc -m -dynamic -o $@ $<

sample: $(SAMPLE_CODE)

LDRECORDS	=	LINKAREA SPAAREA SCRAREA
DBRECORDS	=	DBPATH DB-META DB-ADRS

SAMPLE1.so	: SAMPLE1.CBL ENUM-VALUE MCPAREA $(LDRECORDS) $(DBRECORDS)
SAMPLEB.so	: SAMPLEB.CBL ENUM-VALUE MCPAREA $(DBRECORDS) 

adrs.create:	adrs.db
	$(DBGEN) -create adrs.db > adrs.create

DB-ADRS:	adrs.db
	$(COPYGEN) -prefix adr- -dbrec -ld $(LDNAME) adrs.db > DB-ADRS

DB-META:	metadb.db
	$(COPYGEN) -prefix meta- -dbrec -ld $(LDNAME) metadb.db > DB-META

DBPATH:	directory adrs.db metadb.db
	$(COPYGEN) -dbpath -ld $(LDNAME) > DBPATH

LINKAREA:	$(LINKREC) $(DIRECTORY)
	$(COPYGEN) -linkage -prefix lnk- -ld $(LDNAME) > LINKAREA

SPAAREA:	$(SPAREC)
	$(COPYGEN) $(SPAREC) -name SPAAREA -prefix spa- -ld $(LDNAME) > SPAAREA

SCRAREA.SE:	$(DIRECTORY) $(LINKREC) $(SPAREC) $(SCRREC) $(LDFILE)
	$(COPYGEN) -ld $(LDNAME) -screen -wprefix -name scrarea > SCRAREA.SE

MCPAREA: $(DIRECTORY)
	$(COPYGEN) -mcp  > MCPAREA

