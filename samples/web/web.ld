name	web;

# bind <window name> <hander name> <module name> ;

HandlerPath	".:/home/ogochan/MONTSUQI/panda/aps/.libs";

handler	"Ruby"	{
	class		"Ruby";
    loadpath	"=/web";
	locale		"utf-8";
};

bind	""				"Ruby"	"Login";
bind	"login"			"Ruby"	"Login";
bind	"main"			"Ruby"	"Main";
#bind	"main"			"C"	"main";

bind	"next"			"Ruby"	"Main";
bind	"file"			"Ruby"	"FileUp";

home	"=/web";

data	{
	spa		webspa;
	window	{
		login;
		main;
		next;
		file;
	};
};

db	"BLOB"	{
	blob;
};
