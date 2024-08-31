import re
import typing
from urllib.parse import urlparse, parse_qs

from cached_property import cached_property


WEEKDAY_REGEXP = r'({})'.format('|'.join([
    'понедельник',
    'вторник',
    'среда',
    'четверг',
    'пятница',
    'суббота',
    'воскресенье',
]))
MONTH_GEN_REGEXP = r'({})'.format('|'.join([  # gen - родительный падеж
    'января',
    'февраля',
    'марта',
    'апреля',
    'мая',
    'июня',
    'июля',
    'августа',
    'сентября',
    'октября',
    'ноября',
    'декабря',
]))
DAY_REGEXP = r'({})'.format('|'.join(str(i) for i in range(1, 32)))
DATE_REGEXP = \
    rf'(сегодня|завтра|послезавтра|{DAY_REGEXP} {MONTH_GEN_REGEXP}) \({WEEKDAY_REGEXP}\)'
TIME_REGEXP = r'\d{2}:\d{2}'

MARKET_ICON_URL = 'https://avatars.mds.yandex.net/get-marketpic/330747/market-logo/100x100'


class Price(object):
    PURE_REGEXP = r'(от )?\d+ \S+'
    REGEXP = r'(?P<from>от )?(?P<value>\d+) (?P<currency>\S+)'

    def __init__(self, value, currency, is_from_price=False):
        self.value = value
        self.currency = currency
        self.is_from_price = is_from_price

    @classmethod
    def from_match(cls, match):
        return cls(
            int(match.group('value')),
            match.group('currency'),
            match.group('from') is not None
        )

    @classmethod
    def from_str(cls, str_price):
        match = re.fullmatch(cls.REGEXP, str_price)
        if match:
            return cls.from_match(match)

    def __eq__(self, other):
        return self.value == other.value \
            and self.currency == other.currency \
            and self.is_from_price == other.is_from_price

    def __str__(self):
        from_prefix = 'от ' if self.is_from_price else ''
        return f'{from_prefix}{self.value} {self.currency}'

    def __repr__(self):
        from_prefix = 'from ' if self.is_from_price else ''
        return f'<Price {from_prefix}{self.value} {self.currency}>'


class Netlocs(object):
    MARKET = 'market.yandex.ru'
    MARKET_MOBILE = 'm.market.yandex.ru'
    BLUE_MARKET = 'pokupki.market.yandex.ru'
    BLUE_MARKET_MOBILE = 'm.pokupki.market.yandex.ru'

    @classmethod
    def is_white_market(cls, loc):
        return loc in (cls.MARKET, cls.MARKET_MOBILE)

    @classmethod
    def is_blue_market(cls, loc):
        return loc in (cls.BLUE_MARKET, cls.BLUE_MARKET_MOBILE)


class Suggests(object):
    CANCEL = 'Хватит ' + b'\xe2\x9d\x8c'.decode()  # cross
    START_AGAIN = 'Поиск заново ' + b'\xe2\x86\xa9\xef\xb8\x8f'.decode()  # return arrow
    ORDER_WITH_ALICE = 'Заказать с Алисой'
    CHECKOUT = 'Оформить заказ'

    @classmethod
    def get_choice_suggests(cls):
        return [cls.CANCEL, cls.START_AGAIN]

    @classmethod
    def get_product_card_suggests(cls, has_order=False):
        suggests = cls.get_choice_suggests()
        if has_order:
            suggests = [cls.ORDER_WITH_ALICE] + suggests
        return suggests


class Buttons(object):
    ORDER_WITH_ALICE = '<font color="#ffffff">Заказать с Алисой</font>'
    BLUE_GALLERY_ORDER_WITH_ALICE = 'Заказ через Алису'
    CHECKOUT = 'Оформить заказ на Яндекс.Маркете'
    ABOUT_SUPPLIER = 'О продавце'
    ABOUT_PRODUCT = 'О товаре'
    CHECKOUT_TERMS_OF_USE = 'Условия покупки'
    PRODUCT_DETAILS = 'Подробнее о товаре'
    PRODUCT_OFFERS = r'\d+ предложени(е|я|й) на Маркете'
    OPEN_MARKET = 'Посмотреть на Маркете'

    @staticmethod
    def get_gallery_tail_titles(count=r'\d+'):
        return [rf'Посмотреть {count} вариант(|а|ов) на Маркете$']

    @staticmethod
    def get_blue_gallery_tail_titles(count=r'\d+'):
        return [rf'Посмотреть {count} вариант(|а|ов), которы(е|й) можно купить на Маркете$']


