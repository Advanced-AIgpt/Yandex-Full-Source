import argparse
import re
from collections import defaultdict

import yt.wrapper as yt
from alice.boltalka.generative.training.data.movies.util import yt_read_rows

OTZOVIK_HARDCODED_MAP = {
    '50 оттенков серого': 'пятьдесят оттенков серого',
    'один плюс один (1+1)': '1+1',
    'мстители 4: финал': 'мстители: финал',
    'лунтик': 'лунтик и его друзья',
    'реалити-шоу дом-2 (тнт)': 'дом 2',
    'хатико - самый верный друг': 'хатико: самый верный друг',
    'чернобыль. зона отчуждения': 'чернобыль: зона отчуждения',
    'лед-2': 'лед 2',
    'джуманджи 2: зов джунглей': 'джуманджи: зов джунглей',
    'мальчишник: часть 3': 'мальчишник: часть iii',
    'тор 3: рагнарек': 'тор: рагнарек',
    'стражи галактики 2': 'стражи галактики. часть 2',
    'аватар 3d': 'аватар',
    'малефисента. владычица тьмы': 'малефисента: владычица тьмы',
    'джентельмены': 'джентльмены',
    'мужское женское': 'мужское-женское',
    'пятая волна': '5-я волна',
    'трансформеры 5: последний рыцарь': 'трансформеры: последний рыцарь',
    'голодные игры: сойка-пересмешница. часть 1': 'голодные игры: сойка-пересмешница. часть i',
    'человек паук: вдали от дома': 'человек-паук: вдали от дома',
    '50 оттенков свободы': 'пятьдесят оттенков свободы',
    'монстры на каникулах 3': 'монстры на каникулах 3: море зовет',
    'чернобыль 2. зона отчуждения': 'чернобыль: зона отчуждения',
    'гуляй, вася': 'гуляй, вася!',
    'возвращение в простоквашино': 'простоквашино',
    'шазам': 'шазам!',
    'обитель зла 6: последняя глава': 'обитель зла: последняя глава',
    '(не) идеальный мужчина': '(не)идеальный мужчина',
    'астрал: глава 3': 'астрал 3',
    'о, боже... мы в фильме ужасов': 'о боже... мы в фильме ужасов',
    'вне себя': 'вне/себя',
    'гоголь.страшная месть': 'гоголь. страшная месть',
    'восемь подруг оушена': '8 подруг оушена',
    'астрал 4. последний ключ': 'астрал 4: последний ключ',
    'всегда говори "да"': 'всегда говори «да»',
    'звездные войны. последние джедаи': 'звездные войны: последние джедаи',
    'дивергент 2: инсургент': 'дивергент, глава 2: инсургент',
    'отель белград': 'отель «белград»',
    'бабушка легкого поведения 2 - престарелые мстители': 'бабушка легкого поведения 2. престарелые мстители',
    'игра престолов 8 сезон': 'игра престолов',
    'люди в черном интернэшнл': 'люди в черном: интернэшнл',
    'статус:свободен': 'статус: свободен',
    'смешарики. пин-код': 'смешарики: пин-код',
    'дивергент 3: за стеной': 'дивергент, глава 3: за стеной',
    'человек-швейцарский нож': 'человек - швейцарский нож',
    'валл-и': 'валл·и',
    'супер нянь': 'superнянь',
    'мажор-3': 'мажор 3',
    'голодные игры: сойка-пересмешница. часть 2': 'голодные игры: сойка-пересмешница. часть ii',
    'орел и решка. перезагрузка': 'орел и решка',
    'кловерфилд 10': 'кловерфилд, 10',
    '50 оттенков черного': 'пятьдесят оттенков черного',
    'агенты щ.и.т.': 'агенты «щ.и.т.»',
    'гарри поттер и дары смерти: часть 2': 'гарри поттер и дары смерти: часть ii',
    'хранители снов 3d': 'хранители снов',
    'отель гранд будапешт': 'отель «гранд будапешт»',
    '11/22/63': '11.22.63',
    'скорый москва-россия': 'скорый «москва-россия»',
    'комедийное телешоу comedy club': 'камеди клаб',
    'эдди "орел': 'эдди «орел»',
    'конец этого чертового мира': 'конец ***го мира',
    'поймай меня, если сможешь!': 'поймай меня, если сможешь',
    'ешь. молись. люби': 'ешь, молись, люби',
    'операция "ы" и другие приключения шурика': 'операция «ы» и другие приключения шурика',
    'любит-не любит': 'любит не любит',
    'вверх!': 'вверх',
    'фиксики. большой секрет': 'фиксики: большой секрет',
    'гарри поттер': 'гарри поттер и философский камень',
    '\'другой мир: войны крови': 'другой мир: войны крови',
    'гарри поттер и дары смерти: часть 1': 'гарри поттер и дары смерти: часть i',
    'миссия невыполнима 6: последствия': 'миссия невыполнима: последствия',
    ' двое: я и моя тень': 'двое: я и моя тень',
    'хан соло. звездные войны. истории': 'хан соло: звездные войны. истории',
    'стартрек. бесконечность': 'стартрек: бесконечность',
    'я-начало': 'я - начало',
    'тест на беременность. 2 сезон': 'тест на беременность 2',
    'кот гром и заколдованный дом 3d': 'кот гром и заколдованный дом',
    'ральф 3d': 'ральф',
    'добро пожаловать в зомбилэнд': 'добро пожаловать в zомбилэнд',
    'полицейский с рублевки. полицейская академия. часть 5': 'полицейский с рублевки',
    'в бой идут одни старики': 'в бой идут одни «старики»',
    'ничего хорошего в отеле эль рояль': 'ничего хорошего в отеле «эль рояль»',
    'лето. одноклассники. любовь (lol)': 'лето. одноклассники. любовь',
    'проект х: дорвались': 'проект x: дорвались',
    'нимфоманка. часть 1': 'нимфоманка: часть 1',
    'как назвать эту любовь': 'как назвать эту любовь?',
    'не могу сказать "прощай': 'не могу сказать «прощай»',
    'королек -птичка певчая': 'королек - птичка певчая',
    'зомбилэнд: контрольный выстрел': 'zомбилэнд: контрольный выстрел',
    'перл харбор': 'перл-харбор',
    'меч 2': 'меч',
    'лучше чем люди': 'лучше, чем люди',
    'если свекровь - монстр': 'если свекровь - монстр…',
    'время приключений с финном и джейком': 'время приключений',
    'американская история х': 'американская история x',
    'бригада. наследник': 'бригада: наследник',
    'зови меня своим именем': 'назови меня своим именем',
    'чернобыль: зона отчуждения финал': 'чернобыль: зона отчуждения. финал',
    'прогулка 3d': 'прогулка',
    'помню - не помню': 'помню - не помню!',
    'грэвити фоллс': 'гравити фолз',
    'геракл: начало легенды 3d': 'геракл: начало легенды',
    'капитан америка: первый мститель': 'первый мститель',
    'чернобыль. зона отчуждения. финал': 'чернобыль: зона отчуждения. финал',
    'шаг вперед 3': 'шаг вперед 3d',
    'мой маленький пони: дружба это магия': 'мой маленький пони',
    'полицейский с рублевки 4': 'полицейский с рублевки',
    'неоспоримый 3: искупление': 'неоспоримый 3',
    'карты, деньги и два ствола': 'карты, деньги, два ствола',
    'крымский мост. сделано с любовью.': 'крымский мост. сделано с любовью!',
    'лего': 'лего фильм',
    'лжец лжец': 'лжец, лжец',
    'женщины против мужчин 2: крымские каникулы': 'женщины против мужчин: крымские каникулы',
    'шаг вперед - 4': 'шаг вперед 4',
    'игра джеральда': 'игра джералда',
    'эксперимент "офис': 'эксперимент «офис»',
    'оно следует за тобой': 'оно',
    'южный парк (south park)': 'южный парк'
}

