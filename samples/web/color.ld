name	color;

# bind <window name> <hander name> <module name> ;

HandlerPath	".:/home/ogochan/MONTSUQI/panda/aps/.libs";

handler	"Ruby"	{
	class		"Ruby";
    loadpath	"=/web";
	locale		"utf-8";
};

bind	"main.area1"	"Ruby"	"MainColor";
#bind	"main.area2"	"Ruby"	"MainColor";
#bind	"main.area1"	"C"		"mainColor";
bind	"main.area2"	"C"		"mainColor";

bind	"next.area1"	"Ruby"	"MainColor";
#bind	"next.area2"	"C"		"mainColor";
#bind	"next.area1"	"Ruby"	"mainColor";
bind	"next.area2"	"C"		"mainColor";

home	"=/web";

data	{
	spa		webspa;
	window	{
		main;
		next;
		login;
	};
};
