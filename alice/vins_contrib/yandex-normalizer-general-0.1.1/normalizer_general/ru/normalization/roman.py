#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories
import morphology
import case_control

print 'Roman numerals'

roman_digit = anyof("ivxlcdm")

roman_ones = (replace("i", "1") |
              replace("ii", "2") |
              replace("iii", "3") |
              replace("iv", "4") |
              replace("v", "5") |
              replace("vi", "6") |
              replace("vii", "7") |
              replace("viii", "8") |
              replace("ix", "9"))

roman_tens = ((replace("x", "1") |
               replace("xx", "2") |
               replace("xxx", "3") |
               replace("xl", "4") |
               replace("l", "5") |
               replace("lx", "6") |
               replace("lxx", "7") |
               replace("lxxx", "8") |
               replace("xc", "9")) +
              (insert("0") | roman_ones))

roman_hundreds = ((replace("c", "1") |
                   replace("cc", "2") |
                   replace("ccc", "3") |
                   replace("cd", "4") |
                   replace("d", "5") |
                   replace("dc", "6") |
                   replace("dcc", "7") |
                   replace("dccc", "8") |
                   replace("cm", "9")) +
                  (insert("00") |
                   insert("0") + roman_ones |
                   roman_tens))

roman_thousands = ((replace("m", "1") |
                    replace("mm", "2") |
                    replace("mmm", "3")) +
                   (insert("000") |
                    insert("00") + roman_ones |
                    insert("0") + roman_tens |
                    roman_hundreds))

roman = (roman_ones | roman_tens | roman_hundreds | roman_thousands)

# Not accurate enough; lots of false positives, but enough for our purposes.
paradigm_zt1m = [
    ("", feats('noun', 'sg', 'nom')),
    ("а", feats('noun', 'sg', 'gen')), # 'acc' -- messes with the case for numerals (no animacy)
    ("у", feats('noun', 'sg', 'dat')),
    ("ом", feats('noun', 'sg', 'instr')),
    ("е", feats('noun', 'sg', 'loc')),
    ]

