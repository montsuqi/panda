name	sample;

# bind <window name> <hander name> <module name> ;

handler	"Ruby"	{
	class		"Exec";
	serialize	"CGI";
	start		"/usr/bin/ruby -Ke %p/%m.rb";
	locale		"euc-jp";
	encoding	"URL";
};

#bind	""			"dotCOBOL"	"SAMPLE1";
#bind	"list"		"dotCOBOL"	"SAMPLE1";

bind	""			"OpenCOBOL"	"SAMPLE1";
bind	"list"		"OpenCOBOL"	"SAMPLE1";

#bind	""			"Ruby"		"SAMPLE1";
#bind	"list"		"Ruby"		"SAMPLE1";

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

db	"system"	{
	metadb;
#	system;
};
