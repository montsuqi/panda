# PANDA -- a simple transaction monitor
# Copyright (C) 2004-2008  ogochan
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

require	'monlib';

server = PG_Server.new("localhost",0,"panda:sample","ogochan",";suPer00");

server.setValue("list.fixed1.key.value","ogo%");

ev = server.event("Search");

printf("ev[0] = [%s]\n",ev[0]);
printf("ev[1] = [%s]\n",ev[1]);

count = server.getValue("list.fixed1.clist1.count").to_i
printf("count = %d\n",count);

i = 0;
while i < count
  printf("[%s][%s][%s][%s]\n",
		 server.getValue(sprintf("list.fixed1.clist1.item[%d].value1",i)),
		 server.getValue(sprintf("list.fixed1.clist1.item[%d].value2",i)),
		 server.getValue(sprintf("list.fixed1.clist1.item[%d].value3",i)),
		 server.getValue(sprintf("list.fixed1.clist1.item[%d].value4",i)));
  i += 1;
end

val = server.getValues("list.fixed1.clist1.item");
val.each{ | name, value |
  printf("%s: %s\n",name,value); };

server.close;
