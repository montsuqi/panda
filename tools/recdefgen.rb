#!/usr/bin/env ruby
require "xmlparser"
require "kconv"
include Kconv
require "uconv"
include Uconv

$button_enable = false     
if ARGV[0] == "-b" 
	$button_enable = true 
	ARGV.shift 
end       

def	putTab(n)
	while	n > 0
		printf("\t");
		n -= 1;
	end
end

class	Stack
	def	initialize
		@st = Array.new;
	end
	def	push(val)
		@st.push(val);
	end
	def	pop
		@st.pop;
	end
	def	top
		@st[@st.size-1];
	end
	def	body
		@st;
	end
end

class	Ssignal
	def	initizlize
		@handler = "";
		@name = "";
		@data = "";
	end
	def	name= (val)
		@name = val;
	end
	def	name
		@name;
	end
	def	handler= (val)
		@handler = val;
	end
	def	handler
		@handler;
	end
	def	data= (val)
		@data = val;
	end
	def	data
		@data;
	end
end

class	Widget
	def	initialize
		@child = Array.new;
		@child_name = "";
		@chars = 0;
		@klass = "";
		@name = "";
		@label = "";
		@format = "";
		@width = 0;
		@height = 0;
		@columns = 0;
		@column_width = Array.new;
		@align = "left";
		@signal = Array.new;
		@group = "";
	end
	def	child
		@child
	end
	def	child= (val)
		@child = val;
	end
	def	child_name
		@child_name
	end
	def	child_name= (val)
		@child_name = val;
	end
	def	klass= (val)
		@klass = val;
	end
	def	name= (val)
		@name = val;
	end
	def	name
		@name;
	end
	def	chars= (val)
		@chars = val;
	end
	def	chars
		@chars;
	end
	def	label
		@label
	end
	def	label= (val)
		@label = val;
	end
	def	align
		@align
	end
	def	align= (val)
		@align = val;
	end
	def	signal
		@signal;
	end
	def	signal= (val)
		@signal = val;
	end
	def	width
		@width;
	end
	def	width= (val)
		@width = val;
	end
	def	height
		@height;
	end
	def	height= (val)
		@height = val;
	end
	def	columns
		@columns;
	end
	def	columns= (val)
		@columns = val;
	end
	def	column_width
		@column_width;
	end
	def	column_width= (val)
		@column_width = val;
	end
	def	format
		@format
	end
	def	format= (val)
		@format = val;
	end
	def	group= (val)
		@group = val;
	end

	def	_windowName
		case	@klass
		  when	"GtkWindow"
			title = @name;
		  else
			title = "";
			for	c in @child
				title = c._windowName;
				break	if	title  !=  "";
			end
		end
		return	(title);
	end
	def	_title
		case	@klass
		  when	"GtkWindow"
			title = @label;
		  else
			title = "";
			for	c in @child
				title = c._title;
				break	if	title  !=  "";
			end
		end
		return	(title);
	end
	def	isData
		case	@klass
		  when	"GtkEntry", "GtkToggleButton", "GtkCheckButton", "GtkRadioButton", "GtkList", "GtkCalendar", "GtkNumberEntry", "GtkText", "GtkPandaEntry", "GtkPandaText" ,"GtkCList", "GtkPandaCList", "GtkPandaPS", "GnomePixmap"
			ret = TRUE;
		  when  "GtkLabel"
		  	if	@label == "" or
				@label =~ /^X+$/
			  ret = TRUE;
			end
		  else
			if	@child
				for	c in @child
					ret = c.isData;
					break	if	ret;
				end
			  else
				ret = FALSE;
			end
		end
		return	(ret);
	end
	def	_panda(ind)
		vname = @name;
		size = 0;
		case	@klass
		  when	"top-level"
			for	c in @child
				c._panda(ind);
			end
		  when	"GtkVBox", "GtkHBox", "GtkTable", "GtkFixed", "GtkScrolledWindow", "GtkWindow", "GtkViewport", "GtkNotebook", "GtkFrame", "GtkHandleBox", "GtkMenuBar", "GtkToolbar"
			if	self.isData
				putTab(ind);
				printf("%s\t{\n",vname);
				if		@klass  ==  "GtkNotebook"
					putTab(ind+1);
					printf("pageno\tint;\n");
				end
				for	c in @child
					c._panda(ind+1);
				end
				putTab(ind);
				printf("};\n");
			end
		  when	"GtkCList", "GtkPandaCList"
			putTab(ind);
			printf("%s\t{\n",vname);
			i = 0;
			for	c in @child
				c._panda(ind+1);
				i += 1;
			end

			putTab(ind+1);
			printf("count\tint;\n");
			putTab(ind+1);
			if @klass == "GtkPandaCList"
				printf("row\tint;\n");
				putTab(ind+1);
			end
			printf("item\t{\n");
			i = 0;
			for	c in @child
				putTab(ind+2);
				printf("value%d\tvarchar(%d);\n",i,Integer(self.column_width[i])/8);
				i += 1;
			end
			putTab(ind+1);
			printf("}\t[??];\n");
			putTab(ind+1);
			printf("select\tbool[??];\n");

			putTab(ind);
			printf("};\n");
		  when	"GtkList"
			putTab(ind);
			printf("%s\t{\n",vname);
			i = 0;
			for	c in @child
				c._panda(ind+1);
				i += 1;
			end
			putTab(ind+1);
			printf("count\tint;\n");
			putTab(ind+1);
			printf("item\tvarchar(??)[??];\n");
			putTab(ind+1);
			printf("select\tbool[??];\n");

			putTab(ind);
			printf("};\n");
		  when	"GtkOptionMenu"
			putTab(ind);
			printf("%s\t{\n",vname);
			i = 0;
			for	c in @child
				c._panda(ind+1);
				i += 1;
			end
			putTab(ind+1);
			printf("count\tint;\n");
			putTab(ind+1);
			printf("item\tvarchar(??)[??];\n");
			putTab(ind+1);
			printf("select\tint;\n");
			putTab(ind);
			printf("};\n");
		  when	"GtkCombo", "GtkPandaCombo"
			putTab(ind);
			printf("%s\t{\n",vname);
			@child[0]._panda(ind+1);
			putTab(ind+1);
			printf("count\tint;\n");
			putTab(ind+1);
			printf("item\tvarchar(%d)[??];\n",@child[0].chars);
			putTab(ind);
			printf("};\n");
		  when	"GtkLabel"
			if		@label  ==  "" or
					@label  =~  /^X+$/
				putTab(ind);
				printf("%s\t{\n",vname);
				putTab(ind+1);
				if	@label  ==  ""
					printf("value\tvarchar(%d);\n",Integer(self.width)/8);
				  else
					printf("value\tvarchar(%d);\n",label.length);
				end
				putTab(ind);
				printf("};\n");
			end
		  when	"GtkEntry", "GtkPandaEntry"
			putTab(ind);
			printf("%s\t{\n",vname);
			putTab(ind+1);
			printf("value\tvarchar(%d);\n",@chars);
			putTab(ind);
			printf("};\n");
		  when	"GtkText", "GtkPandaText"
			putTab(ind);
			printf("%s\t{\n",vname);
			putTab(ind+1);
			printf("value\tvarchar(??);\n");
			putTab(ind);
			printf("};\n");
		  when	"GtkNumberEntry"
			putTab(ind);
			printf("%s\t{\n",vname);
			putTab(ind+1);
			len = @format.tr(".,","").length;
			s = @format.split(".");
			if		s[1]  !=  nil
				slen = s[1].tr(",","").length;
				printf("value\tnumber(%d,%d);\n",len,slen);
			  else
				printf("value\tnumber(%d);\n",len);
			end;
			putTab(ind);
			printf("};\n");
		  when	"GtkToggleButton", "GtkCheckButton", "GtkRadioButton"
			putTab(ind);
			printf("%s\t{\n",vname);
			putTab(ind+1);
			printf("value\tbool;\n");
			if		@label  ==  ""
				putTab(ind+1);
				printf("label\tvarchar(??);\n");
			end
			putTab(ind);
			printf("};\n");
		  when  "GtkButton" 
			if	$button_enable 
				putTab(ind); 
				printf("%s\t{\n",vname); 
				putTab(ind+1); 
				printf("state int;\n"); 
				putTab(ind); 
				printf("};\n"); 
			end 
		  when  "GtkProgressBar" 
			putTab(ind); 
			printf("%s\t{\n",vname); 
			putTab(ind+1); 
			printf("value\tint;\n"); 
			putTab(ind); 
			printf("};\n"); 
		  when	"GtkCalendar"
			putTab(ind);
			printf("%s\t{\n",vname);
			putTab(ind+1);
			printf("year\tint;\n");
			putTab(ind+1);
			printf("month\tint;\n");
			putTab(ind+1);
			printf("day\tint;\n");
			putTab(ind);
			printf("};\n");
		  when  "GnomePixmap", "GtkPandaPS"
			putTab(ind);
			printf("%s\t{\n",vname);
			putTab(ind+1);
		        printf("%s\tobject;\n",vname);
			putTab(ind);
			printf("};\n");
		  else
			;
		end
	end
	def	panda
		self._panda(0);
	end
