# PANDA -- a simple transaction monitor
# 
# Copyright (C) 1998-2005 Ogochan.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA

#
#	Ruby interface for Exec handler
#

DEBUG=false;

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
  def dbComm(str)
	if DEBUG
	  $stderr.printf("<<%s\n",str);
	end
	@fpDBW.printf("%s\n",str);
	@fpDBW.flush;
	line = @fpDBR.gets.chop;
	if DEBUG
	  $stderr.printf(">>%s\n",line);
	end
	line;
  end
  def initialize
	@fpDBR = IO.new(3,"r");
	@fpDBW = IO.new(4,"w");
  end
  def execFunction(func)
	str  = "dbctrl.rc=0&";
	str += "dbctrl.func=" + func;
	@line = dbComm(str);
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
	@line = @db.dbComm(str);
	unPack;
  end
  def getSchema(db,pname = "", func = "")
	@values = Hash.new;
	str  = "dbctrl.rc=%2D1&";
	str += "dbctrl.func=" + func + "&";
	str += "dbctrl.rname=" + @name + "&";
	str += "dbctrl.pname=" + pname;
	@line = @db.dbComm(str);
	unPack;
  end
  def execFunction(func, pname = "")
	str  = "dbctrl.rc=0&";
	str += "dbctrl.func=" + func + "&";
	str += "dbctrl.rname=" + @name + "&";
	str += "dbctrl.pname=" + pname + "&";
	str += pack;
	@line = @db.dbComm(str);
	unPack;
  end
end

class PandaDC < PandaCore
  def initialize
	line = $stdin.gets
	@line = line.chomp;
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
