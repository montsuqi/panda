all: sample adrs.create

clean:
	rm -f *.so *~
	rm -f LINKAREA SPAAREA SCRAREA.SE

TOP			=	$(shell pwd)
TOOLDIR		= 	/home/ogochan/MONTSUQI/panda/tools

LDNAME	=	sample

DIRECTORY	=	$(TOP)/directory
LINKREC		=	$(TOP)/link.rec
MCPSPA		=	$(TOP)/mcpspa.rec
METAREC		=	$(TOP)/metadb.db
SPAREC		=	$(TOP)/$(LDNAME)spa.rec
LDFILE		=	$(TOP)/$(LDNAME).ld
SCRREC		=	$(TOP)/list.rec

DBGEN = $(TOOLDIR)/dbgen -record $(TOP)
COPYGEN = $(TOOLDIR)/copygen -dir $(DIRECTORY) -ddir $(TOP) -record $(TOP)

SAMPLE_SRC =	SAMPLE1.CBL SAMPLEB.CBL ORCGH01.CBL ORCGHID1.CBL

SAMPLE_CODE = $(SAMPLE_SRC:.CBL=.so)

%.so	: %.CBL
	cobc -m -dynamic $<

sample: $(SAMPLE_CODE)

LDRECORDS	=	LINKAREA SPAAREA SCRAREA
DBRECORDS	=	DBPATH DB-META DB-ADRS DB-SHELL

SAMPLE1.so	: SAMPLE1.CBL ENUM-VALUE MCPAREA $(LDRECORDS) $(DBRECORDS)
ORCGH01.so	: ORCGH01.CBL ENUM-VALUE MCPAREA $(LDRECORDS) $(DBRECORDS)
ORCGHID1.so	: ORCGHID1.CBL ENUM-VALUE MCPAREA $(LDRECORDS) $(DBRECORDS)

SAMPLEB.so	: SAMPLEB.CBL ENUM-VALUE MCPAREA $(DBRECORDS) 

adrs.create:	adrs.db
	$(DBGEN) -create adrs.db > adrs.create

DB-ADRS:	adrs.db
	$(COPYGEN) -prefix adr- -dbrec -ld $(LDNAME) adrs.db > DB-ADRS

DB-META:	metadb.db
	$(COPYGEN) -prefix meta- -dbrec -ld $(LDNAME) metadb.db > DB-META

DB-SHELL:	shell.db
	$(COPYGEN) -prefix shell- -dbrec -ld $(LDNAME) shell.db > DB-SHELL

DBPATH:	directory adrs.db metadb.db shell.db
	$(COPYGEN) -dbpath -ld $(LDNAME) > DBPATH

LINKAREA:	$(LINKREC) $(DIRECTORY)
	$(COPYGEN) -linkage -prefix lnk- -ld $(LDNAME) > LINKAREA

SCRAREA.SE:	$(DIRECTORY) $(LINKREC) $(SPAREC) $(SCRREC) $(LDFILE)
	$(COPYGEN) -ld $(LDNAME) -screen -wprefix -name scrarea > SCRAREA.SE

MCPAREA: $(DIRECTORY)
	$(COPYGEN) -mcp  > MCPAREA