class PhraseOption(typing.NamedTuple):
    index: int
    value: str


def get_phrase_options(phrase):
    result = []
    for line in phrase.split('\n'):
        match = re.match(r'(?P<index>\d+)\) (?P<option>.+)', line)
        if match:
            result.append(PhraseOption(int(match['index']), match['option']))
    return result


class ChoicePhrases(object):
    @staticmethod
    def get_activation_phrases():
        return [
            (r'Люблю шопинг! Когда надоест, скажите «Алиса, хватит»\. '
             r'Какой товар вам посоветовать\?'),
            (r'Начнём шопинг\. Не забывайте — когда я увлечена покупками, о другом думать не '
             r'могу\. В общем, если надоест, просто скажите: «Алиса, хватит»\. '
             r'Что вам посоветовать\?'),
            (r'Начнём покупки\. Но помните — когда я увлечена шопингом, в Яндекс смотреть '
             r'некогда\. Если передумаете, скажите «Алиса, хватит»\. Какой товар вам нужен\?'),
             r'Начнём! Если шопинг вас утомит, скажите «Алиса, хватит»\. Что вы ищете\?',
        ]

    @staticmethod
    def get_ask_continue_phrases(query=r'.+'):
        return [
            (rf'С учетом вашего последнего запроса "{query}" вот что я нашла на Маркете\. '
             r'Если какой-то товар понравился - нажмите на него\.'),
            (rf'Могу предложить следующие варианты с Маркета по вашему запросу "{query}"\. '
             r'Если какой-то товар понравился - нажмите на него\.'),
            (rf'Весь Маркет просмотрела, отобрала самое лучшее по запросу "{query}"\. '
             r'Просто нажмите на понравившийся товар.'),
            (rf'Везде все изучила - вот варианты на Маркете по вашему запросу "{query}"\. '
             r'Нажмите на понравившийся\.'),
        ]

    @staticmethod
    def get_ask_continue_blue_phrases(query=r'.+'):
        return [
            (rf'С учетом вашего последнего запроса «{query}» вот что я нашла среди товаров, '
             r'которые можно купить на Яндекс\.Маркете\. '
             r'Если какой-то товар понравился - нажмите на него\.'),
            (rf'Могу предложить следующие варианты по вашему запросу «{query}», '
             r'которые можно купить на Яндекс\.Маркете\. '
             r'Если какой-то товар понравился - нажмите на него\.'),
            (rf'Везде все изучила - вот варианты по вашему запросу «{query}», '
             r'которые можно купить на Яндекс\.Маркете\. '
             r'Нажмите на понравившийся\.'),
        ]

    @staticmethod
    def get_point_to_market_phrases():
        return [
            (r'По вашему запросу нельзя ничего купить прямо на Яндекс\.Маркете\. '
             r'Но я нашла \d+ вариант(а|ов)? в других магазинах\. Смотрите\.'),
        ]

    @staticmethod
    def get_start_again_phrases():
        return ['Хорошо. Какой товар я могу вам посоветовать?']

    @staticmethod
    def get_canceled_phrases():
        return [
            r'Хорошо, вернемся к покупкам в другой раз\.',
            r'Хорошо, вернемся к покупкам немного позже\.',
            r'Как скажете\. Если вы захотите продолжить шоппинг - скажите мне об этом\.',
        ]


