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
	@values = Hash.new;
	@line.split("&").each { | elem |
	  e = elem.split("=");
	  @values[e[0]] = decode(e[1]);
	}
  end
  def pack
	str = "";
	@values.each { | key, value |
	  str += key + "=" + encode(value) + "&";
	}
	str.chop;
  end
  def dump(port, name = /.*/)
#	port.printf("line = [%s]\n",@line);
	@values.each { | key, value |
	  if  name =~ key
		port.printf("%s = [%s]\n",key,value);
	  end
	}
  end
end

class PandaDC < PandaCore
  def initialize
	@line = $stdin.gets.chomp;
	unPack;
  end
  def run
	case @values["mcparea.dc.status"]
	when "LINK"
	  start(self);
	when "PUTG"
	  name = "do_" + @values["mcparea.dc.event"];
	  if respond_to?(name)
		__send__(name, self);
	  end
	else
	end
	out;
  end
  def out
	str = pack;
	printf("%s\n",str);
	$stdout.flush;
#	f = open("sample.log","w");
#	f.printf("%s\n",str);
#	f.close;
  end
  def []=(name,value)
	@values[name] = value;
  end
  def [](name)
	@values[name];
  end
  def putWindow(name,widget = "" ,type = "")
	@values["mcparea.dc.window"] = name;
	@values["mcparea.dc.widget"] = widget;
	@values["mcparea.dc.puttype"] = type;
	@values["mcparea.dc.status"] = "PUTG";
	@values["mcparea.rc"] = 0;
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
	@values["dbctrl.rc"];
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
  end
  def []=(name,value)
	@values[@name+"."+name] = value;
  end
  def [](name)
	@values[@name+"."+name];
  end
  def execFunction(func, pname = "")
	str  = "dbctrl.rc=0&";
	str += "dbctrl.func=" + func + "&";
	str += "dbctrl.rname=" + @name + "&";
	str += "dbctrl.pname=" + pname + "&";
	str += pack;
	@db.fpDBW.printf("%s\n",str);
#$stderr.printf(">>%s\n",str.chop);
	@db.fpDBW.flush;
	@line = @db.fpDBR.gets.chomp;
	unPack;
	@values["dbctrl.rc"].to_i;
  end
end
