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
    main.entry = ""
    main.state = 0;
    main.check = false
    for i in 0..2 do
      main.radio[i] = false;
    end
    main.radio[2] = true;

    for i in 0..99 do
      main.optionmenu.item[i] = sprintf("option %d",i);
    end
    main.optionmenu.select = 3
    main.optionmenu.count = 100

    main.toggle = false;

    node.put_window
  end
  def do_red(node, db)
    main = node.windows["main"]
    spa = node.spa
    main.area3.color = "red"
    p main.entry
    p main.state
    main.state += 1;
    node.put_window
  end
  def do_blue(node, db)
    main = node.windows["main"]
    spa = node.spa
    main.area3.color = "blue"
    p main.entry
    p main.state
    main.state += 1;
    node.put_window
  end
  def do_yellow(node, db)
    main = node.windows["main"]
    spa = node.spa
    main.area3.color = "yellow"
    p main.entry
    p main.state
    main.state += 1;
    node.put_window
  end
  def do_next(node, db)
    main = node.windows["main"]
    sleep(10)
    p main.entry
    p main.state
    main.state += 1;
    node.put_window("CHANGE","next")
  end
  def do_file(node, db)
    main = node.windows["main"]
    puts("link to File")
    p main.entry
    p main.state
    main.state += 1;
    node.put_window("CHANGE","file/file")
  end
  def do_list(node, db)
    main = node.windows["main"]
    for i in 0..99 do
      if  main.list.select[i] then
        p "select" + i.to_s
      end
    end
    i = main.optionmenu.select;
    printf("[%s]",main.optionmenu.item[i]);
    if  main.check then
      p "check on"
    else
      p "check off"
    end
    for i in 0..2 do
      p main.radio[i],i
      if  main.radio[i] then
        p "radio" + i.to_s
      end
    end
    if  main.toggle then
      p "toggle on"
    else
      p "toggle off"
    end
    node.put_window
  end
  def do_search(node, db)
    main = node.windows["main"]
    puts("link to Search")
    p main.entry
    p main.state
    main.state += 1;
    node.put_window("CHANGE","search")
  end
  def do_exit(node, db)
    main = node.windows["main"]
    puts("exit")
    p main.entry
    node.put_window("EXIT","http://www.nichibenren.or.jp/index.html")
  end
end