names_zt1m = ["аарон",
              "абас",
              "аббас",
              "абгар",
              "абдулла-хан",
              "абдул-меджид",
              "абдул-хамид",
              "август",
              "ага-хан",
              "агустин",
              "адам",
              "ад-дин",
              "адольф",
              "аймон",
              "акбар",
              "алам",
              "аларих",
              "александр",
              "аллен",
              "алоиз",
              "алпин",
              "альбер",
              "альберт",
              "альбрехт",
              "альпин",
              "альфонс",
              "амазасп",
              "амасис",
              "аменемхет",
              "аменхотеп",
              "андраш",
              "андроник",
              "анлав",
              "антигон",
              "антиох",
              "антуан",
              "ар-рахман",
              "ариарат",
              "ариобарзан",
              "артабан",
              "артавазд",
              "артаксеркс",
              "арташес",
              "артемис",
              "артур",
              "арчибальд",
              "арчил",
              "аршак",
              "асен",
              "аспаруг",
              "арнувандас",
              "атрейдес",
              "атруис",
              "афинагор",
              "ахмед",
              "ашот",
              "ашшур-нацир-апал",
              "бабкен",
              "баграт",
              "бакар",
              "бакур",
              "балдуин",
              "балиан",
              "басараб",
              "бахадыр",
              "бахрам",
              "баян",
              "баязид",
              "бейбарс",
              "бернард",
              "бернхард",
              "бенедикт",
              "беренгар",
              "бин",
              "богдан",
              "богдо-гэгэн",
              "бодуэн",
              "болеслав",
              "борис",
              "борис-михаил",
              "боэмунд",
              "брендан",
              "бржетислав",
              "брут",
              "будик",
              "бьёрн",
              "бэйкер",
              "вазген",
              "валеран",
              "валент",
              "валентиниан",
              "валериан",
              "вальдемар",
              "вараз-тироц",
              "вардан",
              "вахтанг",
              "вацлав",
              "видимир",
              "виллем",
              "вильгельм",
              "вингер",
              "виссарион",
              "влад",
              "владислав",
              "вратислав",
              "вуд",
              "гаврил",
              "гагик"
              "гарегин",
              "гарибальд",
              "гарольд",
              "гартнарт",
              "гасан",
              "гаспар",
              "гастон",
              "геворг",
              "гейтс",
              "генрих",
              "георг",
              "герман",
              "герольд",
              "гигес",
              "гиерон",
              "гильом",
              "гиркан",
              "годред",
              "гордиан",
              "гофрайд",
              "григор",
              "гримоальд",
              "грифид",
              "гудр" + anyof(u"её") +"д",
              "гумберт",
              "гурам",
              "гурген",
              "густав",
              "гутрум",
              "гутфрид",
              "давид",
              "давит",
              "даггс",
              "дагоберт",
              "даин",
              "дан",
              "девлет",
              "джалал",
              "джах",
              "джаяварман",
              "джекоб",
              "джексон",
              "джон",
              "джонс",
              "диниш",
              "дитрих",
              "доброслав",
              "домангарт",
              "домналл",
              "дональд",
              "дугал",
              "дункан",
              "дунхад",
              "дэвисон",
              "дэнетор",
              "дэниел",
              "жак",
              "жан",
              "жуан",
              "зипоит",
              "ибрагим",
              "ибрахим",
              "иван",
              "иванэ-атабак",
              "ивар",
              "идвал",
              "иеровоам",
              "изяслав",
              "имар",
              "инос",
              "иоанн",
              "иоасаф",
              "иоганн",
              "иоганн-георг",
              "иосиф",
              "ирод",
              "исаак",
              "ислям",
              "исмаил",
              "ител",
              "иштван",
              "йездигерд",
              "йожеф",
              "йоханн",
              "кавад",
              "кадваллон",
              "казимир",
              "кай-каус",
              "кай-кубад",
              "каликст",
              "кальман",
              "камбис",
              "каплан",
              "карел",
              "карл",
              "карлос",
              "карлуш",
              "касим",
              "каспиан",
              "кей-кубад",
              "кёлвульф",
              "кеннет",
              "кинан",
              "кинг",
              "кир",
              "климент",
              "кнут",
              "коилл",
              "коломан",
              "коналл",
              "конрад",
              "констант",
              "константин",
              "котис",
              "крак",
              "кристиан",
              "кристофер",
              "ксеркс",
              "курд",
              "кылыч-арслан",
              "лайош",
              "левон",
              "левкон",
              "леон",
              "леонид",
              "леопольд",
              "лех",
              "липот",
              "лливелин",
              "лоуб",
              "луарсаб",
              "луи-филипп",
              "луиш",
              "людвиг",
              "людовик",
              "лютер",
              "магнус",
              "марган",
              "мартинес",
              "майлгун",
              "макариос",
              "максимилиан",
              "максимин",
              "малик-шах",
              "малькольм",
              "мануил",
              "мануэл",
              "марван",
              "марган",
              "мартин",
              "мартинес",
              "масуд",
              "матиаш",
              "матьяш",
              "махмуд",
              "меджид",
              "мейнхард",
              "мелик-шах",
              "менандр",
              "менелик",
              "ментухотеп",
              "меткалф",
              "мехмед",
              "мжеж",
              "мигел",
              "мидас",
              "миндовг",
              "милан",
              "мириан",
              "митридат",
              "михал",
              "михаил",
              "мкртич",
              "моймир",
              "морган",
              "мохаммед",
              "муваталлис",
              "мурад",
              "мухаммед",
              "мушег",
              "мэттьювс",
              "мюллер",
              "навуходоносор",
              "наполеон",
              "нерсес",
              "неофит",
              "нехтон",
              "никифор",
              "никомед",
              "нортон",
              "олав",
              "олаф",
              "орхан",
              "оскар",
              "осман",
              "осоркон",
              "осред",
              "отакар",
              "оттокар",
              "оттон",
              "оуайн",
              "пав" + qq("е") + "л",
              "пакор",
              "перисад",
              "п" + anyof(u"её") + "тр",
              "петрислав",
              "полемон",
              "поррекс",
              "попел",
              "пресиан",
              "пржемысл",
              "притхвирадж",
              "псамметих",
              "псусеннес",
              "пшемысл",
              "рагнальд",
              "раймунд",
              "рамсес",
              "рев",
              "ризон",
              "рикер",
              "рискупорид",
              "ричард",
              "роберт",
              "рожер",
              "рокфеллер",
              "роман",
              "рудольф",
              "руфим",
              "саадет",
              "саак",
              "савромат",
              "сайфуддин",
              "салманасар",
              "саргон",
              "сатир",
              "саурмаг",
              "сахиб",
              "свен",
              "сверкер",
              "святополк",
              "святослав",
              "себастьян",
              "селевк",
              "селим",
              "селямет",
              "сенусерт",
              "сигеберт",
              "сигиберт",
              "сигизмунд",
              "сигурд",
              "сикст",
              "сильвестр",
              "симеон",
              "симион",
              "симон",
              "сингх",
              "сисил",
              "ситрик",
              "скарлат",
              "скорпион",
              "смбат",
              "собеслав",
              "соломон",
              "спарток",
              "спытигнев",
              "станислав",
              "стефан",
              "стефаноз",
              "сукман",
              "сулейман",
              "сумбат",
              "супан",
              "суппилулиумас",
              "суракат",
              "сурьяварман",
              "такер",
              "талоркан",
              "тассилон",
              "тахмасп",
              "тедош",
              "теймураз",
              "теодон",
              "теодорик",
              "теодориус",
              "теодорих",
              "тетрик",
              "тиглатпаласар",
              "тигран",
              "томас",
              "томислав",
              "трдат",
              "тудхалияс",
              "тутмос",
              "тутхалияс",
              "уайт",
              "уилсон",
              "уильям",
              "уильямс",
              "уильямсон",
              "улаф",
              "умма-хан",
              "уолкер",
              "урбан",
              "урош",
              "уэйнрайт",
              "фадл",
              "фазлун",
              "фарнабаз",
              "фарнаваз",
              "фарнак",
              "фарнвайл",
              "фарсман",
              "фарук",
              "фейсал",
              "феодор",
              "феофил",
              "ф" + anyof(u"её") + "дор",
              "фернвайл",
              "фергус",
              "фердинанд",
              "ференц",
              "ферхар",
              "фетих",
              "фетх",
              "филиберт",
              "филипп",
              "флорестан",
              "форд",
              "франтишек",
              "франц",
              "франциск",
              "фредерик",
              "фридрих",
              "фуад",
              "хакон",
              "хамазасп",
              "хамид",
              "ханс-адам",
              "харальд",
              "харольд",
              "хасан",
              "хаттусилис",
              "хаффман",
              "хвалимир",
              "хетум",
              "хивел",
              "хильдерик",
              "хильперик",
              "хлодвиг",
              "хокон",
              "хорик",
              "хосров",
              "хуан",
              "хуццияс",
              "шавур",
              "шамши-адад",
              "шапур",
              "шах",
              "шешонк",
              "штефан",
              "шишман",
              "щербан",
              "эван",
              "эвкратид",
              "эгберт",
              "эдберт",
              "эдвульф",
              "эдмунд",
              "эдуард",
              "эйрик",
              "экберт",
              "эккехард",
              "эльфволд",
              "эммануил",
              "энгельберт",
              "энгус",
              "эохайд",
              "эрик",
              "эрнст",
              "этельберт",
              "этельред",
              "этодиус",
              "юлиан",
              "юстин",
              "юстиниан",
              "юхан",
              "язид",
              "яков",
              "ян",
              "янош",
              "ярослав",
              "яхмос",
              ]
