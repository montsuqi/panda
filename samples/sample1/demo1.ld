name	demo1;

arraysize	20;
textsize	200;

wfc		"localhost:9001";

#bind	""			"dotCOBOL"	"TPDEMO1";
#bind	"project1"	"dotCOBOL"	"TPDEMO1";
#bind	"project2"	"dotCOBOL"	"TPDEMO1";
#bind	"project5"	"dotCOBOL"	"TPDEMO3";

bind	""			"OpenCOBOL"	"TPDEMO1";
bind	"project1"	"OpenCOBOL"	"TPDEMO1";
bind	"project2"	"OpenCOBOL"	"TPDEMO1";
bind	"project5"	"OpenCOBOL"	"TPDEMO3";

multiplex_group	demo1;

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
