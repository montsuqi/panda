#!/usr/bin/ruby -Ke

require "apslib"

$stderr.printf("SAMPLEB start\n");

db = PandaDB.new;
db.execFunction("DBOPEN");
db.execFunction("DBSTART");

adrs = PandaTable.new(db,"adrs");
adrs["mail.home"] = "ogochan%";
adrs.execFunction("DBSELECT","mail");
while adrs.execFunction("DBFETCH","mail") == 0
  adrs.dump($stderr);
end

db.execFunction("DBCOMMIT");
db.execFunction("DBDISCONNECT");

$stderr.printf("SAMPLEB end\n");
