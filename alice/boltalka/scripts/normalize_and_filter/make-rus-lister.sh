#!/bin/bash -x
./filter_rus_lister.py --bad-dict ../../filters/bad_dict.txt,../../filters/bad_rus.txt --rus-lister ../../filters/rus-lister.txt 1> new_rus_lister.txt 2> err.txt
cat new_rus_lister.txt | bash ./build-rus-lister-dict.sh > tmp.tmp
./post_filter_rus_lister.py --bad-dict ../../filters/bad_rus.txt --rus-lister tmp.tmp > rus-lister.dict
rm tmp.tmp
