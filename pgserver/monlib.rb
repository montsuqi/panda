require	'socket';

VER="1.0.7";

class	PG_Server
	def	initialize(host,port,prog,user,pass)
	  if  port  ==  0
		port = 8011;
	  end
	  @host = host;
	  @port = port;
	  @s = TCPSocket.new(@host,@port);
	  @s.printf("Start: %s %s %s %s\n",VER,user,pass,prog);
	  msg = @s.gets.chomp;
	  printf("return [%s]\n",msg);
	  if  (  msg  ==  /^Error: (.*?)/  )
		printf("error: %s\n",$1);
		@s.close
	  else
		@values = Hash.new(nil);
		@s.printf("\n");
	  end
	end
	def	getValue(name)
		@s.printf("%s\n",name);
		@s.gets.chomp;
	end
	def	event(event)
		@s.printf("Event: %s\n",event);
		@values.each{ | name, value | @s.printf("%s: %s\n",name,value) };
		@s.printf("\n");
	end
	def	event2(event,widget)
		@s.printf("Event: %s:%s\n",event,widget);
		@values.each{ | name, value | @s.printf("%s: %s\n",name,value) };
		@s.printf("\n");
	end
	def	getValue(name)
		@s.printf("%s\n",name);
		@s.gets.chomp;
	end
	def	setValue(name,value)
		@values[name] = value;
	end
	def	close
		@s.printf("End\n");
		@s.close;
	end
end

