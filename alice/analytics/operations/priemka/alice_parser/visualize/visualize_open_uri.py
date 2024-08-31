# coding: utf-8

from future import standard_library
standard_library.install_aliases()
import re

import urllib.parse
from urllib.parse import unquote
from alice.analytics.utils.json_utils import get_path


import sys
reload(sys)  # noqa
sys.setdefaultencoding('utf8')  # noqa

from .utils import load_translations
t = load_translations('visualize_open_uri')
t.install()


def _(text):
    """
    HACK: Обёртка над gettext для python2 строк
    вместо: `_ = t.gettext`
    :param str text:
    """
    return t.gettext(text.decode('utf-8'))


def get_open_uri_action(uri, sub_name, record, is_quasar=True):
    """
    Возвращает действие, которое выполнила Алиса для директивы open_uri
    :param str uri:
    :param str sub_name:
    :param dict record:
    :param bool is_quasar:
    :return:
    """
    intent = record['intent']
    if uri.endswith(';end'):
        uri = uri[:-4]

    if sub_name in ('taxi_order_open_app', 'taxi_open_app', 'taxi_open_app_with_order'):
        return _('Открывается приложение или сайт с заказом такси')  # no informative url

    if sub_name == 'taxi_show_legal':
        return _('Открывается/Показывается страница сайта с пользовательским соглашением Яндекс.Такси')

    if sub_name == 'taxi_redirect_to_passport' and not is_quasar:
        return _('Открывается Яндекс.Паспорт для ввода данных')

    if sub_name == 'taxi_call_2_driver':
        return _('Производится звонок водителю Яндекс.Такси')

    if sub_name == 'avia_checkout':
        return _('Открывается сайт Яндекс Авиабилеты, с указанными данными перелета')

    if sub_name == 'navigation_open_app_or_site':
        try:
            if uri:
                if 'browser_fallback_url=' in uri and 'utm_referrer' in uri:
                    uri = unquote(re.search('browser_fallback_url=.+utm_referrer', uri).group(0)[21:-12]) \
                        .encode('iso-8859-1').decode('utf-8')
                elif 'utm_referrer' in uri:
                    uri = unquote(re.search('.+utm_referrer', uri).group(0)[:-12]).encode('iso-8859-1').decode('utf-8')
                if '?&' in uri:
                    uri.replace('?&', '?')
                uri = uri.rstrip('&').rstrip('?').rstrip('/')
                return _('Открывается сайт {} (или соответствующее приложение на устройстве)').format(uri)
        except Exception:
            return _('Открывается указанное приложение или сайт')

    if sub_name == 'navigation_open_site':
        try:
            if uri:
                if 'utm_referrer' in uri:
                    uri = unquote(re.search('.+utm_referrer', uri).group(0)[:-12]).encode('iso-8859-1').decode('utf-8')
                if '?&' in uri:
                    uri.replace('?&', '?')
                uri = uri.rstrip('&').rstrip('?').rstrip('/')
                return _('Открывается сайт ') + uri
        except Exception:
            return _('Открывается указанный сайт')

    if sub_name == 'computer_vision_redirect':
        return _('Показываются похожие картинки в поиске')

    if sub_name == 'computer_vision_image_recognizer':
        return _('Запускает видоискатель поиска по картинке')

    if sub_name == 'yandexauto_launch':
        if uri == 'yandexauto://launch?name=yandexradio':
            return _('Запускается Яндекс.Радио')
        elif uri == 'yandexauto://launch?name=music':
            return _('Запускается Яндекс.Музыка')
        elif uri == 'yandexauto://launch?name=navi':
            return _('Запускается Яндекс.Навигатор')
        elif uri == 'yandexauto://launch?name=weather':
            return _('Запускается Яндекс.Погода')
        elif uri == 'yandexauto://launch?name=settings':
            return _('Открываются настройки Яндекс.Авто')
        return _('Запускается приложение в Яндекс.Авто')

    if sub_name == 'yandexauto_sound':
        if uri == 'yandexauto://sound?action=volume_up':
            return _('Увеличивается уровень громкости в Яндекс.Авто')
        elif uri == 'yandexauto://sound?action=volume_down':
            return _('Уменьшается уровень громкости в Яндекс.Авто')
        elif uri == 'yandexauto://sound?action=mute':
            return _('Выключается звук в Яндекс.Авто')
        elif uri == 'yandexauto://sound?action=unmute':
            return _('Включается звук в Яндекс.Авто')
        return _('Производится управление звуком в Яндекс.Авто')

    if sub_name == 'yandexauto_media_control':
        if uri == 'yandexauto://media_control?source=a2dp':
            return _('Включается bluetooth')
        elif uri == 'yandexauto://media_control?source=fm':
            return _('Включается/выключается радио в мультимедийной системе Яндекс.Авто')
        elif uri == 'yandexauto://media_control?action=pause':
            return _('Вопроизведение ставится на паузу в мультимедийной системе Яндекс.Авто')
        elif uri == 'yandexauto://media_control?action=play':
            return _('Вопроизведение включается в мультимедийной системе Яндекс.Авто')
        elif uri == 'yandexauto://media_control?action=next':
            return _('Включается следующий трек в мультимедийной системе Яндекс.Авто')
        elif uri == 'yandexauto://media_control?action=prev':
            return _('Включается предыдущий трек в мультимедийной системе Яндекс.Авто')
        return _('Производится управление воспроизведением в мультимедийной системе Яндекс.Авто')

    if sub_name == 'yandexauto_fm_radio':
        try:
            if uri.startswith('yandexauto://fm_radio'):
                url = unquote(re.search('name=[^&]*', uri).group(0).replace('name=', '')) \
                    .encode('iso-8859-1').decode('utf-8')
                freq = unquote(re.search('frequency=[^&]*', uri).group(0).replace('frequency=', '')) \
                    .encode('iso-8859-1').decode('utf-8')
                return _('Включается радио "{}", частота - ').format(url) + freq[:-2] + '.' + freq[-2]
        except Exception:
            try:
                if uri.startswith('yandexauto://fm_radio'):
                    url = unquote(re.search('name=[^&]*', uri).group(0).replace('name=', ''))
                    freq = unquote(re.search('frequency=[^&]*', uri).group(0).replace('frequency=', ''))
                    return 'Включается радио "' + url + '", частота - ' + freq[:-2] + '.' + freq[-2]
            except Exception:
                pass
        return _('Включается выбранное радио в Яндекс.Авто')

    if sub_name == 'navi_show_point_on_map':
        return _('Показывается точка на карте в навигаторе')

    if sub_name == 'navi_tanker':
        return _('Показываются или скрываются заправки в навигаторе')

    if sub_name == 'navi_map_search':
        try:
            if uri.startswith('yandexnavi://map_search'):
                url = unquote(re.search('text=[^&]*', uri).group(0).replace('text=', '')) \
                    .encode('iso-8859-1').decode('utf-8')
                return _('Открывается поиск на карте в навигаторе по запросу <{}>').format(url)
        except:
            try:
                url = unquote(re.search('text=[^&]*', uri).group(0).replace('text=', '')).decode('utf-8')
                return _('Открывается поиск на карте в навигаторе по запросу <{}>').format(url)
            except:
                pass
        return _('Осуществляется поиск на карте в навигаторе по заданному запросу')

    if sub_name == 'navi_set_place':
        return _('Производится установка дома/работы в навигаторе')

    if sub_name == 'navi_auth_for_refuel':
        return _('Показывается сообщение: "Необходима авторизация для оплаты заправки через приложение"')

    if sub_name == 'navi_add_point':
        comment = ''
        try:
            if uri.startswith('yandexnavi://add_point'):
                comment = unquote(re.search('comment=[^&]*', uri).group(0).replace('comment=', '')) \
                    .encode('iso-8859-1').decode('utf-8')
        except UnicodeEncodeError:
            comment = unquote(re.search('comment=[^&]*', uri).group(0).replace('comment=', ''))
        except AttributeError:
            pass
        if comment:
            return _('Устанавливается дорожное событие в навигаторе с комментарием "{}"').format(comment)
        return _('Производится постановка дорожных событий в навигаторе')

    if sub_name == 'navi_clear_route':
        return _('Сбрасывается построенный маршрут')

    if sub_name == 'navi_set_sound_scheme':
        return _('Устанавливается выбранный голос помощника')

    if sub_name == 'navi_build_route_on_map':
        return _('Строится маршрут в навигаторе до указанной точки')

    if sub_name == 'navi_external_confirmation':
        if uri == 'yandexnavi://external_confirmation?confirmed=1':
            return _('Начинается ведение по построенному маршруту в навигаторе')
        if uri == 'yandexnavi://external_confirmation?confirmed=0':
            return _('Сбрасывается построенный маршрут')

    if sub_name == 'navi_carparks_route':
        return _('Производится построение маршрута в навигаторе до парковочного места')

    if sub_name == 'navi_show_ui':
        return _('Открывается приложение в навигаторе')

    if sub_name == 'navi_show_ui/bookmarks' or sub_name == 'navi_show_ui_bookmarks':
        return _('Открывается экран закладок в навигаторе')

    if sub_name == 'navi_show_ui/map' or sub_name == 'navi_show_ui_map':
        if uri == 'yandexnavi://show_ui/map?carparks_enabled=1':
            return _('Открывается экран "Карта" в навигаторе с отображенными парковками')
        return _('Открывается экран "Карта" в навигаторе с указанными параметрами')

    if sub_name == 'navi_show_ui/map/travel' or sub_name == 'navi_show_ui_map_travel':
        return _('Открывается экран "Ведения по маршруту" в навигаторе')

    if sub_name == 'navi_show_ui/map/parkings' or sub_name == 'navi_show_ui_map_parkings':
        return _('Открывается экран "Оплата парковок" в навигаторе')

    if sub_name == 'navi_show_ui/map/voice_control' or sub_name == 'navi_show_ui_map_voice_control':
        return _('Открывается экран "Голосовой ввод" в навигаторе')

    if sub_name == 'navi_show_ui/menu' or sub_name == 'navi_show_ui_menu':
        return _('Открывается экран "Меню" в навигаторе')

    if sub_name == 'navi_show_ui/menu/fines' or sub_name == 'navi_show_ui_menu_fines':
        return _('Открывается экран "Оплата штрафов" в навигаторе')

    if sub_name == 'navi_show_ui/menu/statistics' or sub_name == 'navi_show_ui_menu_statistics':
        return _('Открывается экран "Мои поездки" в навигаторе')

    if sub_name == 'navi_show_ui/menu/settings' or sub_name == 'navi_show_ui_menu_settings':
        return _('Открывается экран "Настройки" в навигаторе')

    if sub_name == 'navi_show_ui/menu/gas_stations' or sub_name == 'navi_show_ui_menu_gas_stations':
        return _('Открывается экран "Меню -> Заправки" в навигаторе')

    if sub_name == 'navi_show_ui/menu/settings_mastercard' or sub_name == 'navi_show_ui_menu_settings_mastercard':
        return _('Открывается экран "Mastercard" (с типами карт) в навигаторе')

    if sub_name == 'navi_show_ui/menu/gas_stations_manual' or sub_name == 'navi_show_ui_menu_gas_stations_manual':
        return _('Открывается экран "Меню -> Заправки -> Как это работает" в навигаторе')

    if sub_name == 'navi_show_ui/menu/gas_stations_support' or sub_name == 'navi_show_ui_menu_gas_stations_support':
        return _('Открывается экран "Меню -> Заправки -> Служба поддержки" в навигаторе')

    if sub_name == 'navi_show_ui/menu/gas_stations_payment' or sub_name == 'navi_show_ui_menu_gas_stations_payment':
        return _('Открывается экран "Меню -> Заправки -> Способ оплаты" в навигаторе')

    if sub_name == 'navi_show_ui/menu/offers' or sub_name == 'navi_show_ui_menu':
        return _('Открывается экран "Скидки и Подарки" в навигаторе')

    if sub_name == 'navi_show_user_position':
        return _('Показывается месторасположение пользователя')
    # Производится отцентрирование пользователя по геопозиции в навигаторе звучит корректнее, но непонятно:)

    if sub_name == 'navi_traffic' or sub_name == 'navi_show_traffic' or sub_name == 'navi_layer_traffic':
        if uri == 'yandexnavi://traffic?traffic_on=1':
            return _('Отображается слой пробок в навигаторе')
        if uri == 'yandexnavi://traffic?traffic_on=0':
            return _('Скрывается слой пробок в навигаторе')
        return _('Производится управление слоем пробок в навигаторе')

    if sub_name == 'navi_set_setting':
        if uri == 'yandexnavi://set_setting?name=rasterMode&value=Map':
            return _('Открывается схематичное изображение карты (без спутникового изображения)')
        if uri == 'yandexnavi://set_setting?name=rasterMode&value=Sat':
            return _('Открывается спутниковое изображение карты')

    if sub_name == 'navi_update_flag_setting':
        return _('Производится установка части флагов настройки в навигаторе')

    if sub_name == 'music_yamusic_play':
        try:
            if uri:
                url = unquote(re.search('browser_fallback_url=.+', uri).group(0)[21:]) \
                    .encode('iso-8859-1').decode('utf-8')
                if url.find('?from=alice') != -1:
                    url = url[:url.find('?from=alice')]
                url = url.rstrip('/')
                return _('Включается {} в Яндекс.Музыке').format(url)
        except Exception:
            return _('Воспроизводится музыка в Яндекс.Музыке')

    if sub_name == 'music_ambient_sound':
        return _('Включаются звуки окружающей среды')

    if sub_name == 'music_hardcoded_sound':
        return _('Включается музыка (это может быть подкаст)')

    if sub_name == 'music_quasar_play':
        return _('Воспроизводится музыка в Яндекс.Станции')

    if sub_name == 'music_snippet_play':
        try:
            url = unquote(re.search('fallback_url=.*', uri).group(0)).encode('iso-8859-1').decode('utf-8')[13:]
            if url.find('?from=alice') != -1:
                url = url[:url.find('?from=alice')]
            url = url.rstrip('?').rstrip('/')
            return _('Включается ') + url
        except (AttributeError, NameError):
            pass
        return _('Включается указанная музыка')

    if sub_name == 'translate_yandex':
        return _('Открывается Яндекс.Переводчик')

    if sub_name == 'translate_google':
        return _('Открывается Google.Translate')

    if sub_name == 'translate_empty':
        return _('Открывается переводчик')

    if sub_name == 'phone_call':
        return _('Осуществляется звонок выбранному абоненту')

    if sub_name == 'serp_gallery_call':
        return _('Осуществляется звонок по номеру, найденному в выдаче поиска')

    if sub_name == 'serp_gallery_open':
        return _('Открывается сайт, найденный в выдаче поиска')

    if sub_name == 'market_go_to_shop':
        return _('Осуществляется переход на страницу продукта на сайте магазина')

    if sub_name == 'reminder_show':
        return _('Открываются напоминания через сайт https://calendar.yandex.ru/todo')
        # происходит в случае отмены каких-либо сценариев в напоминании

    if sub_name == 'todo_reminder_show':
        return _('Открываются напоминания через сайт https://calendar.yandex.ru/todo')

    if sub_name == 'video_search_video':
        return _('Производится поиск видео с последующим воспроизведением контента')
        # Если устройство не поддерживает такую функцию, то будет автоматическое открытие данного видео.
        # Такая ситуация возможна при фразах 'воспроизведи' и ее производные

    if sub_name == 'video_search':
        return _('Производится поиск видео. Автовоспроизведение видео не происходит')
        # Такая ситуация возможна при фразах 'порекомендуй', 'найди' и другие

    if sub_name == 'personal_assistant.scenarios.show_route__show_route_on_map':
        return _('Необходимый маршрут отображается на карте')  # no informative url

    if sub_name == 'personal_assistant.scenarios.find_poi__show_on_map':
        return _('Найденная организация показывается на карте')

    if sub_name.startswith('personal_assistant.scenarios.get_my_location'):
        return _('Отображается информация о местоположении пользователя')

    if sub_name == 'personal_assistant.scenarios.find_poi__call':
        return _('Совершается звонок в найденную на карте организацию')

    if sub_name == 'personal_assistant.scenarios.search__show_on_map':
        return _('Показываются результаты поиска на карте')

    if sub_name == 'personal_assistant.scenarios.search':
        if uri.startswith('intent:?url=yandexmusic'):
            if 'fallback_url=' in uri and 'utm_referrer' in uri:
                uri = unquote(re.search('fallback_url=.+utm_referrer', uri).group(0)[13:-12]) \
                    .encode('iso-8859-1').decode('utf-8')
            elif 'utm_referrer' in uri:
                uri = unquote(re.search('.+utm_referrer', uri).group(0)[:-12]).encode('iso-8859-1').decode('utf-8')
            if '?&' in uri:
                uri.replace('?&', '?')
            uri = uri.rstrip('&').rstrip('?').rstrip('/')
            return _('Открывается сайт {} (или соответствующее приложение на устройстве)').format(uri)

    if sub_name == 'market_open_serp_search':
        return _('Производится поиск по запросу <{}>').format(
            unquote(re.search('text=[^&]*', uri).group(0).replace('text=', ''))
        )

    if 'device-discovery' in uri or 'add-device' in uri:
        return _('Алиса открывает приложение для подключения новых устройств Умного Дома')

    offline_maps_intent_ios = 'intent:?url=yandexmaps%3A%2F%2Fyandex.ru%2Fmaps%2Foffline-maps'
    if uri.startswith('yandexmaps://maps.yandex.ru/offline-maps') or uri.startswith('intent://yandex.ru/maps/offline-maps') or uri.startswith(offline_maps_intent_ios):
        if uri.startswith(offline_maps_intent_ios):
            uri = unquote(uri)
        msg = _('Открываются Яндекс.Карты и начинается загрузка офлайн-карты ')
        if uri.find('search_by=name') != -1:
            name_from = uri.find('name=')
            name_to = uri.find('&', name_from)
            if name_from != -1 and name_to != -1:
                name = uri[name_from + 5:name_to]
                msg += '\'' + unquote(name) + '\''
            else:
                msg += 'ОШИБКА'
        else:
            msg += _('видимой области')
        return msg

    if uri.startswith('zen://open_feed?'):
        param = 'export_params=alice_search%3D'
        query_from = uri.find(param)
        query_to = uri.find('&', query_from)
        text = _('ОШИБКА')
        if query_from != -1 and query_to != -1:
            query = uri[query_from + len(param):query_to]
            text = unquote(query.replace('+', '%20'))

        return _('Открывается лента Дзена с публикациями, похожими на <{}>').format(text)

    try:
        parsed_uri = urllib.parse.urlparse(uri)
    except:
        parsed_uri = urllib.parse.urlparse(uri.decode('utf-8'))

    if uri.startswith('https://yandex.ru/products/search?') or 'viewport_id=products' in uri:
        product_search = _('ОШИБКА')
        if parsed_uri and parsed_uri.query:
            try:
                query_text = urllib.parse.parse_qs(parsed_uri.query).get('text', [None])[0]
            except:
                try:
                    query_text = urllib.parse.parse_qs(parsed_uri.query.decode('utf-8')).get('text', [None])[0]
                except:
                    query_text = None
            if query_text:
                product_search = query_text

        if product_search == _('ОШИБКА') and record.get('generic_scenario') == 'goods':
            semantic_frame_object = get_path(record, ['analytics_info', 'analytics_info', 'Goods', 'matched_semantic_frames', 0, 'slots', 0, 'value'])
            if semantic_frame_object:
                product_search = semantic_frame_object

        return _('Открывается поиск по товарам по запросу <{}>').format(product_search)

    try:
        if uri.find('translate.ya') != -1:
            url = re.search('text=[^&]*', uri).group(0).replace('text=', '')
            query = unquote(url).encode('iso-8859-1').decode('utf-8')
            return _('Открывается перевод ({}) текста <{}>').format(uri.split('lang=')[-1], query)

        # sub_name == 'personal_assistant.scenarios.search', 'serp_search' и
        # 'personal_assistant.scenarios.search__serp' обрабатываем здесь же
        elif 'viewport_id=serp' in uri or intent.endswith('search') or (uri.find('yandex') != -1 and uri.find('search') != -1):
            url = re.search('text=[^&]*', uri).group(0).replace('text=', '')
            query = unquote(url).encode('iso-8859-1').decode('utf-8')
            return _('Открывается поиск по запросу <{}>').format(query)
    except:
        try:
            if uri.find('translate.ya') != -1:
                url = re.search('text=[^&]*', uri).group(0).replace('text=', '')
                query = unquote(url).decode('utf-8')
                return _('Открывается перевод ({}) текста <{}>').format(uri.split('lang=')[-1], query)

            # sub_name == 'personal_assistant.scenarios.search', 'serp_search' и
            # 'personal_assistant.scenarios.search__serp' обрабатываем здесь же
            elif 'viewport_id=serp' in uri or intent.endswith('search') or (uri.find('yandex') != -1 and uri.find('search') != -1):
                url = re.search('text=[^&]*', uri).group(0).replace('text=', '')
                query = unquote(url).decode('utf-8')
                return _('Открывается поиск по запросу <{}>').format(query)
        except:
            return _('Открывается указанное приложение/сайт')

    if intent.endswith('map'):
        return _('Открываются Яндекс.Карты')

    elif intent.find('weather') != -1:
        return _('Открывается Яндекс.Погода')

    elif uri.startswith('http:') and len(uri) < 35:
        return _('Открывается веб-страница ') + (uri[7:-1] if uri.endswith('/') else uri[7:])

    elif uri.startswith('https:') and len(uri) < 35:
        return _('Открывается веб-страница ') + (uri[8:-1] if uri.endswith('/') else uri[8:])

    elif intent.endswith('open_site_or_app'):
        return _('Открывается указанное приложение/сайт')

    elif intent.endswith('call'):
        return _('Совершается вызов по указанному номеру')

    if uri.startswith('yandexbrowser-open-url://app-settings%3A%2F%2F'):
        return _('Открываются настройки приложения Яндекс Браузер')

    if is_quasar:
        return _('Производится поиск по указанному запросу')
    else:
        return _('Открывается указанное приложение/сайт')
