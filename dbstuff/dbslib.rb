# PANDA -- a simple transaction monitor
# Copyright (C) 2001-2008  ogochan
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

require	'socket'

VER="1.4.0"
#VER="1.4.3"
VER="1.5.1"

class	DB_Server
	def get_event
	  msg = @s.gets
	  if  (  msg  =~  /^Exec\: (.*?)\:(.*?)$/  )
		rc = $1.to_i
		@count = $2.to_i
	  elsif  (  msg  =~  /^Exec\: (.*?)$/  )
		rc = $1.to_i
		@count = 1
	  else
		printf("error: connection lost ?\n")
		@s.close
		@count = 0
		rc = -1
	  end
	  return rc
	end
	def decode(string):protected
	  if  string
		string.tr('+', ' ').gsub(/((?:%[0-9a-fA-F]{2})+)/n) do
		  [$1.delete('%')].pack('H*')
		end
	  end
	end
	def encode(string):protected
	  if  string
		string.gsub(/([^ a-zA-Z0-9_.-]+)/n) do
		  '%' + $1.unpack('H2' * $1.size).join('%').upcase
		end.tr(' ', '+')
	  end
	end

	def	initialize(host,port,user,pass)
	  if  port  ==  0
		port = 8013;
	  end
	  @host = host;
	  @port = port;
	  @s = TCPSocket.new(@host,@port);
	  @s.printf("%s %s %s stringe\n",VER,user,pass);
	  msg = @s.gets.chomp;
	  if  ( msg  =~  /^Connect\: OK/  )
		@values = Hash.new(nil);
		if  (  msg  =~  /^Connect\: OK;(.*?)$/  )
		  ver = $1.split(/\./)
		  @version = ( ( ver[0].to_i * 100 ) + ver[1].to_i ) * 100 + ver[2].to_i
		else
		  @version = 10403
		end
	  elsif  (  msg  =~  /^Error\: (.*?)$/  )
		printf("error: %s\n",$1);
		@s.close
	  else
		@s.close
	  end
	end
	def exec_data(rec):protected
	  if rec
		rec.each{ | name, value | @s.printf("%s: %s\n",name,encode(value.to_s)) }
	  end
	  @s.printf("\n")
	end
	def	dbops(func):protected
	  @s.printf("Exec: %s\n",func)
	  @s.printf("\n")
	  rc = get_event
	  if  (  @version  <  10500  )
		@s.printf("\n")
	  end
	  rc;
	end
	def open
	  return dbops("DBOPEN")
	end
	def start
	  return dbops("DBSTART")
	end
	def commit
	  return dbops("DBCOMMIT")
	end
	def disconnect
	  return dbops("DBDISCONNECT")
	end
	def	getSchema(rname,pname)
	  @s.printf("Schema: %s:%s\n",rname,pname)
	  @s.gets.chomp
	end

	def getPathTables(rname = '')
	  @s.printf("PathTables: %s\n",rname)
	  pathes = Hash.new
	  while  is = @s.gets
		is.chomp!
		printf("is = [%s]\n",is)
		break if  (  is  ==  ''  )
		pathes[is] = is
	  end
	  return pathes
	end
	def	getValues(name,limit):protected
	  if  ( limit == 1 )
		@s.printf("%s\n",name)
		rec = nil
	  else
		@s.printf("%s;%d\n",name,limit)
		rec = Array.new
	  end
	  @s.flush
	  item = nil
	  while  is = @s.gets
		is.chomp!
		printf("is = [%s]\n",is)
		if  (  is  ==  ''  )
		  break if !item
		  if  (  @count  ==  1  )
			rec = item
		  else
			rec << item
		  end
		  item = nil
		else
		  if  !item
			item = Hash.new
		  end
		  dat = is.split(/: /)
		  var = dat[0].split(/;/)
#		  printf("name = [%s]\n",var[0])
		  item[var[0]] = decode(dat[1])
#		  printf("dat = [%s]\n",item[var[0]])
#		  printf("type = [%s]\n",var[1])
		end
	  end
	  @s.printf("\n")
	  @s.flush;
	  return rec
	end
	def	recordops(func,rname,pname,rec=nil,limit=1)
	  if  ( limit == 1 )
		@s.printf("Exec: %s:%s:%s\n",func,rname,pname);
	  else
		@s.printf("Exec: %s:%s:%s:%d\n",func,rname,pname,limit);
	  end
	  @s.flush;
	  exec_data(rec)
	  rc = get_event;
          printf("rc => #{rc}, count => #{@count}\n");
	  if  (  (  @version  >=  10500  ) && ( @count  ==  0  )  )
		ret = nil
	  else
            print "func => #{func}\n"
            if func == "DBSELECT" || func == "DBFETCH"
              ret = getValues(rname,limit);
            end
	  end
	  return ret
	end
	def	close
		@s.printf("End\n");
		@s.close;
	end
end

