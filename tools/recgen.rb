#!/usr/bin/env ruby
# PANDA -- a simple transaction monitor
# Copyright (C) 2005-2007  ogochan
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
require "getoptlong"

$button_enable = false
$text_enable = false
$notab_enable = false

def usage
  $stderr.puts "Usage: #{File.basename($0)} [OPTIONS] [FILE]"
  $stderr.puts "画面定義体からプロトコルv2用のPL定義体を生成する。"
  $stderr.puts ""
  $stderr.puts "OPTIONS:"
  $stderr.puts "  -b, --button GtkButtonに関するPL定義体の出力を有効にする"
  $stderr.puts "  -t, --text   varchar型をtextとして出力する"
  $stderr.puts "  -n, --notab  インデントに4個の空白を用いる"
  $stderr.puts "  -e, --emacs  インデントに4個の空白を用い、"
  $stderr.puts "               Emacs用に末尾にLocal Variablesを追加する"
  $stderr.puts "  -h, --help   このメッセージを出力して終了する"
end

parser = GetoptLong.new
parser.set_options(['--button', '-b', GetoptLong::NO_ARGUMENT],
                   ['--text',   '-t', GetoptLong::NO_ARGUMENT],
                   ['--notab',  '-n', GetoptLong::NO_ARGUMENT],
                   ['--emacs',  '-e', GetoptLong::NO_ARGUMENT],
                   ['--help',   '-h', GetoptLong::NO_ARGUMENT])
begin
  parser.each_option do |name, arg|
    case name
    when "--button"
      $button_enable = true 
    when "--text"
      $text_enable = true 
    when "--notab"
      $notab_enable = true 
    when "--emacs"
      $emacs_enable = true 
      $notab_enable = true 
    when "--help"
      usage
      exit
    end
  end
rescue
  usage
  exit(1)
end

def	putTab(n)
	while	n > 0
        if $notab_enable
          printf("    ");
        else
          printf("\t");
        end
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

class	Element
	@@db = Hash.new;
	attr_accessor :name, :type, :klass, :item, :dim
	def	initialize(name)
		@name = name;
		@klass = :record;
		@type = "";
		@dim = Array.new(0);
		@item = Hash.new;
		@item_ary = []
	end
	def	out(ind)
		case	@klass
		  when	:record
			putTab(ind);
            if $notab_enable
              printf("%s {\n",@name);
            else
              printf("%s\t{\n",@name);
            end
			@item_ary.each do |key|
				@item[key].out(ind+1)
			end
			putTab(ind);
			printf("}");
		  else
			putTab(ind);
            if $notab_enable
              printf("%s %s",@name,@type);
            else
              printf("%s\t%s",@name,@type);
            end
		end
		if	@dim.size > 0
			for	i in @dim
				if		i  ==  0
					printf("[??]");
				  else
					printf("[%d]",i+1);
				end
			end
		end
		printf(";\n");
	end
	def	_append(name, type);
		ename = name[0];
		e = ename.split(/\[|\]\[|\]/);
		body = e[0];
		if	self.item.member?(body)
			el = self.item[body];
			if	e.size > 1
				if	el.dim.size != ( e.size - 1 )
					printf("array dimension not match. [%s]\n",body);
				  else
					i = 0;
					while i < el.dim.size
						if el.dim[i] < e[i+1].to_i
							el.dim[i] = e[i+1].to_i
						end
						i += 1;
					end
				end
			end
		  else
			el = Element.new(body);
			if	e.size > 1
				j = 1;
				while	j < e.size
					el.dim << e[j].to_i;
					j += 1;
				end
			end
			self.item.store(body,el);
			@item_ary.push(body)
		end
		if		name.size  ==  1
			el.type = type;
			if	type != ""
				el.klass = :data;
			end
		  else
			name.shift;
			el._append(name,type);
		end
		self;
	end
	def	append(name, type)
		self._append(name.split("."),type);
	end
end

class	Gsignal
	attr_accessor	:name, :handler, :data
	def	initizlize
		@handler = "";
		@name = "";
		@data = "";
	end
end

