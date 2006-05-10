class TestDB
  def start(para, db)
    db.open
    db.start
    dbr = db["dbruby"]
    dbr.arg = 10
    dbr.exec("fact","cal")
    puts(dbr.ret);
    db.commit
    db.disconnect
  end
end