class CheckoutPhrases(object):
    @staticmethod
    def get_ask_item_count_phrases():
        return ['Сколько единиц данного товара будем заказывать?']

    @staticmethod
    def get_ask_email_phrases():
        return [
            ('Чтобы продолжить шопинг, вам необходимо авторизоваться и сообщить мне об этом. '
             'Либо просто введите вашу электронную почту.'),
            'Супер! Теперь введите ваш электронный адрес или войдите в личный кабинет в Яндексе.',
            ('Двигаемся к финалу. Пожалуйста, напишите вашу почту или войдите в личный кабинет '
             'в Яндексе.'),
        ]

    @staticmethod
    def get_ask_phone_phrases():
        return [
            r'Есть\! Двигаемся к финалу\. Теперь мне нужен номер вашего телефона\.',
            r'Супер\! Можно оформлять заказ\. Назовите свой телефонный номер\.',
            r'Теперь мне нужен номер вашего телефона\.',
            r'Назовите свой телефонный номер\.',
        ]

    @classmethod
    def get_start_phrases(cls):
        return cls.get_ask_item_count_phrases() + cls.get_ask_email_phrases()

    @staticmethod
    def get_ask_address_phrases():
        return [
            'Теперь мне нужен адрес доставки — город, улица, номер и корпус дома.',
            'Теперь назовите адрес — город, улицу, номер и корпус дома.',
        ]

    @staticmethod
    def get_ask_address_or_select_pvz_phrases():
        return [
            (r'Теперь мне нужен адрес доставки — город, улица, номер и корпус дома\.\n'
             r'Или скажите: "да", если вас устраивает такой вариант:\n'),
            (r'Теперь назовите адрес — город, улицу, номер и корпус дома\.\n'
             r'Или скажите: "да", если вас устраивает такой вариант:\n'),
        ]

    @staticmethod
    def get_ask_address_or_select_option_phrases():
        return [
            (r'Теперь мне нужен адрес доставки — город, улица, номер и корпус дома\.\n'
             r'Или назовите номер одного из вариантов ниже:\n'),
            (r'Теперь назовите адрес — город, улицу, номер и корпус дома\.\n'
             r'Или назовите номер одного из вариантов ниже:\n'),
        ]

    @staticmethod
    def get_delivery_unavailable_phrases():
        return [
            'А сюда товары пока не доставляют. Если хотите, можем изменить адрес',
            'По этому адресу товары пока не доставляют. Можем записать другой',
        ]

    @staticmethod
    def get_ask_delivery_options_phrases():
        return [
            ('Выберите удобный день и время доставки. '
             'Вот варианты — назовите номер самого подходящего.'),
            'Выберите день и время доставки. Вот варианты — назовите номер самого подходящего.',
            ('Выберите подходящее время доставки. '
             'Вот разные варианты — вам нужно только назвать номер.'),
        ]

    @staticmethod
    def get_confirm_order_phrases():
        return [
            'Готово! Если все верно, скажите — «да», и я начну оформлять заказ.',
            'Записала! Если все правильно, скажите — «да», и я начну оформлять заказ.',
        ]

    @staticmethod
    def get_not_confirmed_order_phrases():
        return [
            ('Я пока не научилась изменять заказы. Но вот ссылка, по которой это можно сделать. '
             'Либо можем начать выбирать другой товар.'),
            ('Менять заказы я ещё не умею. Держите ссылку на свой заказ — можете изменить его, '
             'как вздумается. Либо давайте выберем какой-нибудь другой товар.'),
        ]


class ShoppingList(object):
    @staticmethod
    def get_items_added_phrases(items=None):
        items_regexp = '.*'
        if items:
            items_regexp = ', '.join(items[:-1]) + ' и ' + items[-1]
        return [rf'Добавила в ваш список покупок {items_regexp}\.']

    @staticmethod
    def get_item_added_phrases(item=r'.*'):
        return [rf'Добавила {item} в ваш список покупок\.']

    @staticmethod
    def get_duplicate_item_phrases():
        return [r'Такой товар уже есть в вашем списке покупок\.']

    @staticmethod
    def get_duplicate_items_phrases():
        return [r'Такие товары уже есть в вашем списке покупок\.']

    @staticmethod
    def get_items_removed_phrases(items=None):
        items_regexp = '.*'
        if items:
            items_regexp = ', '.join(items[:-1]) + ' и ' + items[-1]
        return [rf'Удалила из вашего списка покупок {items_regexp}\.']

    @staticmethod
    def get_item_removed_phrases(item=r'.*'):
        return [rf'Удалила {item} из вашего списка покупок\.']

    @staticmethod
    def get_all_removed_phrases():
        return [r'Ваш список покупок теперь пуст\.']

    @staticmethod
    def get_nothing_to_remove_phrases():
        return [r'Не нашла такого товара в вашем списке покупок\.']

    @staticmethod
    def get_not_found_phrases():
        return [r'Не нашла такого товара в вашем списке покупок\.']

    @staticmethod
    def get_show_list_card_phrases():
        return [r'Вот что сейчас в вашем списке покупок\.']

    @staticmethod
    def get_show_list_voice_phrases(items=None):
        count_regexp = r'\d+'
        items_regexp = r'.*'
        if items:
            count_regexp = len(items)
            items_regexp = '\n'.join(rf'{i+1}\) {item}\.' for i, item in enumerate(items))
        return [
            rf'В вашем списке покупок сейчас лежит {count_regexp} товар(|а|ов)\.\n{items_regexp}',
        ]

    @staticmethod
    def get_empty_list_phrases():
        return [r'Ваш список покупок пока пуст\.']


