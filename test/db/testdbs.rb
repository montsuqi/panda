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

require	'dbslib'

TEST_COUNT = 1000

server = DB_Server.new("localhost",0,"test","test")

test1 = Hash.new(nil);


rc = server.open
printf("DBOPEN rc = %d\n",rc)
rc = server.start
printf("DBSTART rc = %d\n",rc)

printf("[%s]\n",server.getSchema("test1","id2"));
#test1.each{ | name, value | printf("%s: %s\n",name,value) };

if !nil
  now = Time.now
  test1["test.code"] = TEST_COUNT;
  rec = server.recordops("DBSELECT","test1","id2",test1)
  while ( rec = server.recordops("DBFETCH","test1","id2",test1) )
#	rec.each{ | name, value | printf("%s: %s\n",name,value) }
  end
  rec = server.recordops("DBCLOSECURSOR","test1","id2")
  printf("%f sec\n", Time.now - now)
end

if !nil
  now = Time.now
  test1["test.code"] = TEST_COUNT;
  rec = server.recordops("DBSELECT","test1","id2",test1)
  rec = server.recordops("DBFETCH","test1","id2",test1,10000)
  rec.each { | item |
	# item.each{ | name, value | printf("%s: %s\n",name,value) }
  }
  rec = server.recordops("DBCLOSECURSOR","test1","id2")
  printf("%f sec\n", Time.now - now)
end

if !nil
  now = Time.now
  test1["test.code"] = TEST_COUNT;
  rec = server.recordops("DBSELECT","test1","id3",test1,10000)
  rec.each { | item |
	#  item.each{ | name, value | printf("%s: %s\n",name,value) }
  }
  printf("%f sec\n", Time.now - now)
end

rc = server.commit
printf("DBCOMMIT rc = %d\n",rc)
rc = server.disconnect
printf("DBDISCONNECT rc = %d\n",rc)

server.close;
