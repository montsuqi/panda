require "uconv"
include Uconv

class Sample
  def start(dummy,db)
	db.open
	db.start
#	memo = db["memo"]
#	memo.each("memo", "memo" => "国語") do | memo |
#	memo.each("memo", "memo" => "title1") do | memo |
#	  printf("%s:%s\n",Uconv.u8toeuc(memo.title), Uconv.u8toeuc(memo.body));
#	end
#	memo = db["memo"]
#	memo.each("all") do | memo |
#	  printf("%s:%s\n",Uconv.u8toeuc(memo.title), Uconv.u8toeuc(memo.body));
#	end
	memo = db["word"]
#	memo.each("word", "word" => "動作") do | word |
	memo.each("word", "word" => "") do | word |
	  printf("%d:",word.wordCode);
	  printf("%s:",Uconv.u8toeuc(word.dickCode));
	  printf("%s:",Uconv.u8toeuc(word.word));
	  printf("%s:",Uconv.u8toeuc(word.ruby));
	  printf("%d\n",word.turn);
	end
	db.commit
	db.disconnect
  end
end
