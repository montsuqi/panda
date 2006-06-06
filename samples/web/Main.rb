class Main
  def start(node, db)
    main = node.windows["main"]
    spa = node.spa
    main.user = spa.user
    main.message = ""
    main.area3.color = "white"
    for i in 0..99 do
      main.list.select[i] = false
      main.list.value[i] = i.to_s
    end
    main.list.select[10] = true
    main.list.count = 100
#    node.start("main.area1");
#    node.start("main.area2");
    main.message="漢本"
    node.put_window
  end
  def do_red(node, db)
    main = node.windows["main"]
    spa = node.spa
    main.area3.color = "red"
    node.put_window
  end
  def do_blue(node, db)
    main = node.windows["main"]
    spa = node.spa
    main.area3.color = "blue"
    node.put_window
  end
  def do_yellow(node, db)
    main = node.windows["main"]
    spa = node.spa
    main.area3.color = "yellow"
    node.put_window
  end
  def do_next(node, db)
    sleep(10)
    node.put_window("CHANGE","next")
  end
  def do_file(node, db)
    puts("link to File")
    node.put_window("CHANGE","file/file")
  end
  def do_search(node, db)
    puts("link to Search")
    node.put_window("CHANGE","search")
  end
  def do_exit(node, db)
    puts("exit")
    node.put_window("EXIT","http://www.nichibenren.or.jp/index.html")
  end
end
