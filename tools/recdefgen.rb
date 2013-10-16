#!/usr/bin/env ruby
# PANDA -- a simple transaction monitor
# Copyright (C) 2005-2008  ogochan
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

$button_enable = false   
if ARGV[0] == "-b" 
  $button_enable = true 
  ARGV.shift 
end     

def  putTab(n)
  while  n > 0
    printf("\t");
    n -= 1;
  end
end

class  Stack
  def  initialize
    @st = Array.new;
  end
  def  push(val)
    @st.push(val);
  end
  def  pop
    @st.pop;
  end
  def  top
    @st[@st.size-1];
  end
  def  body
    @st;
  end
end

class  Ssignal

  attr_accessor :name,:handler,:data

  def  initizlize
    @handler = "";
    @name = "";
    @data = "";
  end
end

class  Widget

  attr_accessor :child,:child_name,:chars,:klass,:name,:label,:format,
    :width,:height,:columns,:rows,:column_width,:align,:signal,:group

  def  initialize
    @child = Array.new;
    @child_name = "";
    @chars = 256;
    @klass = "";
    @name = "";
    @label = "";
    @format = "";
    @width = 0;
    @height = 0;
    @columns = 0;
    @rows = 0;
    @column_width = Array.new;
    @align = "left";
    @signal = Array.new;
    @group = "";
  end

  def  _windowName
    case  @klass
      when  "GtkWindow"
      title = @name;
      else
      title = "";
      for  c in @child
        title = c._windowName;
        break  if  title  !=  "";
      end
    end
    return  (title);
  end
  def  _title
    case  @klass
      when  "GtkWindow"
      title = @label;
      else
      title = "";
      for  c in @child
        title = c._title;
        break  if  title  !=  "";
      end
    end
    return  (title);
  end
  def  isData
    ret = FALSE
    case  @klass
      when  "GtkEntry", "GtkToggleButton", "GtkCheckButton", 
        "GtkRadioButton", "GtkList", "GtkCalendar", 
        "GtkNumberEntry", "GtkText", "GtkPandaEntry", 
        "GtkPandaText" ,"GtkCList", "GtkPandaCList", 
        "GtkPandaPS", "GnomePixmap", "GnomeFileEntry",
		"GtkFileChooserButton","GtkColorButton",
        "GtkPandaPDF", "GtkPandaDownload", "GtkPandaPrint","GtkPandaTable",
        "GtkWindow","GtkNotebook"
        ret = TRUE;
      when  "GtkLabel"
        if  @label == "" or
        @label =~ /^X+$/
        ret = TRUE;
      end
      else
      if  @child
        for  c in @child
          ret = c.isData;
          break  if  ret;
        end
        else
        ret = FALSE;
      end
    end
    return  (ret);
  end
  def  _panda(ind)
    vname = @name;
    size = 0;
    case  @klass
      when  "top-level"
      for  c in @child
        c._panda(ind);
      end
      when  "GtkVBox", "GtkHBox", "GtkTable", "GtkFixed", 
        "GtkScrolledWindow", "GtkViewport", "GtkNotebook", 
        "GtkFrame", "GtkHandleBox", "GtkMenuBar", "GtkToolbar"
        if  self.isData
          putTab(ind);
          printf("%s\t{\n",vname);
          if    @klass  ==  "GtkNotebook"
            putTab(ind+1);
            printf("pageno\tint;\n");
          end
          if    @klass  ==  "GtkScrolledWindow"
            putTab(ind+1); 
            printf("hpos\tint;\n"); 
            putTab(ind+1); 
            printf("vpos\tint;\n"); 
          end
          for  c in @child
            c._panda(ind+1);
          end
          putTab(ind);
          printf("};\n");
        end
      when "GtkWindow"
        if  self.isData
          putTab(ind);
          printf("%s\t{\n",vname);

        	putTab(ind+1);
        	printf("title\tvarchar(256);\n");
        	putTab(ind+1);
        	printf("popup_summary\tvarchar(256);\n");
        	putTab(ind+1);
        	printf("popup_body\tvarchar(1024);\n");
        	putTab(ind+1);
        	printf("popup_icon\tvarchar(256);\n");
        	putTab(ind+1);
        	printf("popup_timeout\tint;\n");

          for  c in @child
            c._panda(ind+1);
          end
          putTab(ind);
          printf("};\n");
        end
      when  "GtkCList", "GtkPandaCList"
        putTab(ind);
        printf("%s\t{\n",vname);
        i = 0;
        for  c in @child
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
        for  c in @child
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
      when  "GtkList"
        putTab(ind);
        printf("%s\t{\n",vname);
        i = 0;
        for  c in @child
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
      when  "GtkOptionMenu"
        putTab(ind);
        printf("%s\t{\n",vname);
        i = 0;
        for  c in @child
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
      when  "GtkCombo", "GtkPandaCombo"
        putTab(ind);
        printf("%s\t{\n",vname);
        @child[0]._panda(ind+1);
        putTab(ind+1);
        printf("count\tint;\n");
        putTab(ind+1);
        printf("item\tvarchar(%d)[??];\n",@child[0].chars);
        putTab(ind);
        printf("};\n");
      when  "GtkLabel"
      if    @label  ==  "" or
          @label  =~  /^X+$/
        putTab(ind);
        printf("%s\t{\n",vname);
        putTab(ind+1);
        if  @label  ==  ""
          printf("value\tvarchar(%d);\n",Integer(self.width)/8);
          else
          printf("value\tvarchar(%d);\n",label.length);
        end
        putTab(ind);
        printf("};\n");
      end
      when  "GtkEntry", "GtkPandaEntry"
        putTab(ind);
        printf("%s\t{\n",vname);
        putTab(ind+1);
        printf("value\tvarchar(%d);\n",@chars);
        putTab(ind);
        printf("};\n");
      when  "GtkText", "GtkPandaText"
        putTab(ind);
        printf("%s\t{\n",vname);
        putTab(ind+1);
        printf("value\tvarchar(??);\n");
        putTab(ind);
        printf("};\n");
      when  "GtkNumberEntry"
        putTab(ind);
        printf("%s\t{\n",vname);
        putTab(ind+1);
        len = @format.tr(".,","").length;
        s = @format.split(".");
        if    s[1]  !=  nil
          slen = s[1].tr(",","").length;
          printf("value\tnumber(%d,%d);\n",len,slen);
          else
          printf("value\tnumber(%d);\n",len);
        end;
        putTab(ind);
        printf("};\n");
      when  "GtkToggleButton", "GtkCheckButton", "GtkRadioButton"
        putTab(ind);
        printf("%s\t{\n",vname);
        putTab(ind+1);
        printf("value\tbool;\n");
        if    @label  ==  ""
          putTab(ind+1);
          printf("label\tvarchar(??);\n");
        end
        putTab(ind);
        printf("};\n");
      when  "GtkButton" 
      if  $button_enable 
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
      when  "GtkCalendar"
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
      when  "GnomePixmap", "GtkPandaPS", "GtkPandaPDF"
        putTab(ind);
        printf("%s\t{\n",vname);
        putTab(ind+1);
          printf("objectdata\tobject;\n");
        putTab(ind);
        printf("};\n");
      when  "GtkPandaDownload"
        putTab(ind);
        printf("%s\t{\n",vname);
          putTab(ind+1);
          printf("objectdata\tobject;\n");
          putTab(ind+1);
          printf("filename\tvarchar(%d);\n",@chars);
          putTab(ind+1);
          printf("description\tvarchar(%d);\n",@chars);
        putTab(ind);
        printf("};\n");
      when  "GtkPandaPrint"
        putTab(ind);
        printf("%s\t{\n",vname);
          putTab(ind+1);
          printf("item {\n");
            putTab(ind+2);
            printf("url\tvarchar(%d);\n",@chars);
            putTab(ind+2);
            printf("title\tvarchar(%d);\n",@chars);
            putTab(ind+2);
            printf("nretry\tint;\n");
            putTab(ind+2);
            printf("showdialog\tint;\n");
          putTab(ind+1);
          printf("}[10];\n");
        putTab(ind);
        printf("};\n");
      when  "GnomeFileEntry"
        putTab(ind);
        printf("%s\t{\n",vname);
          putTab(ind+1);
          printf("objectdata\tobject;\n");
            putTab(ind+1);
            printf("value\tvarchar(%d);\n",@chars);
        putTab(ind);
        printf("};\n");
      when  "GtkFileChooserButton"
        putTab(ind);
        printf("%s\t{\n",vname);
          putTab(ind+1);
          printf("objectdata\tobject;\n");
          putTab(ind+1);
          printf("filename\tvarchar(%d);\n",@chars);
        putTab(ind);
        printf("};\n");
      when  "GtkColorButton"
        putTab(ind);
        printf("%s\t{\n",vname);
          putTab(ind+1);
          printf("color\tvarchar(%d);\n",@chars);
        putTab(ind);
        printf("};\n");
      when  "GtkPandaTable"
        putTab(ind);
        printf("%s\t{\n",vname);

        putTab(ind+1);
        printf("state\tint;\n");
        putTab(ind+1);
        printf("style\tvarchar(%d);\n",@chars);
        putTab(ind+1);
        printf("trow\tint;\n");
        putTab(ind+1);
        printf("trowattr\tint;\n");
        putTab(ind+1);
        printf("tcolumn\tint;\n");
        putTab(ind+1);
        printf("tvalue\tvarchar(%d);\n",@chars);
        putTab(ind+1);
        printf("rowdata {\n");
        @columns.times{|i|
          putTab(ind+2);
          printf("column%d {\n",i+1);
            putTab(ind+3);
            printf("celldata\tvarchar(%d);\n",@chars);
            putTab(ind+3);
            printf("fgcolor\tvarchar(%d);\n",@chars);
            putTab(ind+3);
            printf("bgcolor\tvarchar(%d);\n",@chars);
          putTab(ind+2);
          printf("};\n");
        }
        putTab(ind+1);
        printf("}[n_rows];\n");

        putTab(ind);
        printf("};\n");
      else
      ;
    end
  end
  def  panda
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
elsif xml =~ /^<\?xml\sversion=.+\sencoding=.UTF-8./i
  encoding = "UTF-8"
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
  parser.parse(xml) do  |type, name, data|
    case  type
      when  XMLParser::START_ELEM
      tag.push(name);
      case  name
        when  /widget/i
        widget = Widget.new;
        tree.push(widget);
        when  /signal/i
        signal = Ssignal.new;
        else
        ;
      end
      when  XMLParser::END_ELEM
      tag.pop;
      case  name
        when  /widget/i
        cur = tree.pop;
        widget = tree.top;
        widget.child << cur;
        when  /signal/i
        widget.signal << signal;
        else
        ;
      end
      when  XMLParser::CDATA
      next if data =~ /^\s*$/;
      data = Uconv.u8toeuc(data)
      path = "";
      for  t in tag.body
        path = path + "/" + t;
      end
      case  path
        when  /\/class$/i
        widget.klass = data;
        when  /\/widget\/name$/i
        widget.name = data;
        when  /\/text_max_length$/i
        widget.chars = Integer(data);
        when  /\/max_length$/i
        widget.chars = Integer(data);
        when  /\/widget\/label$/i
        widget.label = data;
        when  /\/widget\/title/i
        widget.label = data;
        when  /\/widget\/width/i
        widget.width = Integer(data);
        when  /\/widget\/height/i
        widget.height = Integer(data);
        when  /\/widget\/columns/i
        widget.columns = Integer(data);
        when  /\/widget\/rows/i
        widget.rows = Integer(data);
        when  /\/widget\/format/i
        widget.format = data;
        when  /\/widget\/group/i
        widget.group = data;
        when  /\/widget\/child_name/i
        widget.child_name = data;
        when  /\/widget\/column_width/i
        widget.column_width = data.split(",");

        when  /\/widget\/justify/i
        case  data
          when  /GTK_JUSTIFY_CENTER/i
          widget.align = "center";
          when  /GTK_JUSTIFY_LEFT/i
          widget.align = "left";
          when  /GTK_JUSTIFY_RIGHT/i
          widget.align = "right";
          else
          ;
        end
        when  /\/signal\/name/i
        signal.name = data;
        when  /\/signal\/handler/i
        signal.handler = data;
        when  /\/signal\/data/i
        signal.data = data;
        else
        ;
      end
      when  XMLParser::PI
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
