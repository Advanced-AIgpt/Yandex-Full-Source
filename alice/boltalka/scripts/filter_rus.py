#!/usr/bin/python
# coding=utf-8
import sys
import regex
import codecs
sys.stdin = codecs.getreader('utf-8')(sys.stdin)
sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

rus_dict = set()
with codecs.open('/home/alipov/rus.dict', 'r', 'utf-8') as inp:
    for line in inp:
        rus_dict.add(line.strip())

bad_re = regex.compile(ur'ѝ|і|ї|ў|ў|[а-яА-Я]i|i[а-яА-Я]| i ')
for line in sys.stdin:
    if bad_re.search(line.lower()):
        continue
    #if regex.match(ur'.*(і|ї|ў|ў|[а-яА-Я]i|i[а-яА-Я]| i ).*', line.lower()):
        continue
    rus_cnt = 0
    cnt = 0
    s = regex.sub(ur'(\p{P}|`|~|«|»|…)', ' ', line.rstrip().lower())
    for tok in s.split():
        if not tok:
            continue
        if tok in rus_dict:
            rus_cnt += 1
        cnt += 1

    #print rus_cnt / (cnt + 1e-20), line.rstrip()
    if rus_cnt / (cnt + 1e-20) <= 0.5:
        continue

    print line.rstrip()
