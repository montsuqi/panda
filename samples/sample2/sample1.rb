class Sample1
  def start(aps, db)
    list = aps.windows["list"]
    list.fixed1.key.value = ""
    for i in 0..19
      list.fixed1.clist1.item[i].value1 = "";
      list.fixed1.clist1.item[i].value2 = "";
      list.fixed1.clist1.item[i].value3 = "";
      list.fixed1.clist1.item[i].value4 = "";
      list.fixed1.clist1.select[i] = false
    end
    list.fixed1.clist1.count = 0
    aps.link.linktext = "";
    aps.spa.dummy = "";
    aps.put_window("NEW", "list")
  end

  def do_Search(aps, db)
    list = aps.windows["list"]
    adrs = db["adrs"]
    i = 0
    adrs.each("mail", "mail.home" => list.fixed1.key.value) do |item|
      list.fixed1.clist1.item[i].value1 = item.name;
      list.fixed1.clist1.item[i].value2 = item.tel;
      list.fixed1.clist1.item[i].value3 = item.mail.home;
      if item.address
        list.fixed1.clist1.item[i].value4 = item.address[0];
      end
      list.fixed1.clist1.select[i] = false
      break if i == 19
      i += 1
    end
    list.fixed1.clist1.count = i
    aps.put_window
  end

  def do_Quit(aps, db)
    aps.put_window("CLOSE")
  end
end