class RecurringPurchase(object):
    class OrderHistoryProduct(object):
        def __init__(self, title, price_text):
            self.title = title
            self.price_text = price_text

        @cached_property
        def price(self):
            return Price.from_str(self.price_text)

    # XXX у "Повторной покупки" почему-то свои фразы для авторизации. Ну штош...
    @staticmethod
    def get_ask_login_mobile_phrases():
        return [
            (r'Чтобы продолжить шопинг, вам необходимо авторизоваться\. '
             r'Затем просто повторите ваш запрос\.')
        ]

    @staticmethod
    def get_ask_login_again_mobile_phrases():
        return [
            (r'К сожалению, вы все еще не залогинены\. '
             r'Войдите в свой аккаунт в приложении Яндекс\.'),
            (r'К сожалению, вы все еще не залогинены\. '
             r'Пожалуйста, авторизуйтесь в приложении Яндекс\.'),
            (r'К сожалению, вы все еще не залогинены\. '
             r'Вам нужно войти в свой аккаунт в приложении Яндекс\.'),
        ]

    @staticmethod
    def get_orders_history_phrases():
        return [
            r'Смотрите, что нашлось в вашей истории заказов\.',
            r'Вот что я для вас нашла в вашей истории заказов\.',
        ]

    @staticmethod
    def get_dont_understand_confirmation_phrases():
        return [
            r'Извините, не разобрала\. Скажите, пожалуйста, «да» или «нет»\.'
        ]

    @staticmethod
    def get_goto_market_gallery_phrases():
        return [
            (r'Не нашла похожих товаров в вашей истории заказов\. '
             r'Зато смотрите, что можно купить на Яндекс\.Маркете\.'),
        ]

    @staticmethod
    def get_empty_orders_history_phrases():
        return [
            r'К сожалению, я не нашла данного товара в вашей истории заказов на Яндекс\.Маркете\.',
        ]

    @staticmethod
    def get_voice_order_history_items_phrases():
        return [
            r'Нашла в вашей истории заказов на Яндекс\.Маркете следующие товары:',
        ]

    @staticmethod
    def get_voice_order_history_item_phrases():
        return [(
            r'Нашла в вашей истории заказов на Яндекс\.Маркете товар "(?P<title>.+)" '
            rf'за (?P<price_text>{Price.REGEXP})\. Оформляем заказ\?'
        )]

    @classmethod
    def try_get_order_history_item(cls, text):
        for phrase in cls.get_voice_order_history_item_phrases():
            match = re.fullmatch(phrase, text)
            if match:
                return cls.OrderHistoryProduct(match['title'], match['price_text'])

    @staticmethod
    def get_canceled_phrases():
        return [
            r'Хорошо, вернемся к покупкам в другой раз\.',
            r'Хорошо, вернемся к покупкам немного позже\.',
            r'Как скажете\. Если вы захотите продолжить шоппинг - скажите мне об этом\.',
        ]

    @classmethod
    def get_voice_start_checkout_phrases(cls):
        return cls.get_next_delivery_option_phrases() \
            + CheckoutPhrases.get_ask_item_count_phrases()

    @staticmethod
    def get_next_delivery_option_phrases():
        return [(
            rf'Ближайший возможный интервал доставки:\n(?P<option>{Delivery.OPTION_REGEXP})\.\n'
            r'Могу либо оставить его, либо поискать другие варианты\. Он подходит вам\?'
        )]

    @classmethod
    def try_get_next_delivery_option(cls, text):
        for phrase in cls.get_next_delivery_option_phrases():
            match = re.fullmatch(phrase, text)
            if match:
                return match['option']

    @staticmethod
    def get_order_details_phrases():
        return [(
            r'Вы выбрали (?P<title>.+)\. '
            rf'Цена: (?P<price_text>{Price.PURE_REGEXP})\. '
            r'(Количество: (?P<item_count>\d+)\. )?'
            rf'Доставка (?P<delivery_price>бесплатно|{Price.PURE_REGEXP})\. '
            rf'Всего: (?P<total_price_text>{Price.PURE_REGEXP}) '
            r'Телефон: (?P<phone>[\+\- \d]+)\. '
            r'(Способ доставки: (?P<delivery_type>[\w\s]+)\. )?'
            r'Адрес: (?P<address>.+)\. '
            rf'Доставка: (?P<delivery_option>{Delivery.OPTION_REGEXP})\. '
            r'Способ оплаты: Наличными при получении заказа\. '
            r'Если все правильно, скажите — «да», и я начну оформлять заказ\.'
        )]

    @staticmethod
    def get_order_denied_phrases():
        return [(
            r'Тогда у меня не получится оформить вам заказ\. '
            r'Попробуйте это сделать на Яндекс\.Маркете самостоятельно\.'
        )]

    @classmethod
    def try_get_order_details(cls, text):
        for phrase in cls.get_order_details_phrases():
            match = re.fullmatch(phrase, text)
            if match:
                return match.groupdict()


