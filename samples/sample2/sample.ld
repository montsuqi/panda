name	sample;

# bind <window name> <hander name> <module name> ;

#bind	""			"dotCOBOL"	"SAMPLE1";
#bind	"list"		"dotCOBOL"	"SAMPLE1";

bind	""			"OpenCOBOL"	"SAMPLE1";
bind	"list"		"OpenCOBOL"	"SAMPLE1";

arraysize	20;
textsize	200;
cache		5;

home	"=/sample2";

data	{
	spa		samplespa;
	window	{
		list;
	};
};

db	"ORCA"	{
	adrs;
};

db	"shell"	{
	shell;
};
