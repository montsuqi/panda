name	blog;

bind	""			"C"	"entry";
bind	"entry"		"C"	"entry";

arraysize	20;
textsize	200;
cache		5;

home	"=/blog";

data	{
	spa		blogspa;
	window	{
		entry;
	};
};

db	"blog"	{
	blog_entry;
};
