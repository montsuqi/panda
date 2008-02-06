TOP			=	$(shell pwd)
TOOLDIR		= 	../../tools

DIRECTORY	=	$(TOP)/directory

%.so	: %.CBL
	cobc -m -dynamic -o $@ $<

DBGEN = $(TOOLDIR)/dbgen -record $(TOP)
COPYGEN = $(TOOLDIR)/copygen -dir $(DIRECTORY) -ddir $(TOP) -record $(TOP)

DB-TEST1:	test1.db
	$(COPYGEN) -prefix tst- -top 3 -name TST test1.db > DB-TEST1

MCPAREA: $(DIRECTORY)
	$(COPYGEN) -mcp  > MCPAREA
