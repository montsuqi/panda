/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include    <sys/types.h>
#include	<sys/stat.h>
#include    <unistd.h>
#include	<sys/time.h>
#include 	<gtk/gtk.h>
#include	<gtkpanda/gtkpanda.h>
#include	<glade/glade.h>
#include	<json.h>

#include	"types.h"
#include	"glclient.h"
#define		ACTION_MAIN
#include	"bd_config.h"
#include	"action.h"
#include	"dialogs.h"
#include	"styleParser.h"
#include	"gettext.h"
#include	"widgetcache.h"
#include	"widgetOPS.h"
#include	"protocol.h"
#include	"notify.h"
#include	"download.h"
#include	"print.h"
#include	"message.h"
#include	"debug.h"

/* GdkPixbuf RGBA C-Source image dump 1-byte-run-length-encoded */

#ifdef __SUNPRO_C
#pragma align 4 (my_pixbuf)
#endif
#ifdef __GNUC__
static const guint8 glclient_icon[] __attribute__ ((__aligned__ (4))) = 
#else
static const guint8 glclient_icon[] = 
#endif
{ ""
  /* Pixbuf magic (0x47646b50) */
  "GdkP"
  /* length: header (24) + pixel_data (5185) */
  "\0\0\24Y"
  /* pixdata_type (0x2010002) */
  "\2\1\0\2"
  /* rowstride (192) */
  "\0\0\0\300"
  /* width (48) */
  "\0\0\0""0"
  /* height (48) */
  "\0\0\0""0"
  /* pixel_data: */
  "\377\0\0\0\0\244\0\0\0\0\207YYY\377\247\0\0\0\0\202YYY\377\1{zz\377\202"
  "vvv\377\1uuu\377\202ttt\377\1{zz\377\202YYY\377\3\0\0\0\3\0\0\0\2\0\0"
  "\0\1\240\0\0\0\0\202YYY\377\5zzz\377qqq\377ppp\377qqq\377ppp\377\203"
  "ooo\377\3qqq\377rrr\377www\377\202YYY\377\3@@@\26\0\0\0\3\0\0\0\1\235"
  "\0\0\0\0\5YYY\377|||\377qqq\377kkk\377eee\377\205ddd\377\13eee\377hh"
  "h\377jjj\377kkk\377kjj\377nnn\377YYY\377,,,\26\0\0\0\7\0\0\0\3\0\0\0"
  "\1\203\0\0\0\0\205YYY\377\222\0\0\0\0\14YYY\377uuu\377qqq\377mmm\377"
  "ddd\377\\\\\\\377XWW\377XXX\377YYY\377ZZZ\377[[[\377\\\\\\\377\202]]"
  "]\377\10```\377aaa\377eee\377vuu\377YYY\377\0\0\0\24\0\0\0\13\0\0\0\6"
  "\203YYY\377\2vuu\377nnn\377\202jjj\377\2lkk\377YYY\377\202\0\0\0\3\2"
  "\0\0\0\2\0\0\0\1\213\0\0\0\0\202YYY\377\4lll\377hhh\377ggg\377ddd\377"
  "\202\\\\\\\377\15\227\226\226\377\262\261\261\377\257\257\257\377\214"
  "\214\214\377^^^\377UUU\377TTT\377SSS\377TTT\377VVV\377\\\\\\\377eee\377"
  "sss\377\203YYY\377\15sss\377qqq\377kkk\377ccc\377WWW\377MMM\377NNN\377"
  "UUU\377tss\377YYY\377\0\0\0\13\0\0\0\10\0\0\0\4\212\0\0\0\0\4YYY\377"
  "utt\377ggg\377]]]\377\202ZZZ\377\4WWW\377[[[\377\235\235\235\377\363"
  "\363\363\377\202\373\373\373\377\32\361\360\360\377\226\225\225\377T"
  "SS\377LLL\377JJJ\377HHH\377FFF\377HHH\377OOO\377\\\\\\\377hhh\377mmm"
  "\377ooo\377hhh\377bbb\377XXX\377LLL\377@@@\377555\377888\377JJJ\377Y"
  "YY\377\0\0\0#\0\0\0\36\0\0\0\25\0\0\0\14\212\0\0\0\0\4YYY\377sss\377"
  "ddd\377UUU\377\202NNN\377\4KKK\377ggg\377\316\316\316\377\360\357\357"
  "\377\202\373\373\373\377\6\361\361\361\377\316\316\316\377cbb\377DDD"
  "\377BBB\377\77\77\77\377\202<<<\377\13AAA\377LLL\377XXX\377ccc\377hh"
  "h\377ZZZ\377KKK\377BBB\377;;;\377555\377655\377\202YYY\377\202\0\0\0"
  "F\3\0\0\0;\0\0\0*\0\0\0\30\213\0\0\0\0%YYY\377kll\377TTT\377GGG\377C"
  "CC\377BBB\377HHH\377nnn\377\220\222\220\377\244\246\246\377\267\267\267"
  "\377\230\231\231\377poo\377FEE\377<<<\377;;;\377999\377666\377555\377"
  "888\377BBB\377KKK\377UUU\377]]]\377UUU\377\77\77\77\377999\377<<<\377"
  "FFF\377YYY\377\0\0\0a\0\0\0l\0\0\0r\0\0\0l\0\0\0Z\0\0\0\77\0\0\0$\214"
  "\0\0\0\0\11YYY\377y~}\377kqq\377PQQ\377GGG\377FFF\377@@@\377;;;\3778"
  "88\377\202777\377\1""666\377\202555\377\3""444\377333\377222\377\202"
  "111\377\21""777\377\77\77\77\377III\377UUU\377XXX\377EEE\377AAA\377P"
  "PP\377YYY\377\0\0\0\205\0\0\0\221\0\0\0\227\0\0\0\224\0\0\0\206\0\0\0"
  "k\0\0\0I\0\0\0)\214\0\0\0\0\11YYY\377\227\242\240\377\255\271\267\377"
  "\255\260\257\377\241\243\242\377\242\242\242\377\225\225\225\377rrr\377"
  "BAA\377\203000\377\1///\377\203...\377\202---\377\22,,,\377...\37755"
  "5\377@@@\377LLL\377[[[\377WWW\377VVV\377YYY\377\0\0\0\253\0\0\0\262\0"
  "\0\0\264\0\0\0\255\0\0\0\236\0\0\0\205\0\0\0e\0\0\0C\0\0\0%\214\0\0\0"
  "\0\32\0\0\0\3YYY\377\230\246\244\377\264\301\276\377\320\330\330\377"
  "\341\346\344\377\351\355\355\377\351\353\353\377\256\256\256\377}}}\377"
  "\77\77\77\377999\377888\377000\377,,,\377***\377)))\377(((\377'''\377"
  "(((\377...\377888\377DDD\377RQQ\377ccc\377YYY\377\202\0\0\0\320\10\0"
  "\0\0\314\0\0\0\300\0\0\0\252\0\0\0\215\0\0\0m\0\0\0M\0\0\0""0\0\0\0\31"
  "\214\0\0\0\0\20\0\0\0\2\0\0\0\11YYY\377\227\247\244\377\242\265\261\377"
  "\314\331\330\377\350\356\356\377\351\356\356\377\350\356\355\377\343"
  "\346\346\377\220\220\220\377fff\377PPP\377BBB\377333\377)))\377\202&"
  "&&\377\22$$$\377%%%\377***\377]\\\\\377\212\211\211\377\242\242\242\377"
  "\236\240\237\377YYY\377\0\0\0\347\0\0\0\340\0\0\0\317\0\0\0\264\0\0\0"
  "\222\0\0\0l\0\0\0J\0\0\0/\0\0\0\33\0\0\0\15\214\0\0\0\0\21\0\0\0\1\0"
  "\0\0\6\0\0\0\22YYY\377\200\215\214\377\236\261\256\377\301\322\321\377"
  "\323\342\337\377\340\352\351\377\351\356\354\377\335\335\335\377utt\377"
  "___\377NNN\377;;;\377---\377&&&\377\202###\377\21\"\"\"\377;;;\377\301"
  "\301\301\377\327\327\327\377\356\356\356\377\324\333\331\377YYY\377\0"
  "\0\0\357\0\0\0\340\0\0\0\304\0\0\0\235\0\0\0q\0\0\0J\0\0\0,\0\0\0\30"
  "\0\0\0\14\0\0\0\5\215\0\0\0\0\3\0\0\0\3\0\0\0\13\0\0\0\34\202YYY\377"
  "\13\236\264\257\377\272\320\312\377\315\337\332\377\334\344\343\377\220"
  "\223\221\377ccc\377[[[\377QQQ\377===\377///\377&&&\377\202\"\"\"\377"
  "\202!!!\377\17HII\377zzz\377\247\253\252\377\274\312\307\377\261\274"
  "\272\377YYY\377\0\0\0\331\0\0\0\264\0\0\0\205\0\0\0U\0\0\0""0\0\0\0\30"
  "\0\0\0\13\0\0\0\4\0\0\0\1\215\0\0\0\0\15\0\0\0\1\0\0\0\6\0\0\0\20YYY"
  "\377'''\377YYY\377\247\275\273\377\267\312\307\377\177\205\204\377eg"
  "f\377ZZZ\377MMM\377FFF\377\202...\377\2(((\377\"\"\"\377\204!!!\377\15"
  "%%%\377032\377iol\377rvu\377XXX\377\0\0\0\323\0\0\0\250\0\0\0t\0\0\0"
  "C\0\0\0\40\0\0\0\15\0\0\0\4\0\0\0\1\217\0\0\0\0\20\0\0\0\2\0\0\0\10Y"
  "YY\377)))\377;;;\377YYY\377mrp\377ccc\377ZZZ\377FFF\377555\377===\377"
  "555\377...\377+++\377###\377\205!!!\377\13(((\377777\377LLL\377VVV\377"
  "\0\0\0\321\0\0\0\244\0\0\0m\0\0\0;\0\0\0\32\0\0\0\11\0\0\0\2\220\0\0"
  "\0\0\3\0\0\0\1\0\0\0\3\0\0\0\11\202YYY\377\5bcc\377ccc\377YYY\377KKK"
  "\377BBB\377\202999\377\2""222\377111\377\202YYY\377\1(''\377\202!!!\377"
  "\202\40\40\40\377\13$$$\377...\377@@@\377]]]\377VVV\377\0\0\0\246\0\0"
  "\0p\0\0\0>\0\0\0\33\0\0\0\11\0\0\0\2\221\0\0\0\0\20\0\0\0\1\0\0\0\4\0"
  "\0\0\13YYY\377kkk\377\\\\\\\377KKK\377CCC\377>>>\377999\377555\37767"
  "7\377YYY\377\320\331\327\377\320\326\325\377YYY\377\202!!!\377\202\40"
  "\40\40\377\13\"\"\"\377%%%\377222\377KKK\377TTT\377\0\0\0\257\0\0\0z"
  "\0\0\0G\0\0\0!\0\0\0\14\0\0\0\3\222\0\0\0\0\20\0\0\0\1\0\0\0\6YYY\377"
  "ggg\377QQQ\377BBB\377AAA\377>>>\377;;;\377===\377YYY\377\271\311\307"
  "\377\305\326\324\377\325\342\337\377\217\222\220\377---\377\202\40\40"
  "\40\377\1!!!\377\202\"\"\"\377\12***\377BBB\377RRR\377\0\0\0\273\0\0"
  "\0\211\0\0\0T\0\0\0*\0\0\0\21\0\0\0\5\0\0\0\1\222\0\0\0\0\17\0\0\0\3"
  "YYY\377kkk\377```\377NNN\377GGG\377CCC\377BCC\377]`_\377\232\253\250"
  "\377\247\276\271\377\270\316\311\377\316\337\335\377\333\342\340\377"
  "YYY\377\203!!!\377\14\"\"\"\377'''\377,,,\377BBB\377PPP\377\0\0\0\310"
  "\0\0\0\231\0\0\0d\0\0\0""6\0\0\0\27\0\0\0\10\0\0\0\2\213\0\0\0\0\10\0"
  "\0\0\27\0\0\0\212\0\0\0\273\0\0\0\257\0\0\0e\0\0\0\4\0\0\0\0\0\0\0\1"
  "\202YYY\377\3qqq\377a``\377XXX\377\203YYY\377\7\202\217\215\377\231\260"
  "\254\377\255\307\304\377\303\330\324\377\327\344\340\377\221\226\224"
  "\377YYY\377\202!!!\377\14\"\"\"\377(((\377///\377AAA\377QQQ\377\0\0\0"
  "\324\0\0\0\251\0\0\0u\0\0\0C\0\0\0\37\0\0\0\13\0\0\0\3\212\0\0\0\0\12"
  "\0\0\0!\0\0\0\322\0\0\0`\0\0\0'\0\0\0E\0\0\0\320\0\0\0\271\0\0\0\5\0"
  "\0\0\0\0\0\0\3\205YYY\377\27\0\0\0\311\0\0\0\342YYY\377\210\232\226\377"
  "\240\273\267\377\264\315\312\377\301\325\323\377\327\342\337\377\212"
  "\217\215\377xxx\377jjj\377YYY\377---\377111\377BBB\377RRR\377\0\0\0\335"
  "\0\0\0\267\0\0\0\204\0\0\0O\0\0\0&\0\0\0\16\0\0\0\4\212\0\0\0\0\2\0\0"
  "\0\275\0\0\0I\203\0\0\0\0\"\0\0\0\30\0\0\0\363\0\0\0y\0\0\0\0\0\0\0\1"
  "\0\0\0\7\0\0\0\24\0\0\0""0\0\0\0Z\0\0\0\214\0\0\0\265\0\0\0\317\0\0\0"
  "\327YYY\377\202\225\223\377\225\260\253\377\253\306\301\377\303\330\324"
  "\377\335\346\344\377\323\326\326\377\306\306\306\377\266\265\265\377"
  "EDD\377111\377EEE\377SSS\377\0\0\0\344\0\0\0\302\0\0\0\220\0\0\0Y\0\0"
  "\0,\0\0\0\21\0\0\0\5\0\0\0\1\210\0\0\0\0\3\0\0\0\40\0\0\0\360\0\0\0\3"
  "\204\0\0\0\0!\0\0\0\250\0\0\0\326\0\0\0\0\0\0\0\1\0\0\0\4\0\0\0\16\0"
  "\0\0$\0\0\0G\0\0\0q\0\0\0\226\0\0\0\254\0\0\0\261\0\0\0\255YYY\377s\206"
  "\202\377\220\251\246\377\256\306\303\377\303\325\323\377\217\222\222"
  "\377lom\377YYY\377111\377000\377FFF\377QQQ\377\0\0\0\347\0\0\0\307\0"
  "\0\0\226\0\0\0_\0\0\0""0\0\0\0\23\0\0\0\5\0\0\0\1\210\0\0\0\0\3\0\0\0"
  "B\0\0\0\377\0\0\0\247\204\0\0\0\0\2\0\0\0\211\0\0\0\373\202\0\0\0\0\35"
  "\0\0\0\2\0\0\0\11\0\0\0\30\0\0\0""1\0\0\0Q\0\0\0m\0\0\0|\0\0\0~\0\0\0"
  "x\0\0\0sYYY\377u\213\206\377\220\253\247\377fwu\377+//\377!\40\40\377"
  "$##\377'''\377111\377DDD\377NNN\377\0\0\0\351\0\0\0\312\0\0\0\232\0\0"
  "\0a\0\0\0""2\0\0\0\23\0\0\0\6\0\0\0\1\210\0\0\0\0\3\0\0\0\37\0\0\0\372"
  "\0\0\0\263\204\0\0\0\0\2\0\0\0\230\0\0\0\365\202\0\0\0\0\6\0\0\0\1\0"
  "\0\0\5\0\0\0\15\0\0\0\34\0\0\0""0\0\0\0B\202\0\0\0K\7\0\0\0E\0\0\0A\0"
  "\0\0EYYY\377\204\226\223\377PXW\377***\377\202$$$\377\14'&&\377000\377"
  "BBB\377LLL\377\0\0\0\352\0\0\0\313\0\0\0\233\0\0\0b\0\0\0""2\0\0\0\24"
  "\0\0\0\6\0\0\0\1\211\0\0\0\0\2\0\0\0\34\0\0\0\15\203\0\0\0\0\3\0\0\0"
  "\2\0\0\0\334\0\0\0\276\203\0\0\0\0\34\0\0\0\2\0\0\0\6\0\0\0\15\0\0\0"
  "\27\0\0\0\37\0\0\0$\0\0\0#\0\0\0\37\0\0\0\35\0\0\0!\0\0\0""0YYY\377U"
  "XX\377DDD\377,,,\377&&&\377'''\377...\377EEE\377NNN\377\0\0\0\351\0\0"
  "\0\312\0\0\0\231\0\0\0a\0\0\0""2\0\0\0\23\0\0\0\6\0\0\0\1\216\0\0\0\0"
  "\3\0\0\0d\0\0\0\377\0\0\0O\204\0\0\0\0\4\0\0\0\2\0\0\0\5\0\0\0\10\0\0"
  "\0\14\202\0\0\0\15\25\0\0\0\13\0\0\0\12\0\0\0\15\0\0\0\27\0\0\0)YYY\377"
  "]]]\377\77\77\77\377(((\377)))\377333\377EEE\377\0\0\0\363\0\0\0\346"
  "\0\0\0\307\0\0\0\226\0\0\0_\0\0\0""0\0\0\0\23\0\0\0\5\0\0\0\1\215\0\0"
  "\0\0\3\0\0\0""8\0\0\0\366\0\0\0\230\206\0\0\0\0\2\0\0\0\1\0\0\0\2\204"
  "\0\0\0\3\7\0\0\0\2\0\0\0\4\0\0\0\11\0\0\0\25\0\0\0(YYY\377MMM\377\202"
  "...\377\13>>>\377JJJ\377\0\0\0\354\0\0\0\340\0\0\0\277\0\0\0\216\0\0"
  "\0X\0\0\0,\0\0\0\21\0\0\0\5\0\0\0\1\214\0\0\0\0\4\0\0\0""4\0\0\0\356"
  "\0\0\0\227\0\0\0\2\215\0\0\0\0\22\0\0\0\1\0\0\0\3\0\0\0\11\0\0\0\24Y"
  "YY\377LLL\377000\377///\377III\377\0\0\0\326\0\0\0\340\0\0\0\324\0\0"
  "\0\262\0\0\0\201\0\0\0M\0\0\0%\0\0\0\16\0\0\0\4\214\0\0\0\0\3\0\0\0""4"
  "\0\0\0\354\0\0\0n\220\0\0\0\0\21\0\0\0\1\0\0\0\3\0\0\0\11WWW\377===\377"
  ",,,\377;;;\377III\377\0\0\0\300\0\0\0\316\0\0\0\302\0\0\0\236\0\0\0n"
  "\0\0\0\77\0\0\0\35\0\0\0\12\0\0\0\3\213\0\0\0\0\3\0\0\0\40\0\0\0\340"
  "\0\0\0G\222\0\0\0\0\20\0\0\0\1UUU\377ZZZ\377---\377000\377GGG\377\0\0"
  "\0\201\0\0\0\250\0\0\0\271\0\0\0\254\0\0\0\206\0\0\0W\0\0\0/\0\0\0\24"
  "\0\0\0\7\0\0\0\1\212\0\0\0\0\3\0\0\0\3\0\0\0\305\0\0\0I\204\0\0\0\0\2"
  "\0\0\0\10\0\0\0\16\214\0\0\0\0\21YYY\377wvv\365___\377777\377(((\377"
  "888\377MMM\377\0\0\0l\0\0\0\223\0\0\0\242\0\0\0\223\0\0\0m\0\0\0B\0\0"
  "\0\40\0\0\0\15\0\0\0\4\0\0\0\1\212\0\0\0\0\2\0\0\0j\0\0\0\210\205\0\0"
  "\0\0\2\0\0\0O\0\0\0K\210\0\0\0\0\204YYY\377\20ihh\377XWW\377===\377)"
  "))\377222\377III\377\0\0\0:\0\0\0a\0\0\0\203\0\0\0\215\0\0\0y\0\0\0T"
  "\0\0\0/\0\0\0\25\0\0\0\7\0\0\0\2\212\0\0\0\0\3\0\0\0\5\0\0\0\322\0\0"
  "\0\13\205\0\0\0\0\2\0\0\0\201\0\0\0""3\207\0\0\0\0\25YYY\377}}}\377i"
  "ii\377ZYY\377SRR\377JJJ\377;;;\377+++\377***\377GFF\377JJJ\377\0\0\0"
  "\77\0\0\0`\0\0\0x\0\0\0y\0\0\0a\0\0\0>\0\0\0\40\0\0\0\15\0\0\0\4\0\0"
  "\0\1\212\0\0\0\0\2\0\0\0L\0\0\0\227\205\0\0\0\0\3\0\0\0\7\0\0\0\325\0"
  "\0\0\33\207\0\0\0\0\24YYY\377uuu\377]]]\377EEE\377999\377000\377)))\377"
  "***\377>>>\377KKK\377\0\0\0""7\0\0\0O\0\0\0g\0\0\0r\0\0\0h\0\0\0L\0\0"
  "\0,\0\0\0\24\0\0\0\7\0\0\0\2\213\0\0\0\0\2\0\0\0\221\0\0\0\326\204\0"
  "\0\0\270\4\0\0\0\271\0\0\0\343\0\0\0\376\0\0\0\4\210\0\0\0\0\4YYY\377"
  "fee\377III\377*))\377\202###\377\15""333\377EEE\377\0\0\0G\0\0\0X\0\0"
  "\0j\0\0\0s\0\0\0n\0\0\0X\0\0\0""9\0\0\0\36\0\0\0\14\0\0\0\4\0\0\0\1\213"
  "\0\0\0\0\1\0\0\0}\207\0\0\0\304\1\0\0\0\265\212\0\0\0\0\21YYY\377a``"
  "\377100\377$##\377'&&\377@@@\377\77\77\77\377\0\0\0r\0\0\0\177\0\0\0"
  "\204\0\0\0|\0\0\0f\0\0\0G\0\0\0)\0\0\0\23\0\0\0\7\0\0\0\2\237\0\0\0\0"
  "\21\0\0\0\2WWW\377TTT\377333\377555\377BBB\377\0\0\0\211\0\0\0\231\0"
  "\0\0\235\0\0\0\223\0\0\0z\0\0\0X\0\0\0""6\0\0\0\33\0\0\0\13\0\0\0\3\0"
  "\0\0\1\237\0\0\0\0\2\0\0\0\2\0\0\0\11\203PPP\377\13EEE\377\0\0\0\236"
  "\0\0\0\255\0\0\0\250\0\0\0\220\0\0\0k\0\0\0E\0\0\0%\0\0\0\20\0\0\0\6"
  "\0\0\0\1\240\0\0\0\0\17\0\0\0\2\0\0\0\7\0\0\0\25LLL\377HHH\377\0\0\0"
  "x\0\0\0\230\0\0\0\246\0\0\0\235\0\0\0~\0\0\0V\0\0\0""1\0\0\0\27\0\0\0"
  "\11\0\0\0\3\241\0\0\0\0\17\0\0\0\1\0\0\0\5\0\0\0\16\0\0\0\40\0\0\0<\0"
  "\0\0]\0\0\0{\0\0\0\210\0\0\0\177\0\0\0b\0\0\0>\0\0\0\40\0\0\0\15\0\0"
  "\0\4\0\0\0\1\242\0\0\0\0\15\0\0\0\2\0\0\0\7\0\0\0\22\0\0\0$\0\0\0<\0"
  "\0\0S\0\0\0_\0\0\0X\0\0\0B\0\0\0(\0\0\0\23\0\0\0\7\0\0\0\2\243\0\0\0"
  "\0\14\0\0\0\1\0\0\0\3\0\0\0\10\0\0\0\22\0\0\0\40\0\0\0/\0\0\0""7\0\0"
  "\0""3\0\0\0&\0\0\0\26\0\0\0\12\0\0\0\3\210\0\0\0\0"};