tagger_zt1m = tagger(names_zt1m, paradigm_zt1m)

paradigm_zt2m = [
    ("ь", feats('noun', 'sg', 'nom')),
    ("я", feats('noun', 'sg', 'gen')), # 'acc'
    ("ю", feats('noun', 'sg', 'dat')),
    ("ем", feats('noun', 'sg', 'instr')),
    ("е", feats('noun', 'sg', 'loc')),
    ]
names_zt2m = [
    "асен",
    "венцел",
    "габриэл",
    "карол",
    "лазар",
    "лотар",
    "мануэл",
    "раул",
    "хлотар",
    ]
tagger_zt2m = tagger(names_zt2m, paradigm_zt2m)

paradigm_zt6m = [
    ("й", feats('noun', 'sg', 'nom')),
    ("я", feats('noun', 'sg', 'gen')), # 'acc'
    ("ю", feats('noun', 'sg', 'dat')),
    ("ем", feats('noun', 'sg', 'instr')),
    ("е", feats('noun', 'sg', 'loc')),
    ]
names_zt6m = ["агесила",
              "адиль-гере",
              "алексе",
              "амаде",
              "виктор-амаде",
              "борживо",
              "варфоломе",
              "каро",
              "мате",
              "миха",
              "никола",
              "птолеме",
              "эдла",
             ]
