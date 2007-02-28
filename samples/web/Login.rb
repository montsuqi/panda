class Login
  def start(node, db)
    initialize_window(node, db)
    initialize_link(node, db)
    login = node.windows["login"]
	cookie = login.cookie.value
	puts(" login = #{cookie}")
	if login.cookie.value  ==  "aaa"
	  main = node.windows["main"]
	  main.state = 0;
      node.put_window("CURRENT","main")
	else
	  node.put_window("CURRENT","login")
	end
  end
  
  def do_login(node, db)
    spa = node.spa
    login = node.windows["login"]
    user_id = login.id.value
    password = login.password.value
	p user_id
	p password
    user_base = db["user_base"]
    if user_id == ""
      window = nil
      login.message.value = "メールアドレスを正しく入力してください"
      node.put_window
    else
      time = Time.new
      puts("** Login!! #{time} user_id=#{user_id}")
      window = "main"
      login.message.value = ""
      spa.user = user_id
      spa.pass = password
	  login.cookie.value = user_id
      node.put_window("CHANGE","main")
    end
  end
  def do_clear(node, db)
    initialize_window(node,db)
    node.put_window
  end
  
  private

  def initialize_window(node, db)
    login = node.windows["login"]
    login.id.value = ""
    login.password.value = ""
  end
  
  def initialize_link(node, db)
    time = Time.new
    node.link.calendar.year = time.year
    node.link.calendar.month = time.month
    node.link.calendar.day = time.day
  end
end
