require	'dbslib';

server = DB_Server.new("localhost",0,"ogochan",";suPer00");

adrs = Hash.new(nil);

adrs["adrs.mail.home"] = "ogo%";

rc = server.dbops("DBOPEN");
printf("rc = %d\n",rc);
rc = server.dbops("DBSTART");
printf("rc = %d\n",rc);

printf("[%s]\n",server.getSchema("adrs","mail"));

adrs.each{ | name, value | printf("%s: %s\n",name,value) };

rc = server.recordops("DBSELECT","adrs","mail",adrs);
printf("rc = %d\n",rc);

adrs.each{ | name, value | printf("%s: %s\n",name,value) };

while server.recordops("DBFETCH","adrs","mail",adrs) == 0
  adrs.each{ | name, value | printf("%s: %s\n",name,value) };
end

rc = server.dbops("DBCOMMIT");
printf("rc = %d\n",rc);
rc = server.dbops("DBDISCONNECT");
printf("rc = %d\n",rc);

server.close;
