all: demo7 adrs.create

clean:
	rm -f *.so *~ *.P
	rm -f LINKAREA SPAAREA SCRAREA.SE

TOP			=	$(shell pwd)
TOOLDIR		= 	~/panda/tools

LDNAME	=	demo7

DIRECTORY	=	$(TOP)/directory
LINKREC		=	$(TOP)/link.rec
MCPSPA		=	$(TOP)/mcpspa.rec
METAREC		=	$(TOP)/metadb.db
SPAREC		=	$(TOP)/$(LDNAME)spa.rec
LDFILE		=	$(TOP)/$(LDNAME).ld
SCRREC		=	$(TOP)/project6.rec

DBGEN = $(TOOLDIR)/dbgen -record $(TOP)
COPYGEN = $(TOOLDIR)/copygen -dir $(DIRECTORY) -lddir $(TOP) -record $(TOP)

TPDEMO7_SRC =	TPDEMO7.CBL TPDEMO3.CBL SAMPLEA.CBL SAMPLEB.CBL SAMPLEC.CBL

TPDEMO7_CODE = $(TPDEMO7_SRC:.CBL=.so)

%.so	: %.CBL
	cobc -m -dynamic -o $@ -MT $@ -MF ${@:.so=.P} $<

demo7: $(TPDEMO7_CODE)

LDRECORDS	=	LINKAREA SPAAREA SCRAREA
DBRECORDS	=	DBPATH DB-META DB-ADRS DB-SHELL

TPDEMO7.so	: TPDEMO7.CBL ENUM-VALUE MCPAREA $(LDRECORDS) $(DBRECORDS)
ORCGH01.so	: TPDEMO3.CBL ENUM-VALUE MCPAREA $(LDRECORDS) $(DBRECORDS)
SAMPLEA.cod     : SAMPLEA.CBL ENUM-VALUE MCPAREA $(DBRECORDS) 
SAMPLEB.cod     : SAMPLEB.CBL ENUM-VALUE MCPAREA $(DBRECORDS) 
SAMPLEC.cod     : SAMPLEC.CBL ENUM-VALUE MCPAREA $(DBRECORDS) 

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

SPAAREA:	$(SPAREC) $(MCPSPA)
	$(COPYGEN) $(SPAREC) -name SPAAREA -prefix spa- -ld $(LDNAME) > SPAAREA

SCRAREA.SE:	$(DIRECTORY) $(LINKREC) $(SPAREC) $(SCRREC) $(LDFILE)
	$(COPYGEN) -ld $(LDNAME) -screen -wprefix -name scrarea > SCRAREA.SE

MCPAREA: $(DIRECTORY)
	$(COPYGEN) -mcp  > MCPAREA