static struct changed_hander {
	GObject					*object;
	GCallback				func;
	gpointer				data;
	gint					block_flag;
	struct changed_hander 	*next;
} *changed_hander_list = NULL;

static void ScaleWidget(GtkWidget *widget, gpointer data);
static void ScaleWindow(GtkWidget *widget);
static void ReDrawTopWindow(void);

static void
SetWindowIcon(GtkWindow *window)
{
	GdkPixbuf *pixbuf;

	pixbuf = gdk_pixbuf_new_from_inline (-1, glclient_icon, FALSE, NULL);
	if (pixbuf) {
		gtk_window_set_icon (window, pixbuf);
		g_object_unref (pixbuf);
	}
}

extern	void
RegisterChangedHandler(
	GObject *object,
	GCallback func,
	gpointer data)
{
  struct changed_hander *p = xmalloc (sizeof (struct changed_hander));
ENTER_FUNC;
	p->object = object;
	p->func = func;
	p->data = data;
	p->next = changed_hander_list;
	p->block_flag = FALSE;
	changed_hander_list = p;
LEAVE_FUNC;
}

extern void
BlockChangedHandlers(void)
{
  struct changed_hander *p;

ENTER_FUNC;
	for (p = changed_hander_list; p != NULL; p = p->next) {
		p->block_flag = TRUE;		 
		g_signal_handlers_block_by_func (p->object, p->func, p->data);
	}
LEAVE_FUNC;
}

