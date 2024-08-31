## Транскрипция арабского

Сравнение нескольких алгоритмов перевода арабского текста в транскрипцию (фонемы)

См результат в [out1.json](out1.json)

Все алгоритмы требуют на вход полностью диакритизованного текста.

Чтобы получить диакритизованный текст, [есть несколько подходов](https://st.yandex-team.ru/ZELIBOBA-382#61d2f380ee67897cfc3c5339)

### Алгоритмы:
* [Buckwalter](https://en.wikipedia.org/wiki/Buckwalter_transliteration)
* [Arabic-Phonetiser](https://github.com/nawarhalabi/Arabic-Phonetiser) by Nawar Halabi
* transcript by nerevar@ ala google translator

#### Buckwalter
Наиболее широкоиспользующаяся транскрипция. Переводит арабский текст в английский 1к1.
При этом, из-за диакритики, текст получается не очень читаемый на английском. Зато можно восстановить арабский текст

#### Arabic-Phonetiser
Алгоритм g2p для TTS. Код под капотом использует Buckwalter преобразование, а затем работает с английскими символами

Умеет проставлять ударения

#### transcript by nerevar@ ala google translator
Попытка создать транскрипцию [как в google translator](https://translate.google.com/?source=osdd&sl=ar&tl=en&text=%D9%82%D9%90%D9%86%D9%8E%D8%A7%D8%B9%D9%8D%20%D8%A7%D9%84%D9%92%D8%A3%D9%8E%D8%A6%D9%90%D9%85%D9%91%D9%8E%D8%A9%D9%8F%20%D8%A3%D9%8E%D8%B5%D9%92%D8%AD%D9%8E%D8%A7%D8%A8&op=translate), только лучше: произношение в соответствии с ["таблицой Веры"](https://st.yandex-team.ru/ALAS-37)

Цель создания — создать человекочитаемую транскрипцию для русско/англоговорящего человека

Может выкидывать некоторые символы типа [Hamza](https://en.wikipedia.org/wiki/Hamza)

### Код и описаниие файлов
Запустить скрипт:
```
python3 main.py >stdout.log 2>stderr.log
```
На вход принимает файл `in1.json` содержащий массив строк на арабском языке

На выходе файл `out1.json` с массивом объектов: для каждой арабской строки приведена транскрипция каждого алгоритма.

Пример выходного файла:
```
[
...
    {
        "arabic": "قِنَاعٍ الْأَئِمَّةُ أَصْحَاب",
        "transcripted": "qinaaʽain al-aaimmaatu aaSHaab",
        "buckwalter": "qinaAEK Alo>a}im~apu >aSoHaAb",
        "phonemes_with_bnd": "qI0naa'Ei1n l<a<i0'mmatu0 <ASHaa'b",
        "phonemes": "q I0 n aa E i1 n l < a < i0 mm a t u0 < A S H aa b",
        "i": 4996
    },
...
]
```
Описание json:
* arabic - исходный текст на араьбском
* buckwalter - результат buckwalter
* phonemes, phonemes_with_bnd - фонемы g2p Arabic-Phonetiser
* transcripted - результат arabic_transcript
* i - номер utterance


Остальные файлы:
* `in1.all_data.json` - семпл 10к предложений из [YT таблички с диакритикой](https://yt.yandex-team.ru/hahn/navigation?pageSize=200&path=//home/search-functionality/gorb-roman/nirvana/6c3c1c00-6957-4810-a0ec-017c30f8749a/5facd9b9-19c4-4aa3-9de4-b15e2a0dbd94/dc37c580-7466-a786-8edb-061a7caf7743)
* `test_create.py` - семплинг 5к utterance'ов для теста разной длины (1-5 слов). Создаёт `test_input.txt`
* `test_canonize.py` - запускает ArabicTranscripter для каждой строчки из `test_input.txt`, результат сохраняет в файл `test_answer.txt`
* `test_run.py` - запускает ArabicTranscripter для каждой строчки из `test_input.txt`. Проверяет точное совпадение с текстом в соответствующей строчке в `test_answer.txt`
* `stdout.log`, `stderr.log` — логи запуска скрипта main.py
* исходный код:
    * `arabic_buckwalter.py` - код алгоритма buckwalter
    * `arabic_phonetise.py` - copy&paste алгоритм из Arabic-Phonetiser by Nawar Halabi
    * `arabic_transcript.py` + `arabic_mapping.json` - код nerevar@

### arabic_transcript.py TODO:
* Alef maksura
* Hamza
* Несколько странных символов помеченных `???` в `arabic_mapping.json` которые встречались в реальных текстах

см https://en.wikipedia.org/wiki/Romanization_of_Arabic#Comparison_table