class	Widget
	attr_accessor :child, :child_name, :klass, :name, :chars, :label, :align, :signal, :width, :height, :columns, :column_width, :format
	@@record = nil;
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
	end

	def	isData
		case	@klass
		  when	"GtkEntry", "GtkToggleButton", "GtkCheckButton", "GtkRadioButton", "GtkList", "GtkCalendar", "GtkNumberEntry", "GtkText", "GtkPandaEntry", "GtkPandaText" ,"GtkCList", "GtkPandaCList", "GtkScrolledWindow"
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

	def	_panda
		case	@klass
		  when	"top-level", "GtkVBox", "GtkHBox", "GtkTable", "GtkFixed", "GtkViewport", "GtkHandleBox", "GtkMenuBar", "GtkToolbar"
			for	c in @child
				c._panda;
			end
		  when	"GtkWindow"
			@@record = Element.new(@name);
			for	c in @child
				c._panda;
			end
			@@record.out(0);
		  when	"GtkNotebook"
			if	self.isData
				@@record = @@record.append(sprintf("%s.pageno",@name),"int");
				for	c in @child
					c._panda;
				end
			end
		  when	"GtkCList", "GtkPandaCList"
			@@record = @@record.append(sprintf("%s.count",@name),"int");
            if @klass == "GtkPandaCList"
              @@record = @@record.append(sprintf("%s.from",@name),"int");
            end
			for	c in @child
				c._panda;
			end
			i = 0;
			for	c in @child
                l = Integer(self.column_width[i]) / 8
                t = "text"
                t = sprintf("varchar(%d)", l) if !$text_enable || l > 0
				@@record = @@record.append(sprintf("%s.item[0].value%d",@name,i), t)
				i += 1;
			end
			@@record = @@record.append(sprintf("%s.select[0]",@name),"bool");
		  when	"GtkList"
			i = 0;
			for	c in @child
				c._panda;
				i += 1;
			end
			@@record = @@record.append(sprintf("%s.count",@name),"int");
            t = "text"
            t = "varchar(??)" unless $text_enable
			@@record = @@record.append(sprintf("%s.item[0]",@name), t)
			@@record = @@record.append(sprintf("%s.select[0]",@name),"bool");
		  when	"GtkCombo", "GtkPandaCombo"
			@child[0]._panda;
			@@record = @@record.append(sprintf("%s.count",@name),"int");
            l = @child[0].chars
            t = "text"
            t = sprintf("varchar(%d)", l) if !$text_enable || l > 0
			@@record = @@record.append(sprintf("%s.item[0]",@name), t)
		  when	"GtkLabel"
			if		@label  ==  "" or
					@label  =~  /^X+$/
                l = label.length
                l = Integer(self.width)/8 if @label == ""
                t = "text"
                t = sprintf("varchar(%d)", l) if !$text_enable || l > 0
                @@record = @@record.append(sprintf("%s.value",@name), t)
			end
		  when	"GtkEntry", "GtkPandaEntry", "GtkText", "GtkPandaText"
            t = "text"
            t = sprintf("varchar(%d)", @chars) if !$text_enable || @chars > 0
			@@record = @@record.append(sprintf("%s.value",@name), t)
		  when	"GtkNumberEntry"
			len = @format.gsub(/\.,/,"").length;
			s = @format.split(".");
			if		s[1]  !=  nil
				slen = s[1].gsub(",","").length;
				@@record = @@record.append(sprintf("%s.value",@name),
											sprintf("number(%d,%d)",len,slen));
			  else
				@@record = @@record.append(sprintf("%s.value",@name),
											sprintf("number(%d)",len));
			end;
		  when	"GtkToggleButton", "GtkCheckButton", "GtkRadioButton"
			@@record = @@record.append(sprintf("%s.value",@name),"bool");
			if		@label  ==  ""
              t = "text"
              t = "varchar(??)" unless $text_enable
              @@record = @@record.append(sprintf("%s.label",@name), t)
			end
		  when  "GtkButton" 
			if	$button_enable 
			  @@record = @@record.append(sprintf("%s.state",@name),"int");
			  if @label =~ /^X+$/
                l = label.length
                t = "text"
                t = sprintf("varchar(%d)", l) if !$enable_text || l > 0
				@@record = @@record.append(sprintf("%s.label",@name), t)
			  end
			end 
		  when  "GtkFrame" 
			if @label =~ /^X+$/
			  @@record = @@record.append(sprintf("%s.label",@name),
										   sprintf("varchar(%d)",label.length));
			end
			for	c in @child
				c._panda;
			end
		  when  "GtkProgressBar" 
			@@record = @@record.append(sprintf("%s.value",@name),"int");
		  when  "GtkScrolledWindow"
			@@record = @@record.append(sprintf("%s.hpos",@name),"int");
			@@record = @@record.append(sprintf("%s.vpos",@name),"int");
			for	c in @child
				c._panda;
			end
		  when	"GtkOptionMenu"
			@@record = @@record.append(sprintf("%s.count",@name),"int");
			@@record = @@record.append(sprintf("%s.select",@name),"int");
			@@record = @@record.append(sprintf("%s.item[0]",@name),"varchar(??)");
		  when	"GtkCalendar"
			@@record = @@record.append(sprintf("%s.year",@name),"int");
			@@record = @@record.append(sprintf("%s.month",@name),"int");
			@@record = @@record.append(sprintf("%s.day",@name),"int");
          when	"GtkPandaPS"
            t = "text"
            t = "varchar(??)" unless $text_enable
            @@record = @@record.append(sprintf("%s.value",@name),t)
		  else
			;
		end
	end
	def	panda
		self._panda;
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

begin
	signal = nil;
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
				signal = Gsignal.new;
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

widget = Glade.new;

widget.panda;

if $emacs_enable
  puts ""
  puts "# Local Variables:"
  puts "# indent-tabs-mode: nil"
  puts "# tab-width: 4"
  puts "# End:"
end

exit 0

# Local Variables:
# tab-width: 4
# End:
