name	sample;

arraysize	20;
textsize	200;

# bind <window name> <hander name> <module name> ;

bind	""			"OpenCOBOL"	"SAMPLE1";
bind	"list"		"OpenCOBOL"	"SAMPLE1";
bind	"H01"		"OpenCOBOL"	"ORCGH01";
bind	"HID1"		"OpenCOBOL"	"ORCGHID1";

#bind	""			"dotCOBOL"	"SAMPLE1";
#bind	"list"		"dotCOBOL"	"SAMPLE1";
#bind	"H01"		"dotCOBOL"	"ORCGH01";
#bind	"HID1"		"dotCOBOL"	"ORCGHID1";

home	"=/sample3";

data	{
	spa		samplespa;
	window	{
		list;
		H01;
		HID1;
	};
};

db	"ORCA"	{
	metadb;
	adrs;
};

db	"shell"	{
	shell;
};