extern void
UnblockChangedHandlers(void)
{
  struct changed_hander *p;
ENTER_FUNC;
	for (p = changed_hander_list; p != NULL; p = p->next) {
		if (p->block_flag) {
			g_signal_handlers_unblock_by_func (p->object, p->func, p->data);
		}
	}
LEAVE_FUNC;
}

extern	GtkWidget	*
GetWindow(
	GtkWidget	*widget)
{
	GtkWidget	*window;
ENTER_FUNC;
	window = gtk_widget_get_toplevel(widget);
LEAVE_FUNC;
	return (window);
}

extern	char	*
GetWindowName(
	GtkWidget	*widget)
{
	static char	wname[SIZE_LONGNAME];
ENTER_FUNC;
	/*	This logic is escape code for GTK bug.	*/
	strcpy(wname,glade_get_widget_long_name(widget));
	if (strchr(wname,'.')) {
		*(strchr(wname,'.')) = 0;
	}
LEAVE_FUNC;
	return (wname);
}

static	void
RegisterWidgets(
	GtkWidget *widget,
	gpointer data)
{
	WindowData *wdata;


	wdata = (WindowData*)data;
	if(GTK_IS_CONTAINER(widget)) {
		gtk_container_forall(GTK_CONTAINER(widget),RegisterWidgets,data);
	} 
	if (GTK_IS_PANDA_TIMER(widget)) {
		wdata->Timers = g_list_append(wdata->Timers,widget);
	}
}

