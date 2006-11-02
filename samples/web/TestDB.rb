class TestDB
  def start(para, db)
    db.open
    db.start
    dbr = db['memo']
    dbr.key = '1, 2, 3'
    dbr.exec('DBIN','in')
    puts(dbr.ret);
    db.commit
    db.disconnect
  end
end
