require	'monlib';

server = HT_Server.new("localhost",8010,"panda:demo1","ogochan");
#server = HT_Server.new("localhost",8010,"demo","ogochan",";suPer00");
printf("%s = [%s]\n","project1.vbox1.entry1.value",
	server.getValue("project1.vbox1.entry1.value"));
server.commit;

server.start;
server.event("Left","Button");
printf("%s = [%s]\n","project1.vbox1.entry3.value",
	server.getValue("project1.vbox1.entry3.value"));
server.commit;

server.start;
server.setValue("project1.vbox1.entry1.value","おごちゃん");
server.event("Right","Button");
printf("%s = [%s]\n","project1.vbox1.entry3.value",
	server.getValue("project1.vbox1.entry3.value"));
server.commit;

server.start;
server.setValue("project1.vbox1.entry1.value","おご");
server.event("Right","Button");
printf("%s = [%s]\n","project1.vbox1.entry3.value",
	server.getValue("project1.vbox1.entry3.value"));
server.commit;

server.close;