extern	void
ResetTimers(
	char	*wname)
{
	WindowData *data;
	GList *l;

ENTER_FUNC;
	if ((data = GetWindowData(wname)) == NULL) {
		// FIXME sometimes comes here.
		g_warning("%s:%d data is NULL for %s\n", __FILE__, __LINE__,wname);
		return;
	}
	for(l=data->Timers;l!=NULL;l=l->next) {
		gtk_widget_show(GTK_WIDGET(l->data));
		gtk_panda_timer_reset (GTK_PANDA_TIMER(l->data));
	}
LEAVE_FUNC;
}

static	void
_StopTimersAll(
	gpointer	key,
	gpointer	value,
	gpointer	data)
{
	WindowData *wdata;
	GList *l;

	wdata = (WindowData*)value;
	for(l=wdata->Timers;l!=NULL;l=l->next) {
		gtk_widget_hide(GTK_WIDGET(l->data));
		gtk_panda_timer_stop(GTK_PANDA_TIMER(l->data));
	}
}

extern	void
StopTimersAll(void)
{
	g_hash_table_foreach(WINDOWTABLE(Session),_StopTimersAll,NULL);
}

extern	void
_AddChangedWidget(
	GtkWidget	*widget)
{
	char		*name;
	char		*wname;
	char		*key;
	char		*tail;
	WindowData	*wdata;

ENTER_FUNC;
	name = (char *)glade_get_widget_long_name(widget);
	tail = strchr(name, '.');
	if (tail == NULL) {
		wname = StrDup(name);
	} else {
		wname = StrnDup(name, tail - name);
	}
	if ((wdata = GetWindowData(wname)) != NULL) {
		if (g_hash_table_lookup(wdata->ChangedWidgetTable, name) == NULL) {
			key = StrDup(name);
			g_hash_table_insert(wdata->ChangedWidgetTable, key, key);
		}
	}
	free(wname);
LEAVE_FUNC;
}

extern	void
AddChangedWidget(
	GtkWidget	*widget)
{
	if (!ISRECV(Session)) {
		_AddChangedWidget(widget);
	}
}

extern	void
ClearKeyBuffer(void)
{
	GdkEvent	*event; 
ENTER_FUNC;
	while( (event = gdk_event_get()) != NULL) {
		if ( (event->type == GDK_KEY_PRESS ||
			  event->type == GDK_KEY_RELEASE) ) {
 			gdk_event_free(event); 
		} else {
			gtk_main_do_event(event);
 			gdk_event_free(event); 
		}
	}
LEAVE_FUNC;
}

extern	void
SetTitle(GtkWidget	*window)
{
	char		buff[SIZE_BUFF];
	WindowData *wdata;

	wdata = GetWindowData((char *)gtk_widget_get_name(window));
	if (wdata == NULL||wdata->title == NULL) {
		return;
	} 

	if ( window == TopWindow && Session->title != NULL && strlen(Session->title) > 0 ) {
		snprintf(buff, sizeof(buff), "%s - %s", wdata->title, Session->title);
	} else {
		snprintf(buff, sizeof(buff), "%s", wdata->title);
	}
	gtk_window_set_title (GTK_WINDOW(window), buff);
}

extern	void
SetBGColor(GtkWidget *widget)
{
#ifdef LIBGTK_3_0_0
	GdkRGBA color;
	if (BGCOLOR(Session) != NULL) {
		if (gdk_rgba_parse(BGCOLOR(Session),&color)) {
			gtk_widget_override_background_color(widget,GTK_STATE_NORMAL,&color);
		} else {
			gtk_widget_override_background_color(widget,GTK_STATE_NORMAL,NULL);
		}
	} else {
			gtk_widget_override_background_color(widget,GTK_STATE_NORMAL,NULL);
	}
#else
	GdkColor color;
	if (BGCOLOR(Session) != NULL) {
		if (gdk_color_parse(BGCOLOR(Session),&color)) {
			gtk_widget_modify_bg(widget,GTK_STATE_NORMAL,&color);
		} else {
			gtk_widget_modify_bg(widget,GTK_STATE_NORMAL,NULL);
		}
	} else {
			gtk_widget_modify_bg(widget,GTK_STATE_NORMAL,NULL);
	}
#endif
}

static	WindowData*	
CreateWindow(
	const char	*wname,
	const char	*gladedata)
{
	GladeXML	*xml;
	WindowData	*wdata;
	GtkWidget	*window;
	GtkWidget	*child;

ENTER_FUNC;
	if (GetWindowData(wname) != NULL) {
		dbgprintf("%s already in WindowTable", wname);
		return NULL;
	}
	xml = glade_xml_new_from_memory((char*)gladedata,strlen(gladedata),NULL,NULL);
	if ( xml == NULL ) {
		Error("invalid glade data");
	}
	window = glade_xml_get_widget_by_long_name(xml, wname);
	if (window == NULL) {
		Error("Window %s not found", wname);
	}
	wdata = New(WindowData);
	wdata->xml = xml;
	wdata->name = StrDup(wname);
	wdata->title = StrDup((char*)gtk_window_get_title(GTK_WINDOW(window)));
	wdata->fAccelGroup = FALSE;
	wdata->ChangedWidgetTable = NewNameHash();
	wdata->Timers = NULL;
	wdata->tmpl = NULL;
	glade_xml_signal_autoconnect(xml);
	g_hash_table_insert(WINDOWTABLE(Session),strdup(wname),wdata);

	child = (GtkWidget *)g_object_get_data(G_OBJECT(window), "child");
	if (child == NULL) {
		Error("invalid glade:no child");
	}
	g_object_ref(child);
	gtk_widget_show_all(child);
	RegisterWidgets(child,wdata);
	if (IsDialog(window)) {
		dbgprintf("create dialog:%s\n", wname);
		gtk_container_add(GTK_CONTAINER(window), child); 
		wdata->fWindow = FALSE;
		SetWindowIcon(GTK_WINDOW(window));
	} else {
		dbgprintf("create window:%s\n", wname);
		wdata->fWindow = TRUE;
	}
LEAVE_FUNC;
	return wdata;
}

