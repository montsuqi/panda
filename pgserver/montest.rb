require	'monlib';

server = PG_Server.new("localhost",0,"panda:sample","ogochan",";suPer00");

server.setValue("list.fixed1.key.value","ogo%");

server.event("Search");

count = server.getValue("list.fixed1.clist1.count").to_i;
printf("count = %d\n",count);

i = 0;
while i < count
  printf("[%s][%s][%s][%s]\n",
		 server.getValue(sprintf("list.fixed1.clist1.item[%d].value1",i)),
		 server.getValue(sprintf("list.fixed1.clist1.item[%d].value2",i)),
		 server.getValue(sprintf("list.fixed1.clist1.item[%d].value3",i)),
		 server.getValue(sprintf("list.fixed1.clist1.item[%d].value4",i)));
  i += 1;
end

server.close;
