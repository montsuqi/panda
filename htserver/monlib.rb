require	'socket';

class	HT_Server
	def	initialize(host,port,prog,user)
		@host = host;
		@port = port;
		@s = TCPSocket.new(@host,@port);
		@s.printf("Start: %s\t%s\n",prog,user);
		@sesid = @s.gets.chomp;
		self.windowname;
		@values = Hash.new(nil);
	end
	def	start
		@s = TCPSocket.new(@host,@port);
		@s.printf("Session: %s\n",@sesid);
		@sesid = @s.gets.chomp;
		@values.clear;
		@values = Hash.new(nil);
	end
	def	getValue(name)
		@s.printf("%s\n",name);
		@s.gets.chomp;
	end
	def	windowname
		while	win = @s.gets.chomp
			break	if	win.length == 0;
			if	win =~ /Window: (.*)/
				@win = $1;
			end
		end
	end
	def	event(event,widget)
		@s.printf("%s:%s\n",event,widget);
		@values.each{ | name, value | @s.printf("%s: %s\n",name,value) };
		@s.printf("\n");
		self.windowname;
	end
	def	getValue(name)
		@s.printf("%s\n",name);
		@s.gets.chomp;
	end
	def	setValue(name,value)
		@values[name] = value;
	end
	def	commit
		@s.printf("\n");
		@s.close;
	end
	def	close
		self.start;
		@s.printf("\n");
		@s.close;
	end
	def	ses
		@sesid;
	end
end

