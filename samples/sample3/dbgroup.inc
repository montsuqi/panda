db_group	"ORCA"	{
	type	"PostgreSQL";
	port	"localhost";
	name	"ORCA";
	redirect_port	"localhost";
	redirect		"log";
};

db_group	"log"	{
	priority	100;
	type	"PostgreSQL";
	port	"localhost";
	name	"panda";
	file	"orca.log";
};

db_group	"shell"	{
	type	"Shell";
	priority	1000;
};