IRECOMMEND_HARDCODED_MAP = {
    'мультфильм "маша и медведь"': 'маша и медведь',
    'реалити-шоу "дом 2"': 'дом 2',
    'телешоу "битва экстрасенсов"': 'битва экстрасенсов',
    'реалити-шоу "холостяк" на тнт': 'холостяк',
    'хатико самый верный друг': 'хатико: самый верный друг',
    'универ.новая общага': 'универ. новая общага',
    'шоу "голос" на первом канале': 'голос',
    'однажды в... голливуде': 'однажды в… голливуде',
    'джуманджи. зов джунглей.': 'джуманджи: зов джунглей',
    'ток-шоу "званый ужин"': 'званый ужин',
    'сталинград 3d': 'сталинград',
    '"один в один"': 'один в один',
    'странные вещи': 'очень странные дела',
    'шоу "уральские пельмени"': 'уральские пельмени',
    'американская история ужасов: дом-убийца': 'американская история ужасов',
    'сумерки.сага.рассвет часть 1': 'сумерки. сага. рассвет: часть 1',
    'зайцев +1': 'зайцев + 1',
    'ешь.молись.люби.': 'ешь, молись, люби',
    'голодные игры. сойка-пересмешница. часть i': 'голодные игры: сойка-пересмешница. часть i',
    'дьявол носит «prada»': 'дьявол носит prada',
    'аниме унесенные призраками': 'унесенные призраками',
    'ла ла ленд': 'ла-ла ленд',
    'movie 43': 'муви 43',
    'анатомия грей': 'анатомия страсти',
    'шоу "голос.дети"': 'голос',
    'гарри поттер и дары смерти: часть 2': 'гарри поттер и дары смерти: часть ii',
    'comedy club': 'камеди клаб',
    'дракула: неизвестная история': 'дракула',
    'прометей 3d': 'прометей',
    'звёздные войны. эпизод viii. последние джедаи': 'звездные войны: последние джедаи',
    'кингсмэн: секретная служба': 'kingsman: секретная служба',
    'мстители 3d': 'мстители',
    'сумерки.сага. затмение.': 'сумерки. сага. затмение',
    'служебный роман.наше время': 'служебный роман. наше время',
    'американская история ужасов: лечебница': 'американская история ужасов',
    'полный расколбас 18+': 'полный расколбас',
    'sex education': 'половое воспитание',
    'реалити- шоу топ-модель по-американски': 'топ-модель по-американски',
    'аниме ходячий замок': 'ходячий замок',
    'беременна в 16. россия': 'беременна в 16',
    'звоните дикаприо': 'звоните дикаприо!',
    'аниме сэйлор мун': 'красавица-воин сейлор мун эс',
    'телепередача "едим дома"': 'едим дома',
    'красный воробей': 'красный воробей',
    'ледниковый период 4. континентальный дрейф': 'ледниковый период 4: континентальный дрейф',
    'мужское': 'мужское-женское',
    'бригада. наследник': 'бригада: наследник',
    'люди в черном 3 3d': 'люди в черном 3',
    'спирит. душа прерий': 'спирит: душа прерий',
    'ток-шоу холостяк': 'холостяк',
    '007: координаты скайфолл': '007: координаты «скайфолл»',
    'мультфильм "возвращение в простоквашино"': 'простоквашино',
    'дневник бриджет джонс': 'дневник бриджит джонс',
    'ну, погоди': 'ну, погоди!',
    'острые козырьки/ peaky blinders': 'острые козырьки',
    'я – зомби': '',
    'чужестранка/outlander': 'чужестранка',
    'пираты карибского моря 4: на странных берегах': 'пираты карибского моря: на странных берегах',
    'аниме наруто ураганные хроники': 'наруто: ураганные хроники',
    'внутри себя я танцую': '…а в душе я танцую',
    'вечность \\ forever': 'вечность',
    '500 дней лета /': '500 дней лета',
    'новый человек-паук 3d': 'новый человек-паук',
    'на ножах - телеканал пятница': 'на ножах',
    'мир наизнанку с дмитрием комаровым': 'мир наизнанку',
    'соловей разбойник': 'соловей-разбойник',
    'всегда говори «да»': 'всегда говори «да»',
    'звездные войны. эпизод viii. последние джедаи': 'звездные войны: последние джедаи',
    'черная любовь/kara sevda': 'черная любовь',
    'валл-и': 'валл·и',
    'реалити-шоу "взвешенные люди"': 'взвешенные люди',
    'передача "понять, простить"': 'понять. простить',
    'гарри поттер и дары смерти: часть 1': 'гарри поттер и дары смерти: часть i',
    'бегущий в лабиринте. лекарство от смерти.': 'бегущий в лабиринте: лекарство от смерти',
    'дело № 39': 'дело №39',
    'миллионер из трущоб/slumdog millionaire': 'миллионер из трущоб',
    'шрек': 'шрэк',
    'оно/ it follows': 'оно',
    'наша russia. яйца судьбы': 'наша russia: яйца судьбы',
    'сериал альф': 'альф',
    '"следствие вели..."': 'следствие вели…',
    'королек птичка певчая': 'королек - птичка певчая',
    'маша и медведь: машины сказки': 'маша и медведь',
    'развлекательная программа "даешь молодежь!"': 'даешь молодежь!',
    'шрек навсегда': 'шрэк навсегда',
    'отель белград': 'отель «белград»',
    '11.22.63.': '11.22.63',
    'миссия невыполнима 6: последствия': 'миссия невыполнима: последствия',
}


