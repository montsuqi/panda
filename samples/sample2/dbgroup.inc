db_group	"log"	{
	priority	100;
	type	"PostgreSQL";
#	port	"localhost";
#	name	"ORCA";
	file	"orca.log";
};

db_group	"ORCA"	{
	type	"PostgreSQL";
	port	"localhost";
	name	"ORCA";
	redirect_port	"localhost:9300";
	redirect		"log";
};

db_group	"system"	{
	type	"System";
};

db_group	"shell"	{
	type	"Shell";
	priority	1000;
};
