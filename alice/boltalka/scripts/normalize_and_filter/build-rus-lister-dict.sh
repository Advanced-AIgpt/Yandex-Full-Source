#!/bin/bash
grep -P '\[' | \
    grep -Pv 'obsc|obsclite|geo|patrn|persn|famn|abbr|dsbl|org' | cut -f1 | \
    grep -Pv '\[(?:хрен|гнид|мраз|твар|засран|голозад|дерьм|ублюд|подонк|падл|мерзав|обсос|отсос|отсас|обсас|проститут|сутен|быдл|офиг|фигн|фигов|антисемит|навальн|петух|питух|сволоч|дебил|урод|кретин|имбецил|дегенерат|идиот|даун|недоумо|недоумк|анус|москал|хохол|хохл|вырод[ок]|кацап|параш|трах|убог|горласт|фаллос|заднепривод|заднепроход|ораль|опуск|опущ|хабал|чучел|тупорыл|тупост|сатан|отрод[ьи]|рабовлад|работорг|рабс|рабы|недонош|поган[^к]|манат|шкон|шмон|сдох|дохлят|подстилк|упыр|морда[^ш]|толсту|торч|ватник|шпул|ущерб|псин|твит|рукоблуд|сучк|сучо|алко|уби[^р]|.*негрит|негроид|негрск|нигер|чурк|чурек|черномаз|чернокож|иуд|гомосек|лесбиян|некрофил|зоофил|педофил|геронтофил|пук|террор)' | \
    grep -Pv '\[(?:сук|сукин|фиг|фигасе|зиг|туп|раб|мамк|сперм|дох|сдох|сдых|лох|лохан|бомб|морд|нашист|ишак|бух|дм|лич)\]' | \
    grep -Pv '\[(?:хам|дур|туп|свин|гряз|мусл|жид|говн|говен|гей|колорад|совок|совк|ватн|укроп|ска|негр)\]' | \
    grep -Pv '(?:\[опуст\]и|\[паст\]ь|(?:\[рож\].{0,2}$)|(?:^\[нит\]$))' | \
    sed -re 's/\[|\]//g' | \
    grep -Pv 'расчлен|петуш|холокост|путин|аллах|хайль|бандер|гитлер|фашист|кукарек|рожами|паскуд|смерд|долб|динах|мусульм|джихад|ислам|христиан|православ|мусор|еврей|жидов|жидок|кавказ|украин|таджик|узбек|дагест|грузин|чечен|татар|абхаз|осетин|киргиз|казах|махач|либерал|расист|репресс|русофоб|греш|церков|афроамерикан|бабищ|барыж|барыг|возбужд|гестап|гноб|диктатор|курд|коммунист|самогон|нацист|национали|анарх|израил|свастик|ветеран|олигарх|депутат|губерн|президент|парнас|единорос|фекал' | \
    grep -Pv '^(?:очков.|очкy|очко$|очком|лох|перепих|умерш|погиб|сконча|конча|приконч)' | \
    grep -Pv '^поп(?:а|ам|ас|ах|е|ер|ет|ик|ика|ики|ику|ит|ой|ою|у|ы)$' | \
    grep -P '^[а-я]' | sort -u

