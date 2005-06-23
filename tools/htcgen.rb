# PANDA -- a simple transaction monitor
# Copyright (C) 2000-2005  ogochan
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

require "xmlparser"
require "kconv"
include Kconv
require "uconv"
include Uconv

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
	attr_accessor :name, :handler, :data
	def	initizlize
		@handler = "";
		@name = "";
		@data = "";
	end
end

class	Widget
	attr_accessor :child, :child_name, :klass, :name, :chars, :label, :align, :signal,
	:width, :height, :columns, :columns_width, :format, :group
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
		  when	"GtkLabel", "GtkEntry", "GtkToggleButton", "GtkCheckButton", "GtkRadioButton", "GtkList", "GtkCList", "GtkCalendar", "GtkNumberEntry", "GtkText"
			ret = TRUE;
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
	def	_htc(vname)
	  if $v2
		vname = @name
	  else
		vname = vname + @name;
	  end
	  case	@klass
	  when	"top-level"
		for	c in @child
		  c._htc("");
		end
	  when	"GtkWindow"
		for	c in @child
		  c._htc(vname+".");
		end
	  when	"GtkVBox"
		printf("<TABLE>\n");
		for	c in @child
		  printf("<TR>\n");
		  c._htc(vname+".");
		  printf("</TR>\n");
		end
		printf("</TABLE>\n");
	  when	"GtkHBox"
		printf("<TABLE><TR>\n");
		for	c in @child
		  c._htc(vname+".");
		end
		printf("</TR></TABLE>\n");
	  when	"GtkEntry", "GtkPandaEntry"
		printf("<TD><entry name=\"%s.value\" size=%d maxlength=%d></TD>\n",
			   vname, @chars, @chars);
	  when	"GtkNumberEntry"
		printf("<TD><entry name=\"%s.value\" size=%d maxlength=%d></TD>\n",
			   vname, @format.length+2, @format.length+2);
	  when	"GtkText", "GtkPandaText"
		printf("<TD><text name=\"%s.value\"></TD>\n",
			   vname);
	  when	"GtkCombo", "GtkPandaCombo"
		printf("<COMBO count=\"%s.count\" name=\"%s.value\" item=\"%s.item\"><BR>\n",
			   vname, vname+"."+@child[0].name, vname);
	  when	"GtkButton"
		data = "";
		for	s in @signal
		  if	(  s.name     ==  "clicked"     )	&&
			  (  s.handler  ==  "send_event"  )
			data = s.data;
		  end
		end
		if data != ""
		  printf("<TD><button event=\"%s:%s\" face=\"%s\"></TD>\n",
				 data, @name, @label);
		end
	  when	"GtkToggleButton"
		printf("<TD><togglebutton name=\"%s.value\" label=\"%s.label\"></TD>\n",
			   vname, vname);
	  when	"GtkCheckButton"
		printf("<TD><CHECKBUTTON name=\"%s.value\" label=\"%s\"></TD>\n",
			   vname, @label);
	  when	"GtkRadioButton"
		printf("<TD><RADIOBUTTON group=\"%s\" name=\"%s.value\" label=\"%s\"></TD>\n",
			   @group, vname, @label);
	  when	"GtkCalendar"
		printf("<TD><CALENDAR year=\"%s.year\" month=\"%s.month\" day=\"%s.day\">\n",
			   vname, vname, vname);
	  when	"GtkCList", "GtkPandaCList"
		printf("<TABLE BORDER>\n");
		printf("<TR>\n");
		for	c in @child
		  if	@label == ""
			printf("<TD align=\"%s\"><fixed name=\"%s.value\"></TD>\n",
				   c.align, vname+"."+c.name);
		  else
			printf("<TD align=\"%s\">%s</TD>\n", c.align, c.label);
		  end
		end
		printf("<COUNT var=\"i\" from=0 to=\"%s.count\">\n",vname);
		printf("<TR>\n");
		i = 1;
		for	c in @child
		  printf("<TD align=\"%s\"><fixed name=\"%s.item[#i].value%d\"></TD>\n",
				 @align, vname,i);
		  i += 1;
		end
		printf("</TR>\n");
		printf("</COUNT>\n");
		printf("</TABLE>\n");
	  when	"GtkList"
		printf("<TABLE BORDER>\n");
		printf("<COUNT var=\"i\" from=0 to=\"%s.count\">\n",vname);
		printf("<TR>\n");
		printf("<TD><TOGGLEBUTTON name=\"%s.select[#i]\" label=\"%s.item[#i]\">\n",
			   vname,vname);
		printf("</TR>\n");
		printf("</COUNT>\n");
		printf("</TABLE>\n");
	  when	"GtkLabel"
		if	@child_name != ""
		  printf("<H2>");
		end
		if	@label == ""
		  printf("<fixed name=\"%s.value\"></TD>\n",vname);
		else
		  printf("%s\n", @label);
		end
		if	@child_name != ""
		  printf("</H2>");
		end
	  when	"GtkNotebook"
		temp = Array.new;
		for	c in @child
		  if	c.child_name  !=  ""
			printf("<HR>");
			printf("<H2>%s</H2>\n",c.label);
			for	d in temp
			  d._htc(vname+".");
			end
			temp = Array.new;
		  else
			temp << c;
		  end
		end
	  else
		for	c in @child
		  c._htc(vname+".");
		end
	  end
	end
	def	htc
		myname = File.basename($<.filename,".glade");
		title = self._title;
		print <<__EOF__
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN"
 "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<meta name="robots" content="noindex,nofollow">
<meta http-equiv="Content-Type" content="text/html; charset=euc-jp">
__EOF__
		printf("<TITLE>%s</TITLE>\n</HEAD>\n",title);
		print <<__EOF__
<BODY TEXT=\"#202020\" BGCOLOR=\"#C0C0C0\" LINK=\"#00FFFF\" VLINK=\"#00BBBB\">
<form>
__EOF__
		printf("<H1>%s</H1>\n",title);
		self._htc("");
		print <<__EOF__
</form>
</BODY>
</HTML>
__EOF__
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

widget.htc;
exit 0