def read_kp_movie_data(path):
    moviename_to_movie = defaultdict(list)
    for row in yt_read_rows(path):
        movie_name_lowered = row['title'].lower().replace('ё', 'е').replace('–', '-').replace('—', '-')

        moviename_to_movie[movie_name_lowered].append(row)
    return moviename_to_movie


def otzovik_name_and_year_parser(text):
    text = text.lower().strip()
    findall = re.findall(r'"(.+)"', text)
    name = findall[0] if len(findall) > 0 else text

    if name in OTZOVIK_HARDCODED_MAP:
        name = OTZOVIK_HARDCODED_MAP[name]

    findall = re.findall(r'\((\d+)\)$', text)
    year = int(findall[0]) if len(findall) > 0 else None
    return name, year


def irecommend_name_and_year_parser(text):
    if text is None:
        return None, None
    text = text.lower().replace('ё', 'е').replace('–', '-').replace('—', '-').strip()
    name = re.sub(r' *\(.+\)', '', text)
    if ' / ' in name:
        name = name.split(' / ')[0]

    if name in IRECOMMEND_HARDCODED_MAP:
        name = IRECOMMEND_HARDCODED_MAP[name]

    findall = re.findall(r'\((\d+)(,?.*)\)$', text)
    year = int(findall[0]) if len(findall) > 0 else None
    return name, year


