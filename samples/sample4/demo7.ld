name	demo7;

arraysize	20;
textsize	200;

bind    ""                      "OpenCOBOL"     "TPDEMO7";
bind    "project6"          "OpenCOBOL"     "TPDEMO7";
bind    "project5"          "OpenCOBOL"     "TPDEMO3";

home    "=/sample4";

data	{
	spa		demo7spa;
	window	{
		project6;
		project5;
	};
};

db      "ORCA"  {
        metadb;
        adrs;
};

db      "shell" {
        shell;
};