tagger_zt6m = tagger(names_zt6m, paradigm_zt6m)

paradigm_zt7m = [
    ("й", feats('noun', 'sg', 'nom')),
    ("я", feats('noun', 'sg', 'gen')), # 'acc'
    ("ю", feats('noun', 'sg', 'dat')),
    ("ем", feats('noun', 'sg', 'instr')),
    ("и", feats('noun', 'sg', 'loc')),
    ]
names_zt7m = ["алекси",
              "анастаси",
              "антони",
              "арсени",
              "бонифаци",
              "варсонофи",
              "васили",
              "геласи",
              "георги",
              "гонори",
              "горди",
              "григори",
              "дари",
              "деметри",
              "дмитри",
              "евстахи",
              "евфими",
              "иннокенти",
              "иракли",
              "клавди",
              "констанци",
              "лжедмитри",
              "луци",
              "макари",
              "македони",
              "мардари",
              "мелети",
              "пасхали",
              "пи",
              "пруси",
              "серги",
              "сисилли",
              "тибери",
              "тивери",
              "феодоси",
              "фоти",
              "фульгени",
              "юли",
              "юри",
              ]
tagger_zt7m = tagger(names_zt7m, paradigm_zt7m)

paradigm_zt1fm = [
    ("а", feats('noun', 'sg', 'nom')),
    ("ы", feats('noun', 'sg', 'gen')),
    ("е", feats('noun', 'sg', 'dat', 'loc')),
    ("у", feats('noun', 'sg', 'gen')), # !!!! Should be 'acc', but this works better without explicit animacy coding
    ("ой", feats('noun', 'sg', 'instr')),
    ]
names_zt1fm = ["абдалл",
               "агил",
               "агрипп",
               "аминт",
               "ануванд",
               "бел",
               "бхаскар",
               "винтил",
               "гез",
               "далай-лам",
               "дамстр",
               "дынх",
               "йос",
               "канишк",
               "косьм",
               "лабарн",
               "монтесум",
               "мустаф",
               "никол",
               "от",
               "савв",
               "собуз",
               "суппилулиум",
               "фом",
               "цитант",
               "шенуд",
               "элл",
               ]
tagger_zt1fm = tagger(names_zt1fm, paradigm_zt1fm)

paradigm_zt2fm = [
    ("я", feats('noun', 'sg', 'nom')),
    ("и", feats('noun', 'sg', 'gen')),
    ("е", feats('noun', 'sg', 'dat', 'loc')),
    ("ю", feats('noun', 'sg', 'gen')), # !!!! Should be 'acc', but this works better without explicit animacy coding
    ("ей", feats('noun', 'sg', 'instr')),
    ]
