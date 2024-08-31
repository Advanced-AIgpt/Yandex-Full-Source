alipov@nikola2:~/arcadia/alice/boltalka/filters/lists$ cat obscene.txt | ./lower.py | grep -P '^[а-я]+$' > ../bad_rus.txt
alipov@nikola2:~/arcadia/alice/boltalka/filters/lists$ cat tragic.txt sex.txt medicine.txt grugs.txt drugs.txt alcohol.txt abortion.txt | ./lower.py | grep -P '^[а-я]+$' > ../bad_dict.txt
alipov@nikola2:~/arcadia/alice/boltalka/filters/lists$ cat abbs.txt | ./lower.py | grep -P '^[а-я]+$' >> ../bad_rus.txt
alipov@nikola2:~/arcadia/alice/boltalka/filters/lists$ cat abbs.txt | ./lower.py | grep -P '^[a-z]+$' >> ../bad_eng.txt
