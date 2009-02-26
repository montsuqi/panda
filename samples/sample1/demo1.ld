name	demo1;

arraysize	20;
textsize	200;

handler	"RubyExec"	{
	class	"Exec";
	serialize	"CGI";
	start		"/usr/bin/ruby -Ke %p/%m.rb";
	coding		"euc-jp";
	encoding	"URL";
};

handler	"Ruby"	{
	class		"Ruby";
    loadpath	"=/sample1/demo1";
	locale		"utf-8";
};

#bind	""			"dotCOBOL"	"TPDEMO1";
#bind	"project1"	"dotCOBOL"	"TPDEMO1";
#bind	"project2"	"dotCOBOL"	"TPDEMO1";
#bind	"project5"	"dotCOBOL"	"TPDEMO3";

#bind	""			"RubyExec"	"TPDEMO1";
#bind	"project1"	"RubyExec"	"TPDEMO1";
#bind	""			"OpenCOBOL"	"TPDEMO1";
#bind	"project1"	"OpenCOBOL"	"TPDEMO1";

#bind	"project2"	"OpenCOBOL"	"TPDEMO1";
#bind	"project5"	"OpenCOBOL"	"TPDEMO3";

bind	""			"Ruby"		"TpDemo1";
bind	"project1"	"Ruby"		"TpDemo1";
bind	"project2"	"Ruby"		"TpDemo1";

multiplex_group	demo1;

cache		5;
#cache		0;

home	"=/sample1/demo1";

data	{
	spa		demo1spa;
	window	{
		project1;
		project2;
		project5;
	};
};