class MoviedataMapper:
    def __init__(self, name_parser, moviename_to_movie_datas):
        self.name_parser = name_parser
        self.moviename_to_movie_datas = moviename_to_movie_datas

    def __call__(self, row):
        name = row['Result'][0]['_product_header']['product_name_label']
        name, year = self.name_parser(name)
        movie_datas = self.moviename_to_movie_datas.get(name, [])

        set_movie_data = None
        for movie_data in movie_datas:
            if movie_data.get('year', None) == year:
                set_movie_data = movie_data

        if set_movie_data is None:
            set_movie_data = movie_datas[0] if len(movie_datas) > 0 else None

        row['movie_data'] = set_movie_data
        yield row


class MoviedataMapperUgc:
    def __init__(self, moviename_to_movie_datas):
        self.moviename_to_movie_datas = moviename_to_movie_datas
        self.id_to_movie = {}
        for movie_name in self.moviename_to_movie_datas:
            for movie_data in self.moviename_to_movie_datas[movie_name]:
                self.id_to_movie[movie_data['id']] = movie_data

    def __call__(self, row):
        row['movie_data'] = None
        if 'film_id' in row:
            row['movie_data'] = self.id_to_movie.get(row['film_id'], None)
        yield row


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--kp-movie-data-path', default='//home/voice/artemkorenev/boltalka/ugc/3rd_party/kp_movie_data', type=str
    )
    parser.add_argument(
        '--otzovik-data-in', default='//home/voice/artemkorenev/boltalka/ugc/3rd_party/otzovik/data_raw', type=str
    )
    parser.add_argument(
        '--otzovik-data-out',
        default='//home/voice/artemkorenev/boltalka/ugc/3rd_party/otzovik/data_raw_with_movie_data', type=str
    )
    parser.add_argument(
        '--irecommend-data-in', default='//home/voice/artemkorenev/boltalka/ugc/3rd_party/irecommend/data_raw', type=str
    )
    parser.add_argument(
        '--irecommend-data-out',
        default='//home/voice/artemkorenev/boltalka/ugc/3rd_party/irecommend/data_raw_with_movie_data', type=str
    )
    parser.add_argument(
        '--ugc-data-in', default='//home/voice/artemkorenev/boltalka/ugc/3rd_party/ugc/data_raw', type=str
    )
    parser.add_argument(
        '--ugc-data-out', default='//home/voice/artemkorenev/boltalka/ugc/3rd_party/ugc/data_raw_with_movie_data',
        type=str
    )
    args = parser.parse_args()

    moviename_to_movie = read_kp_movie_data(args.kp_movie_data_path)

    yt.run_map(
        MoviedataMapper(otzovik_name_and_year_parser, moviename_to_movie),
        args.otzovik_data_in,
        args.otzovik_data_out,
        memory_limit=yt.common.GB
    )
    yt.run_map(
        MoviedataMapper(irecommend_name_and_year_parser, moviename_to_movie),
        args.irecommend_data_in,
        args.irecommend_data_out,
        memory_limit=yt.common.GB
    )
    yt.run_map(
        MoviedataMapperUgc(moviename_to_movie),
        args.ugc_data_in,
        args.ugc_data_out,
        memory_limit=yt.common.GB
    )


if __name__ == '__main__':
    main()