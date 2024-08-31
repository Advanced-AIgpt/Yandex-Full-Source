#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories

print 'Times'

hour = (qq(remove("0")) + g.digit |
        "1" + g.digit |
        "2" + anyof("123"))
minute = ((remove("0") | anyof("12345")) +
          g.digit)

tail = qq(qq(remove("'")) + pp(g.letter))

time_separator = anyof(".:")

time = (hour + time_separator + minute + tail >>
        (replace("0" + time_separator + "30", "yarım") + tail |
         hour + remove(time_separator + "0") + tail |
         cost(hour + replace(time_separator, " ") + minute + tail, 0.01)))

cvt = convert_words(time)



