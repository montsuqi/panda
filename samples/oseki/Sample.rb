require "uconv"
include Uconv

class Sample
  def start(dummy,db)
	db.open
	db.start
	memo = db["memo"]
	memo.each("memo", "memo" => "国語") do | memo |
#	memo.each("memo", "memo" => "title1") do | memo |
	  printf("%s:%s\n",Uconv.u8toeuc(memo.title), Uconv.u8toeuc(memo.body));
#	  printf("%s:%s\n",memo.title, memo.body);
	end
	db.commit
	db.disconnect
  end
end
