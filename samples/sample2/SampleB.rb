class SampleB
  def start(para, db)
    db.open
    db.start
    adrs = db["adrs"]
    adrs.select("count")
    adrs.fetch("count")
    puts(adrs.count)
    adrs.fetch("count")
    adrs.close_cursor("count")
    db.commit
    db.disconnect
  end
end
