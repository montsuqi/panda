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
	  if  (  msg  =~  /^Error\: (.*?)$/  )
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
	def event_data
	  @values.each{ | name, value | @s.printf("%s: %s\n",name,value) };
	  @s.printf("\n");
	  msg = @s.gets.chomp;
	  if  (  msg  =~  /^Event\: (.*?)$/  )
		if  (  $1  !=  "OK"  )
		  printf("error: %s\n",$1);
		  @s.close
		end
	  else
		printf("error: connection lost ?\n");
		@s.close
	  end
	end
	def	event(event)
	  @s.printf("Event: %s\n",event);
	  event_data;
	end
	def	event2(event,widget)
	  @s.printf("Event: %s:%s\n",event,widget);
	  event_data;
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