static	void
SwitchWindow(
	GtkWidget *window)
{
	GtkWidget	*child,*org;

ENTER_FUNC;
	child = (GtkWidget *)g_object_get_data(G_OBJECT(window), "child");
	g_return_if_fail(child != NULL);

	org = gtk_bin_get_child(GTK_BIN(TopWindow));
	if (org != NULL) {
		gtk_container_remove(GTK_CONTAINER(TopWindow),org);
	}
	gtk_container_add(GTK_CONTAINER(TopWindow),child);
	ScaleWidget(child,NULL);

	gtk_widget_set_name(TopWindow, gtk_widget_get_name(window));

	gtk_widget_show(TopWindow);
	gtk_widget_show(child);

#ifdef LIBGTK_3_0_0
	gtk_window_set_resizable(GTK_WINDOW(TopWindow), FALSE);
#else
	gtk_window_set_resizable(GTK_WINDOW(TopWindow), TRUE);
#endif


LEAVE_FUNC;
}

extern	void
CloseWindow(
	const char *wname)
{
	WindowData	*data;
	GtkWidget	*window;
	GSList		*list;
	int			i;
	GList		*wlist;

ENTER_FUNC;
	dbgprintf("close window:%s\n", wname);
	if ((data = GetWindowData(wname)) == NULL) {
		// FIXME sometimes comes here.
		fprintf(stderr,"%s:%d data %s is NULL\n", __FILE__, __LINE__, wname);
		return;
	}
	window = glade_xml_get_widget_by_long_name((GladeXML *)data->xml, wname);
	if (data->fWindow) {
		if (data->fAccelGroup) {
			for(list = ((GladeXML*)data->xml)->priv->accel_groups;
				list != NULL;
				list = list->next) {
				if (list->data != NULL) {
					gtk_window_remove_accel_group(GTK_WINDOW(TopWindow),
						(GtkAccelGroup*)(list->data));
				}
			}
			data->fAccelGroup = FALSE;
		}
	} else {
		gtk_widget_hide(window);
		gtk_window_set_modal(GTK_WINDOW(window), FALSE);
		wlist = g_list_find(DialogStack, window);
		if (wlist != NULL && wlist->next != NULL && wlist->next->data != NULL) {
			gtk_window_set_transient_for(GTK_WINDOW(wlist->next->data), NULL);
		}
		DialogStack = g_list_remove(DialogStack, window);
		for (i = g_list_length(DialogStack) - 1; i >= 0; i--) {
			if (i >0) {
				gtk_window_set_transient_for(
					(GtkWindow *)g_list_nth_data(DialogStack, i),
					(GtkWindow *)g_list_nth_data(DialogStack, i - 1));
			} else {
				gtk_window_set_transient_for(
					GTK_WINDOW(DialogStack->data), GTK_WINDOW(TopWindow));
			}
		}
	}
LEAVE_FUNC;
}


extern	void
ShowWindow(
	const char *wname)
{
	WindowData	*data;
	GtkWidget	*window;
	GSList		*list;

ENTER_FUNC;
	dbgprintf("show window:%s\n", wname);
	if ((data = GetWindowData(wname)) == NULL) {
		// FIXME sometimes comes here.
		g_warning("%s:%d data is NULL for %s\n", __FILE__, __LINE__,wname);
		return;
	}
	window = glade_xml_get_widget_by_long_name((GladeXML *)data->xml, wname);
	g_return_if_fail(window != NULL);

	if (data->fWindow) {
	dbgmsg("show primari window\n");
		gtk_widget_show(TopWindow);
		gtk_widget_grab_focus(TopWindow);
		if (strcmp(wname, gtk_widget_get_name(TopWindow))) {
			SwitchWindow(window);
			if (!data->fAccelGroup) {
				for(list = ((GladeXML*)data->xml)->priv->accel_groups;
					list != NULL;
					list = list->next) {
					if (list->data != NULL) {
					gtk_window_add_accel_group(GTK_WINDOW(TopWindow),
						(GtkAccelGroup*)(list->data));
					}
				}
				data->fAccelGroup = TRUE;
			}
		}
		SetTitle(TopWindow);
		SetBGColor(TopWindow);
		if (DelayDrawWindow) {
			ReDrawTopWindow();
		}
	} else {
	dbgmsg("show dialog\n");
		GtkWidget *parent = TopWindow;
		int i;

		ScaleWidget(window, NULL);
		ScaleWindow(window);
		SetBGColor(window);

		gtk_widget_show(window);
		gtk_window_set_modal(GTK_WINDOW(window), TRUE);
		for(i = 0; i < g_list_length(DialogStack); i++) {
			if ((gpointer)window != g_list_nth_data(DialogStack, i)) {
				parent = (GtkWidget *)g_list_nth_data(DialogStack, i);
			}
		}
		if (g_list_find(DialogStack, window) == NULL) {
			DialogStack = g_list_append(DialogStack, window);
			gtk_window_set_transient_for(GTK_WINDOW(window), 
				GTK_WINDOW(parent));
		}
		gtk_widget_show(window);
		gtk_window_set_modal(GTK_WINDOW(window), TRUE);
		if (DelayDrawWindow) {
			ReDrawTopWindow();
		}
	}
LEAVE_FUNC;
}


extern	void
ShowBusyCursor(
	GtkWidget	*widget)
{
	static GdkCursor *busycursor = NULL;
	GtkWidget *window;
ENTER_FUNC;
	if (widget == NULL) {
		return;
	}
	if (busycursor == NULL) {
		busycursor = gdk_cursor_new (GDK_WATCH);
	}
	window = gtk_widget_get_toplevel(widget);
	gtk_window_set_title(GTK_WINDOW(window),_("Now loading..."));
#ifndef LIBGTK_3_0_0
	gdk_window_set_cursor(window->window,busycursor);
	gdk_flush ();
#endif
LEAVE_FUNC;
}

extern	void
HideBusyCursor(GtkWidget *widget)
{
ENTER_FUNC;
	GtkWidget	*window;
	if (widget == NULL) {
		return;
	}
	window = gtk_widget_get_toplevel(widget);
	SetTitle(window);
#ifndef LIBGTK_3_0_0
	gdk_window_set_cursor(window->window,NULL);
#endif
LEAVE_FUNC;
}

static GtkWidget*
GetWidgetByWindowNameAndLongName(
	const char *windowName,
	const char *widgetName)
{
	WindowData	*wdata;
	GtkWidget	*widget;
	
	widget = NULL;
	wdata = GetWindowData(windowName);
	if (wdata != NULL && wdata->xml != NULL) {
	    widget = glade_xml_get_widget_by_long_name(
			(GladeXML *)wdata->xml, widgetName);
	}
	return widget;
}

static GtkWidget*
GetWidgetByWindowNameAndName(
	const char *windowName,
	const char *widgetName)
{
	WindowData	*wdata;
	GtkWidget	*widget;
	
	widget = NULL;
	wdata = GetWindowData(windowName);
	if (wdata != NULL && wdata->xml != NULL) {
	    widget = glade_xml_get_widget(
			(GladeXML *)wdata->xml, widgetName);
	}
	return widget;
}

extern	GtkWidget*
GetWidgetByLongName(const char *lname)
{
	char *wname,*p;
	GtkWidget *widget;
	
	wname = g_strdup(lname);
	for(p=wname;*p != 0;p++) {
		if (*p == '.') {
			*p = 0;
			break;
		}
	}
	widget = GetWidgetByWindowNameAndLongName(wname,lname);
	g_free(wname);
	return widget;
}

extern	GtkWidget*
GetWidgetByName(const char *name)
{
	WindowData	*wdata;
	GtkWidget	*widget;
	
	widget = NULL;
	wdata = GetWindowData(THISWINDOW(Session));
	if (wdata != NULL && wdata->xml != NULL) {
	    widget = glade_xml_get_widget((GladeXML *)wdata->xml, name);
	}
	return widget;
}


