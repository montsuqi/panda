class Test1
  def start(para, db)
    db.open
    db.start
    test1 = db['test1']
	print "--- insert --\n"
	for i in 1..10000
	  test1.id = sprintf('%10d',i)
	  test1.class = 1
	  test1.code = i
	  test1.insert('id')
	end
    db.commit
	print "--- select & fetch(1) --\n"
    db.start
    test1 = db['test1']
	test1.each('id2', 'id' => "100" ) do | item |
	  printf("[%s]\n",item.id)
	end
    db.commit
	print "--- select & fetch(2) --\n"
    db.start
    test1 = db['test1']
	test1.select('id2', { 'id' => "100" } )
	items = test1.fetch('id2', nil , 10)
	items.each do | item |
	  printf("[%s]\n",item.id);
	end
	for i in 0..9
	  printf("[%s]\n",items[i].id)
	end
    db.commit
	print "--- select  --\n"
    db.start
    test1 = db['test1']
	items = test1.select('id3', { 'id' => "100" } , 20)
	items.each do | item |
	  printf("[%s]\n",item.id);
	end
    db.commit
    db.disconnect
  end
end
