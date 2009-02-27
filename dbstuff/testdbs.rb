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

require	'dbslib';

server = DB_Server.new("localhost",0,"ogochan","ogochan");

adrs = Hash.new(nil);

adrs["adrs.mail.home"] = "ogo%";

rc = server.dbops("DBOPEN");
printf("rc = %d\n",rc);
rc = server.dbops("DBSTART");
printf("rc = %d\n",rc);

printf("[%s]\n",server.getSchema("adrs","mail"));

adrs.each{ | name, value | printf("%s: %s\n",name,value) };

rc = server.recordops("DBSELECT","adrs","mail",adrs);
printf("rc = %d\n",rc);

adrs.each{ | name, value | printf("%s: %s\n",name,value) };

while server.recordops("DBFETCH","adrs","mail",adrs) == 0
  adrs.each{ | name, value | printf("%s: %s\n",name,value) };
end

rc = server.dbops("DBCOMMIT");
printf("rc = %d\n",rc);
rc = server.dbops("DBDISCONNECT");
printf("rc = %d\n",rc);

server.close;