end

if ((xml = $<.gets).nil?); exit 1; end

if xml =~ /^<\?xml\sversion=.+\sencoding=.EUC-JP./i
	xml.sub!(/EUC-JP/i, "UTF-8")
	encoding = 'EUC-JP'
elsif xml =~ /^<\?xml\sversion=.+\sencoding=.Shift_JIS./i
	xml.sub!(/Shift_JIS/i, "UTF-8")
	encoding = "Shift_JIS"
elsif xml =~ /^<\?xml\sversion=.+\sencoding=.ISO-2022-JP./i
	xml.sub!(/ISO-2022-JP/i, "UTF-8")
	encoding = "ISO-2022-JP"
else
	xml.sub!(/EUC-JP/i, "UTF-8")
	encoding = 'EUC-JP'
end

xml += String($<.read)

if encoding == "EUC-JP"
	xml = euctou8(xml)
elsif encoding == "Shift_JIS"
	xml = euctou8(kconv(xml, EUC, SJIS))
elsif encoding == "ISO-2022-JP"
	xml = euctou8(kconv(xml, EUC, JIS))
end

parser = XMLParser.new

tag = Stack.new;
tree = Stack.new;

widget = Widget.new;
tree.push(widget);
widget.name = "top";
widget.klass = "top-level";

