#!/usr/bin/ruby -Ke

require "apslib"

class Main < PandaDC
  def start(aps,db)
	i = 0;
	while ( i < 20 )
	  aps["list.fixed1.clist1.item[#{i}].value1"] = "";
	  aps["list.fixed1.clist1.item[#{i}].value2"] = "";
	  aps["list.fixed1.clist1.item[#{i}].value3"] = "";
	  aps["list.fixed1.clist1.item[#{i}].value4"] = "";
	  aps["list.fixed1.clist1.select[#{i}]"] = "FALSE";
	  i += 1;
	end
	aps["link.linktext"] = "";
	aps["samplespa.dummy"] = "";
	putWindow("list","key","NEW");
  end
  def do_Search(aps,db)
	adrs = PandaTable.new(db,"adrs");
	adrs["mail.home"] = aps["list.fixed1.key.value"];
	adrs.execFunction("DBSELECT","mail");
	i = 0;
	while ( i < 20 ) && ( adrs.execFunction("DBFETCH","mail") == 0 )
	  aps["list.fixed1.clist1.item[#{i}].value1"] = adrs["name"];
	  aps["list.fixed1.clist1.item[#{i}].value2"] = adrs["tel"];
	  aps["list.fixed1.clist1.item[#{i}].value3"] = adrs["mail.home"];
	  aps["list.fixed1.clist1.item[#{i}].value4"] = adrs["adress[0]"];
	  aps["list.fixed1.clist1.select[#{i}]"] = "FALSE";
	  i += 1;
	end
	aps["list.fixed1.clist1.count"] = i;
	putWindow("list");
  end
  def do_Quit(aps)
	putWindow("list","","CLOSE");
  end
end

Main.new.run;