static  void
ScaleWidget(
    GtkWidget   *widget,
    gpointer    data)
{
	int has_x,x,_x;
	int has_y,y,_y;
	int has_w,w,_w;
	int has_h,h,_h;

#ifdef LIBGTK_3_0_0
	return;
#endif
	if (GTK_IS_CONTAINER(widget) && ! GTK_IS_SCROLLED_WINDOW(widget)) {
		gtk_container_set_resize_mode(GTK_CONTAINER(widget),GTK_RESIZE_QUEUE);
		gtk_container_forall(GTK_CONTAINER(widget), ScaleWidget, data);
	} 
	has_x = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"has_x"));
	has_y = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"has_y"));
	has_w = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"has_width"));
	has_h = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"has_height"));

	if (!has_x || !has_y || !has_w || !has_h) {
      return;
    }

	x = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"x"));
	y = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"y"));
	w = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"width"));
	h = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"height"));

	if (w >= 0 && h >= 0) {
		_x = (int)(x * TopWindowScale.h);
		_y = (int)(y * TopWindowScale.v);
		_w = (int)(w * TopWindowScale.h);
		_h = (int)(h * TopWindowScale.v);

#if 0
		fprintf(stderr,"scalewidget [[%d,%d],[%d,%d]]->[[%d,%d],[%d,%d]]\n",
			x,y,w,h,
			_x,_y,_w,_h);
#endif
		if (!GTK_IS_WINDOW(widget)) {
			gtk_widget_set_size_request(widget,_w,_h); 
			GtkWidget *parent = gtk_widget_get_parent(widget);
			if (parent != NULL && GTK_IS_FIXED(parent)) {
				gtk_fixed_move(GTK_FIXED(parent),widget,_x,_y);
			}
		}
	}
}

static	void
ScaleWindow(
	GtkWidget *widget)
{

	int x,_x;
	int y,_y;
	int w,_w;
	int h,_h;

#ifdef LIBGTK_3_0_0
	return;
#endif
	x = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"x"));
	y = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"y"));
	w = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"width"));
	h = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"height"));

	if (x != 0 || y != 0) {
		gtk_window_get_position(GTK_WINDOW(TopWindow),&_x,&_y);
			_x += (int)(x * TopWindowScale.h);
			_y += (int)(y * TopWindowScale.v);
#if 0
		fprintf(stderr,"move window [%d,%d]->[%d,%d]\n",
			x,y,_x,_y);
#endif
		gtk_window_move(GTK_WINDOW(widget),_x,_y);
	}

	if (w > 0 && h > 0) {
		_w = (int)(w * TopWindowScale.h);
		_h = (int)(h * TopWindowScale.v);
#if 0
		fprintf(stderr,"scale window [%d,%d]->[%d,%d]\n",
			w,h,_w,_h);
#endif
		gtk_widget_set_size_request(widget,_w,_h); 
	} 
}

static	void
ReDrawTopWindow(void)
{
#define WINC (1)
	static int i = 0;
	int width,height;

	gtk_window_get_size(GTK_WINDOW(TopWindow),&width,&height);
    g_signal_handlers_block_by_func(TopWindow,ConfigureWindow,NULL);
	if (i%2 == 0) {
		gtk_window_resize(GTK_WINDOW(TopWindow),width, height-WINC);
	} else {
		gtk_window_resize(GTK_WINDOW(TopWindow),width, height+WINC);
	}
    g_signal_handlers_unblock_by_func(TopWindow,ConfigureWindow,NULL);
	i++;
}

extern	void
ConfigureWindow(GtkWidget *widget,
	GdkEventConfigure *event,
	gpointer data)
{
	static int old_width = 0, old_height = 0;
	static int old_x = 0, old_y = 0;
	int x,y,width,height;
	char buf[16];

	gtk_window_get_position(GTK_WINDOW(widget), &x,&y);
	gtk_window_get_size(GTK_WINDOW(widget), &width,&height);

	if (old_x != x || old_y != y) {
		if (x >= 0) {
			sprintf(buf,"%d",x);
			SetWidgetCache("glclient.topwindow.x",buf);
		}
		if (y >= 0) {
			sprintf(buf,"%d",y);
			SetWidgetCache("glclient.topwindow.y",buf);
		}
	}

#if 0
	fprintf(stderr,"configure window[%d,%d][%d,%d]->[%d,%d][%d,%d]\n",
		old_x,old_y,old_width,old_height,x,y,width,height);
#endif
	if (old_width != width || old_height != height) {
		TopWindowScale.h = (width  * 1.0) / ORIGIN_WINDOW_WIDTH;
		TopWindowScale.v = (height * 1.0) / ORIGIN_WINDOW_HEIGHT;
		ScaleWidget(widget,NULL);
		sprintf(buf,"%d",width);
		SetWidgetCache("glclient.topwindow.width",buf);
		sprintf(buf,"%d",height);
		SetWidgetCache("glclient.topwindow.height",buf);
	}
	old_x = x;
	old_y = y;
	old_width = width;
	old_height = height;
}

extern	void
InitTopWindow(void)
{
	const char *px, *py, *pwidth, *pheight;
	int x,y,width,height;

	px = GetWidgetCache("glclient.topwindow.x");
	py = GetWidgetCache("glclient.topwindow.y");
	if (px != NULL && py != NULL) {
		x = atoi(px); y = atoi(py);
	} else {
		x = 0; y = 0;
	}
	pwidth = GetWidgetCache("glclient.topwindow.width");
	pheight = GetWidgetCache("glclient.topwindow.height");
	if (pwidth != NULL && pheight != NULL) {
		width = atoi(pwidth); height = atoi(pheight);
	} else {
		width = DEFAULT_WINDOW_WIDTH;
		height = DEFAULT_WINDOW_HEIGHT;
	}

	TopWindowScale.h = (ORIGIN_WINDOW_WIDTH  * 1.0) / width;
	TopWindowScale.v = (ORIGIN_WINDOW_HEIGHT * 1.0) / height;

	if (CancelScaleWindow) {
		width = ORIGIN_WINDOW_WIDTH;
		height = ORIGIN_WINDOW_HEIGHT;
		TopWindowScale.h = 1.0;
		TopWindowScale.v = 1.0;
	}

	TopWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_move(GTK_WINDOW(TopWindow),x,y); 
	gtk_container_set_resize_mode(GTK_CONTAINER(TopWindow),GTK_RESIZE_QUEUE);
#if LIBGTK_3_0_0
#  if 0
    gtk_window_set_default_size(GTK_WINDOW(TopWindow),width,height);
#  else
    gtk_window_set_default_size(GTK_WINDOW(TopWindow),
		DEFAULT_WINDOW_WIDTH,
		DEFAULT_WINDOW_HEIGHT);
#  endif
#else
	gtk_widget_set_size_request(TopWindow,width, height);
	GdkGeometry geometry;
	geometry.min_width = DEFAULT_WINDOW_WIDTH;
	geometry.min_height = DEFAULT_WINDOW_HEIGHT;
	gtk_window_set_geometry_hints(GTK_WINDOW(TopWindow),NULL,&geometry,
		GDK_HINT_MIN_SIZE);
	gtk_window_set_wmclass(GTK_WINDOW(TopWindow),"Glclient","Glclient");
#endif

	g_signal_connect(G_OBJECT(TopWindow), 
		"delete_event", G_CALLBACK(gtk_true), NULL);
	if (!CancelScaleWindow) {
		g_signal_connect(G_OBJECT(TopWindow), 
			"configure_event", G_CALLBACK(ConfigureWindow), NULL);
	}

	SetWindowIcon(GTK_WINDOW(TopWindow));
	DialogStack = NULL;
}

