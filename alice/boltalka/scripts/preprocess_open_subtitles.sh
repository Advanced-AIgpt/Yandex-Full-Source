#!/bin/bash
NL='\n'
sed -re "s/([.!?])\s+(\-|–|—)\s*([А-ЯA-Z])/\1$NL\3/g; s/[^а-яА-Яa-zA-Z]*\[[^]]+\][^а-яА-Яa-zA-Z]*//g; s/\[[^[]*//g; s/[^]]*\]//g; s/^\s*(\-|–|—)\s*//" | grep -P '[!?а-яА-Яa-zA-Z0-9]'