class OrdersStatusPhrases(object):
    @staticmethod
    def get_no_orders_phrases():
        return [
            (r'К сожалению, у вас нет заказов на Яндекс\.Маркете\. '
             r'Но вы можете что-нибудь выбрать\!'),
        ]

    @staticmethod
    def get_completed_order_phrases():
        return [
            (r'Вот статус вашего последнего заказа:\n'
             r'- \d+: (Вы отменили заказ|Отменен)\.'),
        ]

    @staticmethod
    def get_unfinished_order_phrases():
        return [
            (r'Вот статус вашего незавершенного заказа:\n'
             r'- \d+: В обработке\.'),
        ]

    @staticmethod
    def get_no_orders_voice_phrases():
        return [
            (r'К сожалению, у вас нет заказов на яндекс маркете\. '
             r'Но вы можете что-нибудь выбрать\!'),
        ]

    @staticmethod
    def get_completed_order_voice_phrases():
        return [
            r'Вот статус вашего последнего заказа:\n-',
        ]

    @staticmethod
    def get_unfinished_order_voice_phrases():
        return [
            r'Вот статус вашего незавершенного заказа:\n-',
        ]


class Auth(object):
    @staticmethod
    def get_ask_login_mobile_phrases():
        return [r'Чтобы продолжить, вам необходимо авторизоваться в приложении Яндекс\.']

    @staticmethod
    def get_ask_login_yabro_phrases():
        return [
            r'Чтобы я могла вам помочь, пожалуйста, авторизуйтесь в Яндексе\.',
            r'Чтобы продолжить, пожалуйста, войдите в свой аккаунт на Яндексе\.',
            r'Чтобы я могла вам помочь, пожалуйста, войдите в свой аккаунт Яндекса\.',
        ]

    @staticmethod
    def get_ask_login_again_mobile_phrases():
        return [
            (r'К сожалению, вы все ещё не залогинены\. '
             r'Войдите в свой аккаунт в приложении Яндекс\.'),
            (r'К сожалению, вы все ещё не залогинены\. '
             r'Пожалуйста, авторизуйтесь в приложении Яндекс\.'),
            (r'К сожалению, вы все ещё не залогинены\. '
             r'Вам нужно войти в свой аккаунт в приложении Яндекс\.'),
        ]