extern	gboolean
IsDialog(GtkWidget *widget)
{
#ifdef LIBGTK_3_0_0
	if (g_object_get_data(G_OBJECT(widget),"IS_DIALOG") != NULL) {
#else
	if (strstr(GTK_WINDOW(widget)->wmclass_class, "dialog") != NULL) {
#endif
		return TRUE;
	} else {
		return FALSE;
	}
}

extern  gboolean    
IsWidgetName(char *name)
{
	return (GetWidgetByWindowNameAndLongName(THISWINDOW(Session), name) != NULL);
}

extern  gboolean    
IsWidgetName2(char *name)
{
	return (GetWidgetByWindowNameAndName(THISWINDOW(Session), name) != NULL);
}

static  void
GrabFocus(
	const char *windowName, 
	const char *widgetName)
{
	GtkWidget 	*widget;

	widget = GetWidgetByWindowNameAndName(windowName,widgetName);
	if (widget != NULL) {
		gtk_widget_grab_focus(widget);
	}
}

extern  void        
UI_Init(int argc, 
	char **argv)
{
	gtk_init(&argc, &argv);
#if 1
	/* set gtk-entry-select-on-focus */
	GtkSettings *set = gtk_settings_get_default();
    gtk_settings_set_long_property(set, "gtk-entry-select-on-focus", 0, 
		"glclient2");
#endif
	gtk_panda_init(&argc,&argv);
#ifndef LIBGTK_3_0_0
	gtk_set_locale();
#endif
	glade_init();
}

extern	void
UI_Main(void)
{
	if (fIMKanaOff) {
		set_im_control_enabled(FALSE);
	}
	gtk_main();
}

extern	void
InitStyle(void)
{
	gchar *gltermrc;
	gchar *rcstr;

	StyleParserInit();
	gltermrc = g_strconcat(g_get_home_dir(),"/gltermrc",NULL);
	StyleParser(gltermrc);
	StyleParser("gltermrc");
	g_free(gltermrc);
	if (  Style  != NULL  ) {
		StyleParser(Style);
	}
	if (Gtkrc != NULL && strlen(Gtkrc)) {
		gtk_rc_parse(Gtkrc);
	} else {
		rcstr = g_strdup_printf(
			"style \"glclient2\" {"
			"  font_name = \"%s\""
			"  fg[NORMAL] = \"#000000\""
			"  text[NORMAL] = \"#000000\""
			"}"
			"style \"tooltip\" {"
			"  fg[NORMAL] = \"#000000\""
			"  bg[NORMAL] = \"#ffffaf\""
			"}"
			"style \"panda-entry\" = \"entry\" {"
			"  text[INSENSITIVE] = @text_color"
			"  base[INSENSITIVE] = \"#ffffff\""
			"}"
			"style \"panda-clist\" {"
			"  text[ACTIVE] = @selected_fg_color"
			"  base[ACTIVE] = @selected_bg_color"
			"}"
			"widget_class \"*.*\" style \"glclient2\""
			"widget_class \"*<GtkPandaEntry>\" style \"panda-entry\""
			"widget_class \"*<GtkNumberEntry>\" style \"panda-entry\""
			"widget_class \"*<GtkPandaCList>\" style \"panda-clist\""
			"widget \"gtk-tooltip*\" style \"tooltip\""
			,FontName);
		gtk_rc_parse_string(rcstr);
		gtk_rc_reset_styles(gtk_settings_get_default());
		g_free(rcstr);
	}
}

extern	int
AskPass(char	*buf,
	size_t		buflen,
	const char	*prompt)
{
	return 0;
}

static void
Ping()
{
	char *dialog,*popup,*abort;

	RPC_GetMessage(&dialog,&popup,&abort);
	if (strlen(abort)>0) {
		ShowInfoDialog(abort);
		exit(1);
	} else if (strlen(dialog)>0) {
		ShowInfoDialog(dialog);
	} else if (strlen(popup)>0) {
		Notify(_("glclient message notify"),popup,"gtk-dialog-info",0);
	}
	g_free(popup);
	g_free(dialog);
	g_free(abort);
}

static	void
PrintReport(
	json_object *obj)
{
	json_object *child;
	char *printer,*oid,*title;
	gboolean showdialog;
	LargeByteString *lbs;

	printer = NULL;
	title = "";
	showdialog = FALSE;

	child = json_object_object_get(obj,"object_id");
	if (!CheckJSONObject(child,json_type_string)) {
		return;
	}
	oid = (char*)json_object_get_string(child);

	child = json_object_object_get(obj,"printer");
	if (CheckJSONObject(child,json_type_string)) {
		printer = (char*)json_object_get_string(child);
	}
	child = json_object_object_get(obj,"title");
	if (CheckJSONObject(child,json_type_string)) {
		title = (char*)json_object_get_string(child);
	}
	child = json_object_object_get(obj,"showdialog");
	if (CheckJSONObject(child,json_type_boolean)) {
		showdialog = json_object_get_boolean(child);
	}
	if (getenv("GLCLIENT_PRINTREPORT_SHOWDIALOG")!=NULL) {
		showdialog = TRUE;
	}
	if (oid == NULL || strlen(oid) <= 0) {
		return;
	}
	if (printer == NULL || strlen(printer) <=0 ) {
		showdialog = TRUE;
	}

	lbs = REST_GetBLOB(oid);
	if (lbs != NULL) {
		if (LBS_Size(lbs) > 0) {
			if (showdialog) {
				ShowPrintDialog(title,lbs);
			} else {
				Print(oid,title,printer,lbs);
			}
		}
		FreeLBS(lbs);
	}
}

static	void
DownloadFile(
	json_object *obj)
{
	json_object *child;
	char *oid,*filename,*desc;
	LargeByteString *lbs;

	filename = "foo.dat";
	desc = "";

	child = json_object_object_get(obj,"object_id");
	if (!CheckJSONObject(child,json_type_string)) {
		return;
	}
	oid = (char*)json_object_get_string(child);

	child = json_object_object_get(obj,"filename");
	if (CheckJSONObject(child,json_type_string)) {
		filename = (char*)json_object_get_string(child);
	}
	child = json_object_object_get(obj,"description");
	if (CheckJSONObject(child,json_type_string)) {
		desc = (char*)json_object_get_string(child);
	}

	if (oid == NULL || strlen(oid) <= 0) {
		return;
	}
	lbs = REST_GetBLOB(oid);
	if (lbs != NULL) {
		if (LBS_Size(lbs) > 0) {
			ShowDownloadDialog(NULL,filename,desc,lbs);
		}
		FreeLBS(lbs);
	}
}

static void
ListDownloads()
{
	int i;
	json_object *obj,*item,*result,*type;

	obj = RPC_ListDownloads();
	result = json_object_object_get(obj,"result");
	if (!CheckJSONObject(result,json_type_array)) {
		Error(_("invalid list_report response"));
	}

	if (fMlog) {
		MessageLogPrintf("ListDownloads %s", (char*)json_object_to_json_string(result));
	}

	for (i=0;i<json_object_array_length(result);i++) {
		item = json_object_array_get_idx(result,i);
		if (!CheckJSONObject(item,json_type_object)) {
			continue;
		}
		type = json_object_object_get(item,"type");
		if (!CheckJSONObject(type,json_type_string)) {
			continue;
		}
		if (!strcmp(json_object_get_string(type),"report")) {
			PrintReport(item);
		} else {
			DownloadFile(item);
		}
	}
	json_object_put(obj);
}

static gint
PingTimerFunc(
	gpointer data)
{

	if (ISRECV(Session)) {
		return 1;
	}
	ListDownloads();
	if (strcmp(SERVERTYPE(Session),"ginbee")) {
		Ping();
	}
	return 1;
}

extern	void
SetPingTimerFunc()
{
	g_timeout_add(PingTimerPeriod,PingTimerFunc,NULL);
}

extern	WindowData *
GetWindowData(
	const char *wname)
{
	return (WindowData*)g_hash_table_lookup(WINDOWTABLE(Session),wname);
}

extern	void
SendEvent(
	const char *window,
	const char *widget,
	const char *event)
{
	json_object *params,*event_data;

	event_data = json_object_new_object();
	json_object_object_add(event_data,"window",
		json_object_new_string(window));
	json_object_object_add(event_data,"widget",
		json_object_new_string(widget));
	json_object_object_add(event_data,"event",
		json_object_new_string(event));
	json_object_object_add(event_data,"screen_data",
		MakeScreenData(window));
	
	params = json_object_new_object();
	json_object_object_add(params,"event_data",event_data);
	RPC_SendEvent(params);
}

static	void
CheckCloseWindow(
	json_object *w,
	int idx)
{
	json_object *child;
	const char *put_type;
	const char *wname;

	child = json_object_object_get(w,"put_type");
	if (child == NULL ||is_error(child)) {
		Error("invalid json part:put_type");
	}
	put_type = (char*)json_object_get_string(child);

	child = json_object_object_get(w,"window");
	if (child == NULL ||is_error(child)) {
		Error("invalid json part:window");
	}
	wname = json_object_get_string(child);

	if (GetWindowData(wname) == NULL) {
		return;
	}
	if (strcmp("new",put_type) && strcmp("current",put_type)) {
		if (fMlog) {
			MessageLogPrintf("close window[%s] put_type[%s]\n",wname,put_type);
		}
		CloseWindow(wname);
	}
}

static json_object *
CopyJSON(
	json_object *obj)
{
	json_object_iter iter;
	json_object *ret;
	int i,length;

	ret = NULL;
	switch(json_object_get_type(obj)) {
	case json_type_boolean:
		ret = json_object_new_boolean(json_object_get_boolean(obj));
		break;
	case json_type_double:
		ret = json_object_new_double(json_object_get_double(obj));
		break;
	case json_type_int:
		ret = json_object_new_int(json_object_get_int(obj));
		break;
	case json_type_string:
		ret = json_object_new_string(json_object_get_string(obj));
		break;
	case json_type_array:
		length = json_object_array_length(obj);
		ret = json_object_new_array();
		for(i=0;i<length;i++) {
			json_object_array_add(ret,CopyJSON(json_object_array_get_idx(obj,i)));
		}
		break;
	case json_type_object:
		ret = json_object_new_object();
		json_object_object_foreachC(obj,iter) {
			json_object_object_add(ret,iter.key,CopyJSON(iter.val));
		}
		break;
	default:
		ret = json_object_new_object();
		break;
	}

	return ret;
}


json_object *
UpdateTemplate(
	json_object *tmpl,
	json_object *obj,
	int level)
{
	json_object_iter iter;
	json_object *ret,*c1,*c2;
	int i;


	ret = NULL;
	if (tmpl == NULL) {
		return NULL;
	}
	switch(json_object_get_type(tmpl)) {
	case json_type_boolean:
		if (CheckJSONObject(obj,json_type_boolean)) {
			ret = json_object_new_boolean(json_object_get_boolean(obj));
		} else {
			ret = json_object_new_boolean(TRUE);
		}
		break;
	case json_type_double:
		if (CheckJSONObject(obj,json_type_double)) {
			ret = json_object_new_double(json_object_get_double(obj));
		} else {
			ret = json_object_new_double(0.0);
		}
		break;
	case json_type_int:
		if (CheckJSONObject(obj,json_type_int)) {
			ret = json_object_new_int(json_object_get_int(obj));
		} else {
			ret = json_object_new_int(0);
		}
		break;
	case json_type_string:
		if (CheckJSONObject(obj,json_type_string)) {
			ret = json_object_new_string(json_object_get_string(obj));
		} else {
			ret = json_object_new_string("");
		}
		break;
	case json_type_array:
		ret = json_object_new_array();
		if (CheckJSONObject(obj,json_type_array)) {
			for(i=0;i<json_object_array_length(tmpl);i++) {
				c1 = json_object_array_get_idx(tmpl,i);
				c2 = json_object_array_get_idx(obj,i);
				json_object_array_add(ret,UpdateTemplate(c1,c2,level+1));
			}
		} else {
			for(i=0;i<json_object_array_length(tmpl);i++) {
				c1 = json_object_array_get_idx(tmpl,i);
				json_object_array_add(ret,UpdateTemplate(c1,NULL,level+1));
			}
		}
		break;
	case json_type_object:
		ret = json_object_new_object();
		if (CheckJSONObject(obj,json_type_object)) {
			json_object_object_foreachC(tmpl,iter) {
				c1 = iter.val;
				c2 = json_object_object_get(obj,iter.key);
				json_object_object_add(ret,iter.key,UpdateTemplate(c1,c2,level+1));
			}
		} else {
			json_object_object_foreachC(tmpl,iter) {
				c1 = iter.val;
				json_object_object_add(ret,iter.key,UpdateTemplate(c1,NULL,level+1));
			}
		}
	default:
		break;
	}
	return ret;
}

static	void
UpdateWindow(
	json_object *w,
	int idx)
{
	json_object *child,*obj,*result,*old;
	const char *put_type;
	const char *wname;
	const char *gladedata;
	WindowData *wdata;

	child = json_object_object_get(w,"put_type");
	if (child == NULL ||is_error(child)) {
		Error("invalid json part:put_type");
	}
	put_type = (char*)json_object_get_string(child);

	child = json_object_object_get(w,"window");
	if (child == NULL ||is_error(child)) {
		Error("invalid json part:window");
	}
	wname = json_object_get_string(child);

	wdata = GetWindowData(wname);
	if (wdata == NULL) {
		obj = RPC_GetScreenDefine(wname);
		result = json_object_object_get(obj,"result");
		child = json_object_object_get(result,"screen_define");
		if (child == NULL ||is_error(child)) {
			Error("can't get screen define:%s",wname);
		}
		gladedata = json_object_get_string(child);
		wdata = CreateWindow(wname,gladedata);
		json_object_put(obj);
	}

	child = json_object_object_get(w,"screen_data");
	if (CheckJSONObject(child,json_type_object)) {
		if (wdata->tmpl == NULL) {
			if (json_object_object_length(child) > 0) {
				wdata->tmpl = CopyJSON(child);
			}
		} else {
			old = wdata->tmpl;
			wdata->tmpl = UpdateTemplate(old,child,0);
			json_object_put(old);
		}
	}

	if (!strcmp("new",put_type)||!strcmp("current",put_type)) {
		if (child == NULL ||is_error(child)) {
			Error("invalid json part:screeen_data");
		}
		UpdateWidget(GetWidgetByLongName(wname),wdata->tmpl);
		if (!strcmp(wname,FOCUSEDWINDOW(Session))) {
			ShowWindow(wname);
		}
		ResetTimers((char*)wname);
		if (fMlog) {
			MessageLogPrintf("show window[%s] put_type[%s]\n",wname,put_type);
		}
	}
}

extern	void
UpdateScreen()
{
	json_object *result,*window_data,*windows,*child;
	const char *f_window,*f_widget;
	int i;

	if (fMlog) {
		MessageLog("====");
	}
	
	result = json_object_object_get(SCREENDATA(Session),"result");
	window_data = json_object_object_get(result,"window_data");
	if (window_data == NULL ||is_error(window_data)) {
		Error("invalid json part:window_data");
	}

	child = json_object_object_get(window_data,"focused_window");
	if (child == NULL ||is_error(child)) {
		Error("invalid json part:focused_window");
	}
	f_window = json_object_get_string(child);
	if (THISWINDOW(Session) != NULL) {
		g_free(THISWINDOW(Session));
	}
	THISWINDOW(Session) = g_strdup(f_window);
	FOCUSEDWINDOW(Session) = (char*)f_window;
	if (fMlog) {
		MessageLogPrintf("focused_window[%s]",f_window);
	}

	child = json_object_object_get(window_data,"focused_widget");
	if (child == NULL ||is_error(child)) {
		Error("invalid json part:focused_widget");
	}
	f_widget = json_object_get_string(child);
	FOCUSEDWIDGET(Session) = (char*)f_widget;

	windows = json_object_object_get(window_data,"windows");
	if (windows == NULL ||
		is_error(windows) ||
		json_object_get_type(windows) != json_type_array) {
		Error("invalid json part:windows");
	}
	for(i=0;i<json_object_array_length(windows);i++) {
		child = json_object_array_get_idx(windows,i);
		CheckCloseWindow(child,i);
	}
	for(i=0;i<json_object_array_length(windows);i++) {
		child = json_object_array_get_idx(windows,i);
		UpdateWindow(child,i);
	}
	if (f_window != NULL) {
		if (!PandaTableFocusCell(f_widget)) {
			GrabFocus(f_window,f_widget);
		}
	}
}

extern	void
TimeSet(
	const char *str)
{
	static struct timeval t0;
	struct timeval t1;
	struct timeval d;
	long ms;
	
	gettimeofday(&t1,NULL);
	timersub(&t1,&t0,&d);
	t0 = t1;
	ms = d.tv_sec * 1000L + d.tv_usec/1000L;
	fprintf(stderr,"%s[%ld]\n",str,ms);
}
