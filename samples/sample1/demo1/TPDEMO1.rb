#!/usr/bin/ruby -Ke

require "apslib"

class Main < APS
  def start(aps)
	aps["demo1spa.count"] = 0;
	aps["project1.vbox1.entry1.value"] = "漢字を入れてみた。";
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
	aps["project1.vbox1.togglebutton1.value"] = "F";

	aps["project5.vbox1.calendar1.year"] = 2003;
	aps["project5.vbox1.calendar1.month"] =9;
	aps["project5.vbox1.calendar1.day"] = 3;
	putWindow("project1","entry2");
  end
  def do_Left(aps)
	aps["demo1spa.count"] = 0;
	aps["project1.vbox1.entry1.value"] = "左";
	aps["project1.vbox1.entry1.style"] = "green";
	aps["project1.vbox1.entry2.value"] = 12345.67;
	aps["project5.vbox1.calendar1.year"] = 2003;
	aps["project5.vbox1.calendar1.month"] =9;
	aps["project5.vbox1.calendar1.day"] = 3;
	putWindow("project1","entry2");
  end
  def do_Right(aps)
	aps["demo1spa.count"] = 0;
	aps["project1.vbox1.entry1.value"] = "右";
	aps["project1.vbox1.entry1.style"] = "red";
	aps["project1.vbox1.entry2.value"] = 12345.67;
	aps["project5.vbox1.calendar1.year"] = 2003;
	aps["project5.vbox1.calendar1.month"] =9;
	aps["project5.vbox1.calendar1.day"] = 3;
	putWindow("project1","entry2");
  end
  def do_PutData
  end
end

Main.new.run;
