require	'socket';

VER="1.1.0";

class	DB_Server
	def get_event
	  msg = @s.gets.chomp;
	  if  (  msg  =~  /^Exec\: (.*?)$/  )
		rc = $1.to_i;
	  else
		printf("error: connection lost ?\n");
		@s.close
		rc = -1;
	  end
	  rc;
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
	  @s.printf("%s %s %s string\n",VER,user,pass);
	  msg = @s.gets.chomp;
	  if  (  msg  =~  /^Error\: (.*?)$/  )
		printf("error: %s\n",$1);
		@s.close
	  else
		@values = Hash.new(nil);
	  end
	end
	def exec_data(rec)
	  rec.each{ | name, value | @s.printf("%s: %s\n",name,encode(value)) };
	  @s.printf("\n");
	end
	def	dbops(func)
	  @s.printf("Exec: %s\n",func);
	  @s.printf("\n");
	  rc = get_event;
	  @s.printf("\n");
	  rc;
	end
	def	getValues(rec,name)
	  @s.printf("%s\n",name);
	  @s.flush;
	  while  is = @s.gets
		is.chomp!
printf("is = [%s]\n",is);
		break if  is  ==  "";
		dat = is.split(/: /);
		rec[dat[0]] = decode(dat[1]);
	  end
	  @s.printf("\n");
	  rec;
	end
	def	recordops(func,rname,pname,rec)
	  @s.printf("Exec: %s:%s:%s\n",func,rname,pname);
	  exec_data(rec)
	  rc = get_event;
	  getValues(rec,rname);
	  rc;
	end
	def	close
		@s.printf("End\n");
		@s.close;
	end
end

