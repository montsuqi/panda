name	demo1;

arraysize	20;
textsize	200;

wfc		"localhost:9001";

#bind	""			"dotCOBOL"	"TPDEMO1";
#bind	"project1"	"dotCOBOL"	"TPDEMO1";
#bind	"project2"	"dotCOBOL"	"TPDEMO1";
#bind	"project5"	"dotCOBOL"	"TPDEMO3";

bind	""			"Exec"		"TPDEMO1.rb";
bind	"project1"	"Exec"		"TPDEMO1.rb";

#bind	""			"OpenCOBOL"	"TPDEMO1";
#bind	"project1"	"OpenCOBOL"	"TPDEMO1";
bind	"project2"	"OpenCOBOL"	"TPDEMO1";
bind	"project5"	"OpenCOBOL"	"TPDEMO3";

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

db	{
	metadb;
};