class Delivery(object):
    PRODUCT_CARD_REGEXP = rf'Доставка: {DATE_REGEXP} - (бесплатно|\d+ ₽)'
    DATETIME_REGEXP = rf'{DATE_REGEXP}( с {TIME_REGEXP} до {TIME_REGEXP})?'
    DATE_RANGE_REGEXP = rf'{DATE_REGEXP} - {DATE_REGEXP}'
    OPTION_REGEXP = rf'{DATETIME_REGEXP}|{DATE_RANGE_REGEXP}'
    COURIER_REGEXP = r'Курьерская доставка: (?P<address>.+) - (?P<price>бесплатно|\d+ ₽)'
    PVZ_REGEXP = r'Самовывоз: (?P<address>.+) - (?P<price>бесплатно|\d+ ₽)'


def assert_picture(picture_url):
    assert picture_url
    # Проверяем, что не получили картинку-стаб. Такая картинка ставится для товаров без изображения
    # Ожидаем, что по частоным запросам из тестов, всегда будут товары с изображениями
    assert picture_url != 'http://yastatic.net/market-export/_/i/desktop/big-box.png'


def assert_suggests(response, expected_suggests):
    actual = [suggest.title for suggest in response.suggests]
    # TODO(bas1330) Хорошо бы использовать тут стандартное pytest сообщение, но не разобрались,
    # как их добавить в assert'ы из модулей
    assert actual == list(expected_suggests), \
        f'expected suggests: {expected_suggests}\n actual suggests: {actual}'


def assert_has_suggests(response, expected_suggests):
    actual_suggests = {suggest.title for suggest in response.suggests}
    missed_suggests = set(expected_suggests) - actual_suggests
    assert len(missed_suggests) == 0, \
        f'missed suggests: {missed_suggests}\nactual suggests: {actual_suggests}'


def fits_regexps(text, regexps, full=False):
    for regexp in regexps:
        check = re.fullmatch if full else re.match
        if check(regexp, text):
            return True
    return False


def assert_with_regexps(text, regexps, **opts):
    assert fits_regexps(text, regexps, **opts), \
        f'got unexpected value "{text}"\nexpected regexps: {regexps}'


def assert_with_regexp(text, regexp, **opts):
    assert_with_regexps(text, [regexp], **opts)


def assert_has_regexp(text, expected):
    assert re.search(expected, text), f'"{expected}" wasn\'t found in string: {text}'


def assert_extended_gallery(gallery, count=None):
    if count is not None:
        assert len(gallery) == count, f'expected: {count} acutal: {len(gallery)}'
    else:
        assert len(gallery) > 0
    for item in gallery:
        assert_picture(item.picture_url)
        assert item.title
        assert item.price.value > 0, f'expected: >0 acutal: {item.price.value}'
        assert item.price.currency == '₽', f'actual currency: {item.price.currency}'
    assert gallery.market_url_icon == MARKET_ICON_URL, \
        f'expected: {MARKET_ICON_URL} actual: {gallery.market_url_icon}'
    assert_with_regexps(gallery.market_url_caption, Buttons.get_gallery_tail_titles())


def assert_blue_gallery(gallery, count=None, price_to=None, price_from=None):
    # XXX немного копипасты с assert_extended_gallery, но маркет и покупки объединяются,
    # и, вполне возможно, останется только одна extended_gallery
    if count is not None:
        assert len(gallery) == count, f'expected: {count} acutal: {len(gallery)}'
    else:
        assert len(gallery) > 0
    for item in gallery:
        assert_picture(item.picture_url)
        assert item.title
        if price_from is not None:
            assert item.price.value >= price_from, \
                f'expected: >={price_from} acutal: {item.price.value}'
        else:
            assert item.price.value > 0, f'expected: >0 acutal: {item.price.value}'
        if price_to is not None:
            assert item.price.value <= price_to, \
                f'expected: <={price_to} acutal: {item.price.value}'
        assert item.price.currency == '₽', f'actual currency: {item.price.currency}'
    assert gallery.market_url_icon == MARKET_ICON_URL, \
        f'expected: {MARKET_ICON_URL} actual: {gallery.market_url_icon}'
    assert_with_regexps(gallery.market_url_caption, Buttons.get_blue_gallery_tail_titles())