names_zt2fm = ["михн",
               "муави",
               ]
tagger_zt2fm = tagger(names_zt2fm, paradigm_zt2fm)

paradigm_zt3fm = [
    ("а", feats('noun', 'sg', 'nom')),
    ("и", feats('noun', 'sg', 'gen')),
    ("е", feats('noun', 'sg', 'dat', 'loc')),
    ("у", feats('noun', 'sg', 'gen')), # !!!! Should be 'acc', but this works better without explicit animacy coding
    ("ей", feats('noun', 'sg', 'instr')),
    ]
names_zt3fm = ["мирч",
               ]
tagger_zt3fm = tagger(names_zt3fm, paradigm_zt3fm)

paradigm_zt6bfm = [
    ("я", feats('noun', 'sg', 'nom')),
    ("и", feats('noun', 'sg', 'gen')),
    ("е", feats('noun', 'sg', 'dat', 'loc')),
    ("ю", feats('noun', 'sg', 'gen')), # !!!! Should be 'acc', but this works better without explicit animacy coding
    (anyof(u"её") + "й", feats('noun', 'sg', 'instr')),
    ]
names_zt6bfm = ["или",
               "иль",
               ]
tagger_zt6bfm = tagger(names_zt6bfm, paradigm_zt6bfm)

paradigm_zt7f = [
    ("я", feats('noun', 'sg', 'nom')),
    ("и", feats('noun', 'sg', 'gen', 'dat', 'loc')),
    ("ю", feats('noun', 'sg', 'gen')), # !!!! Should be 'acc', but this works better without explicit animacy coding
    ("ей", feats('noun', 'sg', 'instr')),
    ]

names_zt7fm = ["манасси",
               "франческо мари",  # won't work?
               ]
tagger_zt7fm = tagger(names_zt7fm, paradigm_zt7f)

paradigm_zt0 = [
    ("", feats('noun', 'sg', 'nom', 'gen', 'dat', 'instr', 'loc')), # 'acc'
    ]
names_zt0m = ["адарнасе",
              "александру",
              "альфонсо",
              "амару",
              "амори",
              "анри",
              "антониу",
              "аргишти",
              "афонсу",
              "балши",
              "барбу",
              "бенигно",
              "бруно",
              "ваче",
              "гази",
              "газы",
              "генри",
              "геро",
              "гонсало",
              "гранде",
              "данил",
              "деметре",
              "джулиано",
              "дуарте",
              "жоффруа",
              "иаго",
              "инге",
              "иясу",
              "кай-хусрау",
              "камеамеа",
              "кинносукэ",
              "козимо",
              "ласло",
              "лео",
              "лешко",
              "лоренцо",
              "луи",
              "менгли",
              "менду",
              "менендо",
              "мешко",
              "михайло",
              "мойзе",
              "мсвати",
              "мурсили",
              "нехо",
              "николае",
              "нуньо",
              "нягое",
              "оноре",
              "отто",
              "педро",
              "педру",
              "петру",
              "пиопи",
              "раду",
              "ренье",
              "родри",
              "саншу",
              "селассие",
              "сети",
              "сефи",
              "таа",
              "тамасабуро",
              "танумафили",
              "тибо",
              "тупоу",
              "уласло",
              "умберто",
              "федерико",
              "фернандо",
              "фернанду",
              "ферранте",
              "франсуа",
              "франческо",
              "фроди",
              "хаджи",
              "хайме",
              "хаттусили",
              "чва",
              "энрике",
              "энтони",
              "эццелино",
              ]
tagger_zt0m = tagger(names_zt0m, paradigm_zt0)

tagger_lev = direct_tagger(("лев", feats('noun', 'sg', 'nom')),
                           (replace("льва", "лев"), feats('noun', 'sg', 'gen')), # 'acc'
                           (replace("льву", "лев"), feats('noun', 'sg', 'dat')),
                           (replace("львом", "лев"), feats('noun', 'sg', 'instr')),
                           (replace("льве", "лев"), feats('noun', 'sg', 'loc')))

