all: sample adrs.create

clean:
	rm -f *.cod *.err *~ db.* dc.*
	rm -f LINKAREA DBCOMM DBAREA

TOP			=	$(shell pwd)
TOOLDIR		= 	/home/ogochan/panda/tools

LDNAME	=	sample

DIRECTORY	=	$(TOP)/directory
LINKREC		=	$(TOP)/link.rec
MCPSPA		=	$(TOP)/mcpspa.rec
METAREC		=	$(TOP)/metadb.db
SPAREC		=	$(TOP)/$(LDNAME)spa.rec
LDFILE		=	$(TOP)/$(LDNAME).ld
SCRREC		=	$(TOP)/list.rec

DBGEN = $(TOOLDIR)/dbgen -record $(TOP)
COPYGEN = $(TOOLDIR)/copygen -dir $(DIRECTORY) -lddir $(TOP) -record $(TOP)

SAMPLE_SRC =	MCPMAIN.CBL MCPSUB.CBL	\
		SAMPLE1.CBL SAMPLEB.CBL ORCGH01.CBL ORCGHID1.CBL

SAMPLE_CODE = $(SAMPLE_SRC:.CBL=.cod)

%.cod	: %.CBL
	cobol -L -E1 -R $<

sample: $(SAMPLE_CODE)

LDRECORDS	=	LINKAREA SPAAREA SCRAREA
DBRECORDS	=	DBPATH DB-META DB-ADRS

MCPMAIN.cod		: MCPMAIN.CBL MCPAREA LDRFILE LDWFILE DB-META
MCPSUB.cod		: MCPSUB.CBL MCPAREA DBINFO DBCOMM DBAREA

SAMPLE1.cod	: SAMPLE1.CBL ENUM-VALUE MCPAREA $(LDRECORDS) $(DBRECORDS)
ORCGH01.cod	: ORCGH01.CBL ENUM-VALUE MCPAREA $(LDRECORDS) $(DBRECORDS)
ORCGHID1.cod: ORCGHID1.CBL ENUM-VALUE MCPAREA $(LDRECORDS) $(DBRECORDS)
SAMPLEB.cod	: SAMPLEB.CBL ENUM-VALUE MCPAREA $(DBRECORDS) 

adrs.create:	adrs.db
	$(DBGEN) -create adrs.db > adrs.create

DBAREA:	directory adrs.db metadb.db
	$(COPYGEN) -db -ld $(LDNAME) > DBAREA

DB-ADRS:	adrs.db
	$(COPYGEN) -prefix adr- -dbrec -ld $(LDNAME) adrs.db > DB-ADRS

DB-META:	metadb.db
	$(COPYGEN) -prefix meta- -dbrec -ld $(LDNAME) metadb.db > DB-META

DBINFO:	directory adrs.db metadb.db
	$(COPYGEN) -dbinfo -ld $(LDNAME) > DBINFO

DBCOMM:	directory adrs.db metadb.db
	$(COPYGEN) -ld $(LDNAME) -dbcomm > DBCOMM

DBPATH:	directory adrs.db metadb.db
	$(COPYGEN) -dbpath -ld $(LDNAME) > DBPATH

LINKAREA:	$(LINKREC) $(DIRECTORY)
	$(COPYGEN) -linkage -prefix lnk- -ld $(LDNAME) > LINKAREA

SCRAREA.SE:	$(DIRECTORY) $(LINKREC) $(SPAREC) $(SCRREC) $(LDFILE)
	$(COPYGEN) -ld $(LDNAME) -screen -wprefix -name scrarea > SCRAREA.SE

LDWFILE: $(DIRECTORY) $(LDFILE) $(SPAREC) $(LINKREC) $(SCRREC)
	$(COPYGEN) -ldw -ld $(LDNAME)  > LDWFILE

LDRFILE: $(DIRECTORY) $(LDFILE) $(SPAREC) $(LINKREC) $(SCRREC)
	$(COPYGEN) -ldr -ld $(LDNAME)  > LDRFILE

MCPAREA: $(DIRECTORY)
	$(COPYGEN) -mcp  > MCPAREA
