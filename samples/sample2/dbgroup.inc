db_group	"log"	{
	priority	100;
	type	"PostgreSQL";
	port	"localhost";
	name	"ORCA";
	redirect_port	"localhost:9302";
#	redirect		"redirect";
	file	"orca.log1";
};

db_group	"redirect"	{
	priority	100;
	type	"PostgreSQL";
#	name	"ORCA";
	redirect_port	"localhost:9301";
	file	"orca.log2";
};

db_group	"ORCA"	{
	type	"PostgreSQL";
	port	"localhost";
	name	"ORCA";
	redirect		"log";
};

db_group	"system"	{
	type	"System";
};

db_group	"shell"	{
	type	"Shell";
	priority	1000;
};
