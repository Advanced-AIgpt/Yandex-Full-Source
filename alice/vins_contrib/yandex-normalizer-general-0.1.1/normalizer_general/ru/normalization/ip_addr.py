#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories

print 'IP addresses'

# Is this the right term for an IP octet?
tetrade = qq(anyof("12")) + rr(g.digit, 1, 2) + insert(feats('numeral', 'card', 'nom', 'mas'))

ip_address = (tetrade + replace(".", " ") +
              tetrade + replace(".", " ") +
              tetrade + replace(".", " ") +
              tetrade)

cvt = convert_words(ip_address)
convert_ip_address = cvt.optimize()

