#!/usr/bin/ruby -Ke

require "apslib"

class Main < PandaDC
  def start(aps)
	$stderr.printf("start TPDEMO1.rb\n");
	f = open("aps.log","w");
	aps.dump(f);
	f.close;

	if aps["demolink.linktext"] == ""
	  aps["demolink.linktext"] = "漢字を入れてみた。";
	end
	aps["project1.vbox1.entry1.value"] = aps["demolink.linktext"];

	aps["project1.vbox1.entry1.style"] = "green";

	aps["project1.vbox1.entry2.value"] = -1234;
	aps["project1.vbox1.entry2.style"] = "red";

	aps["project1.vbox1.entry3.style"] = "blue";

	aps["project1.vbox1.combo1.item[0]"] = "おごちゃん";
	aps["project1.vbox1.combo1.item[1]"] = "えりさ";
	aps["project1.vbox1.combo1.item[2]"] = "ques";
	aps["project1.vbox1.combo1.name.style"] = "blue";
	aps["project1.vbox1.combo1.count"] = 3;

	aps["project1.vbox1.swin2.vp1.list1.item[0]"] = "おごちゃん";
	aps["project1.vbox1.swin2.vp1.list1.item[1]"] = "えりさ";
	aps["project1.vbox1.swin2.vp1.list1.item[2]"] = "ques";
	aps["project1.vbox1.swin2.vp1.list1.item[3]"] = "void_No2";
	aps["project1.vbox1.swin2.vp1.list1.item[4]"] = "Mul6";
	aps["project1.vbox1.swin2.vp1.list1.item[5]"] = "末広";
	aps["project1.vbox1.swin2.vp1.list1.select[2]"] = "T";
	aps["project1.vbox1.swin2.vp1.list1.count"] = 6;

	aps["project1.vbox1.swin1.clist1.label1.value"] = "その１";
	aps["project1.vbox1.swin1.clist1.label1.style"] = "blue";
	aps["project1.vbox1.swin1.clist1.label2.value"] = "その２";
	aps["project1.vbox1.swin1.clist1.label2.style"] = "red";
	aps["project1.vbox1.swin1.clist1.label3.value"] = "その３";
	aps["project1.vbox1.swin1.clist1.label3.style"] = "green";

	aps["project1.vbox1.togglebutton1.label"] = "まだ";
	aps["project1.vbox1.togglebutton1.style"] = "green";
	aps["project1.vbox1.togglebutton1.value"] = "FALSE";

	aps["project1.vbox1.hbox5.checkleft.value"] = "FALSE";
	aps["project1.vbox1.hbox5.checkright.value"] = "FALSE";
	aps["project1.vbox1.hbox5.checknone.value"] = "FALSE";

	aps["project1.vbox1.hbox6.radioleft.value"] = "TRUE";
	aps["project1.vbox1.hbox6.radioright.value"] = "FALSE";
	aps["project1.vbox1.hbox6.radionone.value"] = "FALSE";

	aps["project1.vbox1.notebook1.pageno"] = 3;

	aps["spa.count"] = 0;
	aps["project1.vbox1.notebook1.swin3.text1.value"] = "ノートブックの中身";
	aps["project1.vbox1.notebook1.entry4.value"] = 1234;
	aps["project1.vbox1.notebook1.swin4.clist2.label14.value"] = "名前";
	aps["project1.vbox1.notebook1.swin4.clist2.label15.value"] = "電話番号";
	aps["project1.vbox1.notebook1.swin4.clist2.item[0].value1"] = "おごちゃん";
	aps["project1.vbox1.notebook1.swin4.clist2.item[0].value2"] = "090-3689-3151";
	aps["project1.vbox1.notebook1.swin4.clist2.count"] = 1;

	putWindow("project1","entry2");
  end
  def do_Left(aps)
	aps["project1.vbox1.entry3.value"] =
	  aps["project1.vbox1.entry1.value"] +
	  aps["project1.vbox1.entry2.value"];
	aps["project1.vbox1.hbox5.checkleft.value"] = "FALSE";

	putWindow("project1","entry2");
  end
  def do_Right(aps)
	aps["project1.vbox1.entry3.value"] =
	  aps["project1.vbox1.entry2.value"] +
	  aps["project1.vbox1.entry1.value"];
	aps["project1.vbox1.hbox5.checkright.value"] = "FALSE";

	putWindow("project1","entry2");
  end
  def do_OpenCalc(aps)
	putWindow("project2","entry2","NEW");
  end
  def do_CloseCalc(aps)
	putWindow("project2","entry2","CLOSE");
  end
  def do_Quit(aps)
	f = open("aps.log","a");
	f.printf("Quit\n");
	aps.dump(f);
	f.close;
	putWindow("project3","entry2","CHANGE");
  end
  def do_OpenCalendar(aps)
	putWindow("project5","entry2","CHANGE");
  end
  def do_PutData(aps)
	f = open("log","a");
	f.printf("PutData\n");
	aps.dump(f);
	f.close;

	aps["project1.vbox1.entry1.value"] = aps["demolink.linktext"];

	if aps["project1.vbox1.togglebutton1.value"] == "TRUE"
	  aps["project1.vbox1.swin1.clist1.item[#{aps["spa.count"]}].value1"] =
		aps["project1.vbox1.entry1.value"];
	  aps[format("project1.vbox1.swin1.clist1.item[%d].value2",aps["spa.count"])] =
		aps["project1.vbox1.entry2.value"];
	  aps[format("project1.vbox1.swin1.clist1.item[%d].value3",aps["spa.count"])] =
		aps["project1.vbox1.entry3.value"];
	  aps["spa.count"] = (aps["spa.count"].to_i + 1).to_s;
	  aps["project1.vbox1.swin1.clist1.count"] = aps["spa.count"];
	  aps["project1.vbox1.togglebutton1.label"] = "選んだ";
	else
	  aps["project1.vbox1.togglebutton1.label"] = "選んでない";
	end

	putWindow("project1","entry2");
  end
end

Main.new.run;
