#
#	Ruby interface for Exec handler
#

class APS
  def decode(string)
	if  string
	  string.tr('+', ' ').gsub(/((?:%[0-9a-fA-F]{2})+)/n) do
		[$1.delete('%')].pack('H*')
	  end
	end
  end
  def encode(string)
	if  string
	  string.gsub(/([^ a-zA-Z0-9_.-]+)/n) do
		'%' + $1.unpack('H2' * $1.size).join('%').upcase
	  end.tr(' ', '+')
	end
  end
  def initialize
	line = $stdin.gets.chomp;
	@values = Hash.new;
	line.split("&").each { | elem |
	  e = elem.split("=");
	  @values[e[0]] = decode(e[1]);
	}
  end
  def run
	case @values["mcparea.dc.status"]
	when "LINK"
	  start(@values);
	when "PUTG"
	  name = "do_" + @values["mcparea.dc.event"];
	  if respond_to?(name)
		__send__(name, @values);
	  end
	else
	end
	out;
  end
  def dump(port)
	@values.each { | key, value |
	  port.printf("%s = [%s]\n",key,value);
	}
  end
  def out
	str = "";
	@values.each { | key, value |
	  str += key + "=" + encode(sprintf("%s",value)) + "&";
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