tagger_m = (tagger_zt1m | tagger_zt2m | tagger_zt6m | tagger_zt7m |
            tagger_zt1fm | tagger_zt2fm | tagger_zt3fm | tagger_zt6bfm | tagger_zt7fm |
            tagger_zt0m |
            tagger_lev)
tagger_m = tagger_m.optimize()
producer_m = tagger_m.invert()
producer_m = producer_m.optimize()

paradigm_zt1f = [
    ("а", feats('noun', 'sg', 'nom')),
    ("ы", feats('noun', 'sg', 'gen')),
    ("е", feats('noun', 'sg', 'dat', 'loc')),
    ("у", feats('noun', 'sg', 'acc')),
    ("ой", feats('noun', 'sg', 'instr')),
    ]
names_zt1f = ["береник",
              "бланк",
              "джованн",
              "екатерин",
              "елизавет",
              "изабелл",
#              "иоанн",		would probably do more harm than good, interfering with male name Иоанн
              "клеопатр",
              "маргарит",
              "ранавалун",
              "селен",
              "тамар",
#              "хуан",		again, afraid of interference
              ]
tagger_zt1f = tagger(names_zt1f, paradigm_zt1f)

paradigm_zt6f = [
    ("я", feats('noun', 'sg', 'nom')),
    ("и", feats('noun', 'sg', 'gen')),
    ("е", feats('noun', 'sg', 'dat', 'loc')),
    ("ю", feats('noun', 'sg', 'acc')),
    ("ей", feats('noun', 'sg', 'instr')),
    ]
names_zt6f = ["арсино",
    ]
tagger_zt6f = tagger(names_zt6f, paradigm_zt6f)

paradigm_zt7f = [
    ("я", feats('noun', 'sg', 'nom')),
    ("и", feats('noun', 'sg', 'gen', 'dat', 'loc')),
    ("ю", feats('noun', 'sg', 'acc')),
    ("ей", feats('noun', 'sg', 'instr')),
    ]
names_zt7f = ["артемиси",
              "мари",
              ]
tagger_zt7f = tagger(names_zt7f, paradigm_zt7f)

names_zt0f = ["маргрете",
              "хетепхерес",
              ]
tagger_zt0f = tagger(names_zt0f, paradigm_zt0)

tagger_f = (tagger_zt1f | tagger_zt7f | tagger_zt0f)
tagger_f = tagger_f.optimize()
producer_f = tagger_f.invert()
producer_f = producer_f.optimize()

def test_form(tagger, producer, case):
    return (tagger >>
            pp(g.letter) + feats('noun', case) >>
            producer)

def make_name_number(case):
    def raw_for_gender(tagger, producer, gender):
        return (test_form(tagger, producer, case) +
                word((pp(roman_digit) >> roman) + insert(feats('numeral', 'ord', gender, case))))
    nn_raw = (raw_for_gender(tagger_m, producer_m, 'mas') |
              raw_for_gender(tagger_f, producer_f, 'fem'))
    return (remove("#" + case) + word(nn_raw, permit_inner_space=True) |
            cost(nn_raw, 0.1))

# Prefer nominative when in doubt
royalty = (make_name_number('nom') |
           cost(anyof([make_name_number(case) for case in cat_values('case')]), 0.01))

maybe_dot = qq(remove(ss(" ") + "."))

marker_words = [
    ("в" + maybe_dot, "век", "mas"),
    (failure(), "созыв", "mas"),
    (failure(), "съезд", "mas"),
    (failure(), "столети", "neu"),
    (failure(), "тысячелети", "neu"),
    (failure(), "групп", "fem"),
    (failure(), "олимпиад", "fem"),
    ("ст" + maybe_dot, "степен", "fem"),
]