def assert_orders_gallery(gallery, count=None):
    if count is not None:
        assert len(gallery) == count, f'expected: {count} acutal: {len(gallery)}'
    else:
        assert len(gallery) > 0
    for item in gallery:
        assert_picture(item.picture_url)
        assert item.title
        assert item.price.value > 0, f'expected: >0 acutal: {item.price.value}'
        assert item.price.currency == '₽', f'actual currency: {item.price.currency}'
    assert gallery.market_url is None


def assert_voice_purchase_links(
    links,
    model_id=None,
):
    validator = MarketProductUrlValidator(links[Buttons.ABOUT_PRODUCT]) \
        .assert_netloc(Netlocs.BLUE_MARKET_MOBILE)
    if model_id is not None:
        validator.assert_model_id(model_id)

    assert links[Buttons.ABOUT_SUPPLIER].startswith(
        'https://pokupki.market.yandex.ru/suppliers/info-by-offers?'
    )

    assert links[Buttons.CHECKOUT_TERMS_OF_USE] \
        == 'https://yandex.ru/legal/marketplace_termsofuse/'


class _BaseMarketUrlValidator(object):
    def __init__(self, url):
        if url.startswith('https://yandex.ru/turbo?'):
            parsed = urlparse(url)
            cgi = parse_qs(parsed.query)
            url = cgi['text'][0]
        self._parsed = urlparse(url)
        self._cgi = parse_qs(self._parsed.query)
        self.assert_scheme_is_valid()

    def assert_multiple_param(self, param, expected):
        actual = self._cgi[param]
        assert actual == expected, f'"{param}" expected: {expected}, actual: {actual}'
        return self

    def assert_single_param(self, param, value):
        if value is None:
            return self.assert_param_is_not_set(param)
        return self.assert_multiple_param(param, [value])

    def assert_param_is_not_set(self, param):
        assert param not in self._cgi, \
            f'Expected "{param}" to be not set, got value={self._cgi[param]}'
        return self

    def assert_uuid_is_valid(self):
        assert re.fullmatch(r'[a-z0-9]{32}', self._cgi['uuid'][0])
        return self

    def assert_scheme_is_valid(self):
        assert self._parsed.scheme == 'https', f'Actual scheme: {self._parsed.scheme}'
        return self

    def assert_netloc(self, *expected_netlocs):
        assert self._parsed.netloc in expected_netlocs, \
            f'Actual netloc: {self._parsed.netloc}'
        return self


class MarketSearchUrlValidator(_BaseMarketUrlValidator):
    def __init__(self, url):
        super().__init__(url)

        assert Netlocs.is_blue_market(self._parsed.netloc) \
            or Netlocs.is_white_market(self._parsed.netloc), \
            f'Actual netloc: {self._parsed.netloc}'

        if Netlocs.is_blue_market(self._parsed.netloc):
            assert_with_regexps(self._parsed.path, [
                r'/catalog/[\w\d-]+/\d+/list/?',
                r'/search/?',
            ])
        else:
            assert_with_regexps(self._parsed.path, [
                r'/catalog--[\w\d-]+/\d+/list/?',
                r'/search/?',
            ])

        self.assert_uuid_is_valid()


class MarketProductUrlValidator(_BaseMarketUrlValidator):
    def __init__(self, url):
        super().__init__(url)

        assert Netlocs.is_blue_market(self._parsed.netloc) \
            or Netlocs.is_white_market(self._parsed.netloc), \
            f'Actual netloc: {self._parsed.netloc}'

        path_regexp = r'/product/[\w\d-]+/(?P<model_id>\d+)(/(?P<tab>\w+))?/?' \
            if Netlocs.is_blue_market(self._parsed.netloc) \
            else r'/product--[\w\d-]+/(?P<model_id>\d+)(/(?P<tab>\w+))?/?'
        match = re.fullmatch(path_regexp, self._parsed.path)
        assert match
        self._tab = match['tab']
        self._model_id = match['model_id']

        self.assert_uuid_is_valid()

    def assert_tab(self, expected):
        """
        Value=None means main tab
        """
        assert self._tab == expected, f'Actual tab: {self._tab}'
        return self

    def assert_model_id(self, expected):
        assert self._model_id == str(expected), f'Actual model_id: {self._model_id}'
        return self
