#
#	Ruby interface for Exec handler
#

class APS
  def decode(string)
	if string.nil?
	  string = "";
	end
	if  string
	  string.tr('+', ' ').gsub(/((?:%[0-9a-fA-F]{2})+)/n) do
		[$1.delete('%')].pack('H*')
	  end
	end
  end
  def encode(val)
	if val.class == String
	  string = val;
	else
	  string = val.to_s;
	end
	if  string
	  string.gsub(/([^ a-zA-Z0-9_.-]+)/n) do
		'%' + $1.unpack('H2' * $1.size).join('%').upcase
	  end.tr(' ', '+')
	end
  end
  def initialize
	@line = $stdin.gets.chomp;
	@values = Hash.new;
	@line.split("&").each { | elem |
	  e = elem.split("=");
	  @values[e[0]] = decode(e[1]);
	}
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
  def dump(port, name = /.*/)
	port.printf("line = [%s]\n",@line);
	@values.each { | key, value |
	  if  name =~ key
		port.printf("%s = [%s]\n",key,value);
	  end
	}
  end
  def out
	str = "";
	@values.each { | key, value |
	  str += key + "=" + encode(value) + "&";
	}
	printf("%s\n",str.chop);
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
