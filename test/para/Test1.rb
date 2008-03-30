class Test1
  def start(para, db)
    db.open
    test1 = db['test1']
	print "--- 1 transaction ---"
    db.start
	print "--- insert(1) --\n"
	for i in 1..100
	  test1.count = i
	  test1.code = 0
	  test1.insert('dbw')
	end
	print "--- select(1)  --\n"
    test1 = db['test1']
	items = test1.select('dbr', { 'count' => "100" }, 1000)
	if items
	  items.each do | item |
		printf("[%d][%d]\n",item.count,item.code)
	  end
	end
    db.commit
	sleep 2
	print "--- 2 transaction ---"
    db.start
	print "--- insert(2) --\n"
	for i in 1..100
	  test1.count = i
	  test1.code = 1
	  test1.insert('dbw')
	end
	print "--- select(2)  --\n"
	items = test1.select('dbr', { 'count' => "100" }, 1000)
	if items
	  items.each do | item |
		printf("[%d][%d]\n",item.count,item.code)
	  end
	end
    db.commit
	sleep 2
	print "--- 3 transaction ---"
	print "--- select(3)  --\n"
    db.start
	items = test1.select('dbr', { 'count' => "100" }, 1000)
	if items
	  items.each do | item |
		printf("[%d][%d]\n",item.count,item.code)
	  end
	end
    db.commit
    db.disconnect
  end
end