signal = nil;

begin
	current = 0;
	parser.parse(xml) do	|type, name, data|
		case	type
		  when	XMLParser::START_ELEM
			tag.push(name);
			case	name
			  when	/widget/i
				widget = Widget.new;
				tree.push(widget);
			  when	/signal/i
				signal = Ssignal.new;
			  else
				;
			end
		  when	XMLParser::END_ELEM
			tag.pop;
			case	name
			  when	/widget/i
				cur = tree.pop;
				widget = tree.top;
				widget.child << cur;
			  when	/signal/i
				widget.signal << signal;
			  else
				;
			end
		  when	XMLParser::CDATA
			next if data =~ /^\s*$/;
			data = Uconv.u8toeuc(data)
			path = "";
			for	t in tag.body
				path = path + "/" + t;
			end
			case	path
			  when	/\/class$/i
				widget.klass = data;
			  when	/\/widget\/name$/i
				widget.name = data;
			  when	/\/text_max_length$/i
				widget.chars = Integer(data);
			  when	/\/widget\/label$/i
				widget.label = data;
			  when	/\/widget\/title/i
				widget.label = data;
			  when	/\/widget\/width/i
				widget.width = Integer(data);
			  when	/\/widget\/height/i
				widget.height = Integer(data);
			  when	/\/widget\/columns/i
				widget.columns = Integer(data);
			  when	/\/widget\/format/i
				widget.format = data;
			  when	/\/widget\/group/i
				widget.group = data;
			  when	/\/widget\/child_name/i
				widget.child_name = data;
			  when	/\/widget\/column_width/i
				widget.column_width = data.split(",");

			  when	/\/widget\/justify/i
				case	data
				  when	/GTK_JUSTIFY_CENTER/i
					widget.align = "center";
				  when	/GTK_JUSTIFY_LEFT/i
					widget.align = "left";
				  when	/GTK_JUSTIFY_RIGHT/i
					widget.align = "right";
				  else
					;
				end
			  when	/\/signal\/name/i
				signal.name = data;
			  when	/\/signal\/handler/i
				signal.handler = data;
			  when	/\/signal\/data/i
				signal.data = data;
			  else
				;
			end
		  when	XMLParser::PI
		  else
			next if current == 0
			data.gsub!("\n", "\\n")
			printf("%s\n",Uconv.u8toeuc(data));
			current = 0
		end
	end
rescue XMLParserError
	line = parser.line
	column = parser.column
	printf("%s:%s:%d:%d:E:%s\n",$0,$<.filename,parser.line,parser.column,$!);
	exit 1
end

widget.panda;
exit 0
