all: TPDEMO1.so TPDEMO3.so

clean:
	rm -f *.so *.err *~
	rm -f LINKAREA SPAAREA SCRAREA.SE

LDNAME	=	demo1
SCRREC	=	$(TOP)/project1.rec $(TOP)/project5.rec

LDFILE	=	$(TOP)/$(LDNAME).ld
SPAREC	=	$(TOP)/$(LDNAME)spa.rec

%.so	: %.CBL
	cobc -m -dynamic $<

TPDEMO1.so		: TPDEMO1.CBL MCPAREA LINKAREA SPAAREA SCRAREA DB-META
TPDEMO3.so		: TPDEMO3.CBL MCPAREA LINKAREA SPAAREA SCRAREA DB-META

LINKAREA:	$(LINKREC) $(DIRECTORY)
	$(COPYGEN) -linkage -prefix lnk- -ld $(LDNAME) > LINKAREA

SPAAREA:	$(SPAREC)
	$(COPYGEN) $(SPAREC) -name SPAAREA -prefix spa- -ld $(LDNAME) > SPAAREA

DB-META:	$(METAREC)
	$(COPYGEN) -prefix meta- -dbrec -ld $(LDNAME) $(METAREC) > DB-META

SCRAREA.SE:	$(DIRECTORY) $(SCRREC) $(LDFILE)
	$(COPYGEN) -ld $(LDNAME) -screen -name scrarea > SCRAREA.SE

MCPAREA: $(DIRECTORY)
	$(COPYGEN) -mcp  > MCPAREA
