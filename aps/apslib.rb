# PANDA -- a simple transaction monitor
# 
# Copyright (C) 1998-1999 Ogochan.
#               2000-2003 Ogochan & JMA (Japan Medical Association).
# 
# This module is part of PANDA.
# 
# 	PANDA is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
# to anyone for the consequences of using it or for whether it serves
# any particular purpose or works at all, unless he says so in writing.
# Refer to the GNU General Public License for full details. 
# 
# 	Everyone is granted permission to copy, modify and redistribute
# PANDA, but only under the conditions described in the GNU General
# Public License.  A copy of this license is supposed to have been given
# to you along with PANDA so you can know your rights and
# responsibilities.  It should be in a file named COPYING.  Among other
# things, the copyright notice and this notice must be preserved on all
# copies. 

#
#	Ruby interface for Exec handler
#

class PandaCore
  def decode(string)
	if  string
	  string.tr('+', ' ').gsub(/((?:%[0-9a-fA-F]{2})+)/n) do
		[$1.delete('%')].pack('H*')
	  end
	else
	  ""
	end
  end
  def encode(val)
	if  val
	  if val.class == String
		string = val;
	  else
		string = val.to_s;
	  end
	  string.gsub(/([^ a-zA-Z0-9_.-]+)/n) do
		'%' + $1.unpack('H2' * $1.size).join('%').upcase
	  end.tr(' ', '+')
	else
	  ""
	end
  end
  def unPack
	rc = 0;
	@values = Hash.new;
	@line.split("&").each { | elem |
	  e = elem.split("=");
	  if e[0] =~ /^dbctrl/
		if e[0] == "dbctrl.rc"
		  rc = decode(e[1]).to_i;
		end
	  else
		@values[e[0]] = decode(e[1]);
	  end
	}
	rc;
  end
  def pack
	str = "";
	@values.each { | key, value |
	  str += key + "=" + encode(value) + "&";
	}
	str.chop;
  end
  def dump(port, name = /.*/)
	@values.each { | key, value |
	  if  name =~ key
		port.printf("%s = [%s]\n",key,value);
	  end
	}
  end
  def []=(name,value)
	if @values[name]
	  @values[name] = value;
	else
	  $stderr.printf("[%s] undefined APS variable\n",name);
	end
  end
  def [](name)
	if @values[name]
	  @values[name];
	else
	  $stderr.printf("[%s] undefined APS variable\n",name);
	  "";
	end
  end
  def clear(rec)
	@values.each { | key, value |
	  if key =~ rec
		@values[key] = "";
	  end
	}
  end
end

class PandaDB < PandaCore
  def initialize
	@fpDBR = IO.new(3,"r");
	@fpDBW = IO.new(4,"w");
  end
  def execFunction(func)
	str  = "dbctrl.rc=0&";
	str += "dbctrl.func=" + func;
	@fpDBW.printf("%s\n",str);
#$stderr.printf("%s\n",str);
	@fpDBW.flush;
	@line = @fpDBR.gets.chomp;
	unPack;
  end
  def fpDBR
	@fpDBR
  end
  def fpDBW
	@fpDBW
  end
end

class PandaTable < PandaDB
  def initialize(db,name)
	@name = name;
	@db = db;
	@values = Hash.new;
	str  = "dbctrl.rc=0&";
	str += "dbctrl.rname=" + @name;
	@db.fpDBW.printf("%s\n",str);
#$stderr.printf(">>%s\n",str);
	@db.fpDBW.flush;
	@line = @db.fpDBR.gets.chop;
	unPack;
  end
  def execFunction(func, pname = "")
	str  = "dbctrl.rc=0&";
	str += "dbctrl.func=" + func + "&";
	str += "dbctrl.rname=" + @name + "&";
	str += "dbctrl.pname=" + pname + "&";
	str += pack;
	@db.fpDBW.printf("%s\n",str);
#$stderr.printf(">>%s\n",str);
	@db.fpDBW.flush;
	line = @db.fpDBR.gets;
#$stderr.printf("<<%s",line);$stderr.flush;
	@line = line.chop;
	unPack;
  end
end

class PandaDC < PandaCore
  def initialize
	@line = $stdin.gets.chomp;
	@db = PandaDB.new;
	unPack;
  end
  def run
	case @values["mcparea.dc.status"]
	when "LINK"
	  start(self, @db);
	when "PUTG"
	  name = "do_" + @values["mcparea.dc.event"];
	  if respond_to?(name)
		__send__(name, self, @db);
	  end
	else
	end
	out;
  end
  def out
	str = pack;
	printf("%s\n",str);
	$stdout.flush;
  end
  def putWindow(name,widget = "" ,type = "")
	@values["mcparea.dc.window"] = name;
	@values["mcparea.dc.widget"] = widget;
	@values["mcparea.dc.puttype"] = type;
	@values["mcparea.dc.status"] = "PUTG";
	@values["mcparea.rc"] = 0;
  end
  WidgetNormal = 0;
  WidgetActive = 1;
  WidgetPreLight = 2;
  WidgetSelected = 3
  WidgetInSensitive = 4;
end

class PandaBatch < PandaCore
  def initialize
	@db = PandaDB.new;
  end
  def run
	start(@db);
  end
end
