require	'monlib';

server = PG_Server.new("localhost",0,"panda:sample","ogochan",";suPer00");

server.setValue("list.fixed1.key.value","ogo%");

ev = server.event("Search");

printf("ev[0] = [%s]\n",ev[0]);
printf("ev[1] = [%s]\n",ev[1]);

count = server.getValue("list.fixed1.clist1.count").to_i
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

val = server.getValues("list.fixed1.clist1.item");
val.each{ | name, value |
  printf("%s: %s\n",name,value); };

server.close;
