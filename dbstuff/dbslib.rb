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

class	DB_Server
	def get_event
	  msg = @s.gets.chomp
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

	def	initialize(host,port,user,pass)
	  if  port  ==  0
		port = 8013;
	  end
	  @host = host;
	  @port = port;
	  @s = TCPSocket.new(@host,@port);
	  @s.printf("%s %s %s stringe\n",VER,user,pass);
	  msg = @s.gets.chomp;
	  if  (  msg  =~  /^Error\: (.*?)$/  )
		printf("error: %s\n",$1);
		@s.close
	  else
		@values = Hash.new(nil);
	  end
	end
	def exec_data(rec)
	  if rec
		rec.each{ | name, value | @s.printf("%s: %s\n",name,encode(value.to_s)) }
	  end
	  @s.printf("\n")
	end
	def	dbops(func):protected
	  @s.printf("Exec: %s\n",func)
	  @s.printf("\n")
	  rc = get_event
	  @s.printf("\n")
	  rc;
	end
	def open
	  return dbops("DBOPEN")
	  rc;
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
	def	getValues(name,limit)
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
			break
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
	  return getValues(rname,limit);
	end
	def	close
		@s.printf("End\n");
		@s.close;
	end
end

