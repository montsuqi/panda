#! /usr/bin/env ruby

require 'nkf'

=begin
  
  gdtestttf.rb -

=end

require 'GD'

# change this to be suitable for your Environment
#fontpath = "/usr/share/fonts/truetype/freefont/FreeMono.ttf"
#fontpath = "/usr/share/fonts/truetype/kochi/kochi-gothic-subst.ttf"
fontpath = "/home/ogochan/MONTSUQI/temp/nichiben.ttf"

def deg2rad(degree)
  degree*(Math::PI/180.0)
end

def maxx(brect)
  [[brect[0],brect[2]].max, [brect[4],brect[6]].max].max
end

def maxy(brect)
  [[brect[1],brect[3]].max, [brect[5],brect[7]].max].max
end

def minx(brect)
  [[brect[0],brect[2]].min, [brect[4],brect[6]].min].min
end

def miny(brect)
  [[brect[1],brect[3]].min, [brect[5],brect[7]].min].min
end

#str = "Ruby/GD version 0.7.2"
str = "\xE0\x01"
#str = "¤â¤²"

#ustr = NKF.nkf('-W16 -w8',str);
ustr = NKF.nkf('-E -w8',str);

angle = deg2rad(20)
sz = 36.0
# obtain brect so that we can size the image
err,brect = GD::Image.stringTTF(0, fontpath, sz, angle, 0, 0, ustr)

if err
  puts err
  exit 1
end

x = maxx(brect) - minx(brect) + 6
y = maxy(brect) - miny(brect) + 6
im = GD::Image.new(x,y)

white = im.colorResolve(255,255,255)
black = im.colorResolve(0,0,0)

x = 0 - minx(brect) + 3
y = 0 - miny(brect) + 3

(err, brect) = im.stringTTF(black, fontpath, sz, angle, x, y, ustr)
if err
  puts err
  exit 1
end

im.png $stdout

im.destroy
