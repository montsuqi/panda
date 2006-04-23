class Main
  def start(node, db)
    main = node.windows["main"]
    spa = node.spa
    main.user = spa.user
    main.message = ""
    main.area1.color = "white"
    main.area2.color = "white"
    main.area3.color = "white"
    node.put_window
  end
  def do_red(node, db)
    main = node.windows["main"]
    spa = node.spa
    main.area3.color = "red"
    node.put_window("CURRENT","main")
  end
  def do_blue(node, db)
    main = node.windows["main"]
    spa = node.spa
    main.area3.color = "blue"
    node.put_window("CURRENT","main")
  end
  def do_yellow(node, db)
    main = node.windows["main"]
    spa = node.spa
    main.area3.color = "yellow"
    node.put_window("CURRENT","main")
  end
  def do_next(node, db)
    node.put_window("CHANGE","next")
  end
end
