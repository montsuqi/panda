db_group	"log"	{
	priority	100;
	type	"Oseki";
	port	"localhost";
	name	"test";
	redirect_port	"localhost:9302";
	file	"test.log";
};

db_group	"Oseki"	{
	type	"Oseki";
	port	"localhost:2468";
	user	"ogochan";
	password	"ogochan";
	name	"test";
#	redirect		"log";
};
