class MainColor
  def start(node, db)
  end
  def do_red(node, db)
    main = node.thisscreen;
    spa = node.spa
    main.color = "red"
    node.exit
  end
  def do_blue(node, db)
    main = node.thisscreen;
    spa = node.spa
    main.color = "blue"
    node.exit("yellow")
  end
  def do_yellow(node, db)
    main = node.thisscreen;
    spa = node.spa
    main.color = "yellow"
    node.exit("red")
  end
end