# Mostly copied from date.year_with_case()
def marker_word_with_case(case):
    raw = anyof([(pp(roman_digit) >> roman) + insert(feats('numeral', 'ord', gender, case)) +
                 ((insert(" ") + replace(word(abbr, need_outer_space=False), (marker + feats('noun', 'sg', case) >> morphology.producer))) |
                  word(morphology.check_form(marker, feats('noun', 'sg', case))))
                for abbr, marker, gender in marker_words]).optimize()

    assisted = "#" + case + pp(" ") + raw
    return (assisted | cost(raw, 0.01)).optimize()

def convert_roman_range(gender, case):
    number_feats = feats("numeral", "ord", gender, case)
    roman_number_conversion = (pp(roman_digit) >> roman) + insert(number_feats)
    return roman_number_conversion + pp(ss(" ") + ((insert(" ") + "-" + insert(" ")) | "," | " и ") + ss(" ") + roman_number_conversion)

def convert_roman_range_with_marker(case, need_marker_word=True):
    raw = anyof([convert_roman_range(gender, case) + word(morphology.check_form(marker, feats('noun', case)))
                for abbr, marker, gender in marker_words])
    raw |= convert_roman_range("mas", case) + insert(" ") + replace(word("вв" + maybe_dot, need_outer_space=False, permit_inner_space=True),
                                                                    "век" + feats("noun", "pl", case) >> morphology.producer)
    assisted = "#" + case + pp(" ") + raw
    return (assisted | cost(raw, 0.01)).optimize()

# Markers that can follow in few words after roman number.
# For example: V олимпийские игры.
long_distance_markers = [("фестивал", "mas"),
                          ("игр", "pl")]

def convert_roman_with_context(marker, gender, case):
    number_feats = feats("numeral", "ord", gender, case)
    roman_number_conversion = (pp(roman_digit) >> roman) + insert(number_feats)
    return ((roman_number_conversion | convert_roman_range(gender, case)) +
            rr(pp(" ") + pp(g.letter), 0, 2) + pp(" ") + morphology.check_form(marker, feats("noun", gender, case)))

convert_roman_with_long_distance_marker = anyof([case_control.use_mark(lambda case: convert_roman_with_context(marker, gender, case),
                                                                       permit_inner_space=True,
                                                                       need_outer_space=False)
                                                for marker, gender in long_distance_markers])

# Markers that need nominative
NAMES_WITH_NOM = ["diablo",
                  "warcraft",
                  "mac os"]

with_nom = (anyof(NAMES_WITH_NOM) +
            word((pp(roman_digit) >> roman) + insert(feats('numeral', 'card', 'mas', 'nom'))))

with_marker_words = (marker_word_with_case('nom') |
                     cost(anyof([marker_word_with_case(case) for case in cat_values('case')]), 0.1))

roman_ranges = (convert_roman_range_with_marker('gen') |
                cost(anyof([convert_roman_range_with_marker(case) for case in cat_values('case')]), 0.1))

roman_range_with_prepositions = ("с" + qq("о") + qq(remove(word("#gen"))) +
                                 word(((pp(roman_digit) >> roman) + insert(feats("numeral", "ord", 'mas', 'gen'))) | convert_roman_range('mas', 'gen'), permit_inner_space=True) +
                                 word("по") + qq(remove(word("#acc"))) +
                                 word(((pp(roman_digit) >> roman) + insert(feats("numeral", "ord", 'mas', 'acc')) + qq(word(replace("вв" + maybe_dot, "век"), permit_inner_space=True))) |
                                      (convert_roman_range('mas', 'acc') + (word("век" + qq("а")) | ((ss(" ") | insert(" ")) + word(replace("вв" + maybe_dot, "века"), need_outer_space=False, permit_inner_space=True)))),
                                      permit_inner_space=True))

convert_roman = convert_words(roman_range_with_prepositions |
                              convert_roman_with_long_distance_marker |
                              cost(roman_ranges, 0.1) |
                              cost(with_marker_words, 0.2) |
                              royalty |
                              with_nom, permit_inner_space=True)

