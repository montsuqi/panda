all: TPDEMO2.so

clean:
	rm -f *.so *.err *~
	rm -f LINKAREA SPAAREA SCRAREA.SE MCPAREA

LDNAME	=	demo2
SCRREC	=	$(TOP)/project3.rec

LDFILE	=	$(TOP)/$(LDNAME).ld
SPAREC	=	$(TOP)/$(LDNAME)spa.rec

%.so	: %.CBL
	cobc -m -dynamic $<

TPDEMO2.so		: TPDEMO2.CBL MCPAREA LINKAREA SPAAREA SCRAREA DB-META

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
