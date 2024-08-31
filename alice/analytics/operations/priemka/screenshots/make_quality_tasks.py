#!/usr/bin/env python3
# -*- coding: utf8 -*-
import sys
import argparse
import json
import codecs
import re
from urllib.parse import unquote
from datetime import datetime, timedelta


def get_action(intent, directives_dict, language):
    def check_sub_name(name, subname):
        return name in directives_dict and directives_dict[name][1] == subname

    if language == 'tr':
        if 'open_uri' in directives_dict.keys():
            sub_name = directives_dict['open_uri'][1]

            if sub_name == 'navi_add_point':
                return "Yol uyarısı yapılıyor"

            if sub_name == 'navi_traffic':
                return 'Navigasyonun trafik katmanı ayarlanıyor'

            if sub_name == 'navi_build_route_on_map':
                return 'Navigasyon uygulamasında rota oluşturuluyor'

            if sub_name == 'navi_map_search':
                return 'Sorgu haritada aranıyor'

        return "Cevap ekranda gösterildi"

    if 'open_uri' in directives_dict.keys():
        payload = directives_dict['open_uri']
        uri = payload[0]['uri']
        if uri.endswith(";end"):
            uri = uri[:-4]
        sub_name = payload[1]

        if sub_name in ("taxi_order_open_app", "taxi_open_app"):
            return u'Открывается приложение или сайт с заказом такси'  # no informative url

        if sub_name == "taxi_show_legal":
            return u'Открывается/Показывается страница сайта с пользовательским соглашением Яндекс.Такси'

        if sub_name == "taxi_call_2_driver":
            return u'Производится звонок водителю Яндекс.Такси'

        if sub_name == "avia_checkout":
            return u'Открывается сайт Яндекс Авиабилеты, с указанными данными перелета'

        if sub_name == "navigation_open_app_or_site":
            try:
                if uri:
                    if "browser_fallback_url=" in uri and "utm_referrer" in uri:
                        uri = unquote(re.search("browser_fallback_url=.+utm_referrer", uri).group(0)[21:-12])
                    elif "utm_referrer" in uri:
                        uri = unquote(re.search(".+utm_referrer", uri).group(0)[:-12])
                    if "?&" in uri:
                        uri.replace("?&", "?")
                    uri = uri.rstrip("&").rstrip("?").rstrip("/")
                    return u'Открывается сайт ' + uri + u' (или соответствующее приложение на устройстве)'
            except Exception:
                return u'Открывается указанное приложение или сайт'

        if sub_name == "navigation_open_site":
            try:
                if uri:
                    if "utm_referrer" in uri:
                        uri = unquote(re.search(".+utm_referrer", uri).group(0)[:-12])
                    if "?&" in uri:
                        uri.replace("?&", "?")
                    uri = uri.rstrip("&").rstrip("?").rstrip("/")
                    return u'Открывается сайт ' + uri
            except Exception:
                return u'Открывается указанный сайт'

        if sub_name == "computer_vision_redirect":
            return u'Показываются похожие картинки в поиске'

        if sub_name == "computer_vision_image_recognizer":
            return u'Запускает видоискатель поиска по картинке'

        if sub_name == "yandexauto_launch":
            if uri == "yandexauto://launch?name=yandexradio":
                return u'Запускается Яндекс.Радио'
            elif uri == "yandexauto://launch?name=navi":
                return u'Запускается Яндекс.Навигатор'
            elif uri == "yandexauto://launch?name=weather":
                return u'Запускается Яндекс.Погода'
            return u'Запускается приложение в Яндекс.Авто'

        if sub_name == "yandexauto_sound":
            if uri == "yandexauto://sound?action=volume_up":
                return u'Увеличивается уровень громкости в Яндекс.Авто'
            elif uri == "yandexauto://sound?action=volume_down":
                return u'Уменьшается уровень громкости в Яндекс.Авто'
            elif uri == "yandexauto://sound?action=mute":
                return u'Выключается звук в Яндекс.Авто'
            elif uri == "yandexauto://sound?action=unmute":
                return u'Включается звук в Яндекс.Авто'
            return u'Производится управление звуком в Яндекс.Авто'

        if sub_name == "yandexauto_media_control":
            if uri == "yandexauto://media_control?source=a2dp":
                return u'Включается bluetooth'
            elif uri == "yandexauto://media_control?source=fm":
                return u'Включается/выключается радио в мультимедийной системе Яндекс.Авто'
            elif uri == "yandexauto://media_control?action=pause":
                return u'Вопроизведение ставится на паузу в мультимедийной системе Яндекс.Авто'
            elif uri == "yandexauto://media_control?action=play":
                return u'Вопроизведение включается в мультимедийной системе Яндекс.Авто'
            elif uri == "yandexauto://media_control?action=next":
                return u'Включается следующий трек в мультимедийной системе Яндекс.Авто'
            elif uri == "yandexauto://media_control?action=prev":
                return u'Включается предыдущий трек в мультимедийной системе Яндекс.Авто'
            return u'Производится управление воспроизведением в мультимедийной системе Яндекс.Авто'

        if sub_name == "yandexauto_fm_radio":
            try:
                if uri.startswith('yandexauto://fm_radio'):
                    url = unquote(re.search('name=[^&]*', uri).group(0).replace('name=', ''))
                    freq = unquote(re.search('frequency=[^&]*', uri).group(0).replace('frequency=', ''))
                    return u'Включается радио "' + url + u'", частота - ' + freq[:-2] + u'.' + freq[-2]
            except AttributeError:
                pass
            return u'Включается выбранное радио в Яндекс.Авто'

        if sub_name == "navi_show_point_on_map":
            return u'Показывается точка на карте в навигаторе'

        if sub_name == "navi_tanker":
            return u'Показываются или скрываются заправки в навигаторе'

        if sub_name == "navi_map_search":
            try:
                if uri.startswith('yandexnavi://map_search'):
                    url = unquote(re.search('text=[^&]*', uri).group(0).replace('text=', ''))
                    return u'Открывается поиск на карте в навигаторе по запросу <' + url + u'>'
            except AttributeError:
                pass
            return u'Осуществляется поиск на карте в навигаторе по заданному запросу'

        if sub_name == "navi_set_place":
            return u'Производится установка дома/работы в навигаторе'

        if sub_name == "navi_auth_for_refuel":
            return u'Показывается сообщение: "Необходима авторизация для оплаты заправки через приложение"'

        if sub_name == "navi_show_point_on_map":
            return u'Показывается точка на карте в навигаторе'

        if sub_name == "navi_add_point":
            try:
                if uri.startswith('yandexnavi://add_point'):
                    url = unquote(re.search('comment=[^&]*', uri).group(0).replace('comment=', ''))
                    return u'Устанавливается дорожное событие в навигаторе с комментарием "' + url + u'"'
            except AttributeError:
                pass
            return u'Производится постановка дорожных событий в навигаторе'

        if sub_name == "navi_clear_route":
            return u'Сбрасывается построенный маршрут'

        if sub_name == "navi_set_sound_scheme":
            return u'Устанавливается выбранный голос помощника'

        if sub_name == "navi_build_route_on_map":
            return u'Начинается ведение по построенному маршруту в навигаторе'

        if sub_name == "navi_carparks_route":
            return u'Производится построение маршрута в навигаторе до парковочного места'

        if sub_name == "navi_show_ui":
            return u'Открывается приложение в навигаторе'

        if sub_name == "navi_show_ui/bookmarks" or sub_name == "navi_show_ui_bookmarks":
            return u'Открывается экран закладок в навигаторе'

        if sub_name == "navi_show_ui/map" or sub_name == "navi_show_ui_map":
            if uri == "yandexnavi://show_ui/map?carparks_enabled=1":
                return u'Открывается экран "Карта" в навигаторе с отображенными парковками'
            return u'Открывается экран "Карта" в навигаторе с указанными параметрами'

        if sub_name == "navi_show_ui/map/travel" or sub_name == "navi_show_ui_map_travel":
            return u'Открывается экран "Ведения по маршруту" в навигаторе'

        if sub_name == "navi_show_ui/map/parkings" or sub_name == "navi_show_ui_map_parkings":
            return u'Открывается экран "Оплата парковок" в навигаторе'

        if sub_name == "navi_show_ui/map/voice_control" or sub_name == "navi_show_ui_map_voice_control":
            return u'Открывается экран "Голосовой ввод" в навигаторе'

        if sub_name == "navi_show_ui/menu" or sub_name == "navi_show_ui_menu":
            return u'Открывается экран "Меню" в навигаторе'

        if sub_name == "navi_show_ui/menu/fines" or sub_name == "navi_show_ui_menu_fines":
            return u'Открывается экран "Оплата штрафов" в навигаторе'

        if sub_name == "navi_show_ui/menu/statistics" or sub_name == "navi_show_ui_menu_statistics":
            return u'Открывается экран "Мои поездки" в навигаторе'

        if sub_name == "navi_show_ui/menu/settings" or sub_name == "navi_show_ui_menu_settings":
            return u'Открывается экран "Настройки" в навигаторе'

        if sub_name == "navi_show_ui/menu/gas_stations" or sub_name == "navi_show_ui_menu_gas_stations":
            return u'Открывается экран "Меню -> Заправки" в навигаторе'

        if sub_name == "navi_show_ui/menu/settings_mastercard" or sub_name == "navi_show_ui_menu_settings_mastercard":
            return u'Открывается экран "Mastercard" (с типами карт) в навигаторе'

        if sub_name == "navi_show_ui/menu/gas_stations_manual" or sub_name == "navi_show_ui_menu_gas_stations_manual":
            return u'Открывается экран "Меню -> Заправки -> Как это работает" в навигаторе'

        if sub_name == "navi_show_ui/menu/gas_stations_support" or sub_name == "navi_show_ui_menu_gas_stations_support":
            return u'Открывается экран "Меню -> Заправки -> Служба поддержки" в навигаторе'

        if sub_name == "navi_show_ui/menu/gas_stations_payment" or sub_name == "navi_show_ui_menu_gas_stations_payment":
            return u'Открывается экран "Меню -> Заправки -> Способ оплаты" в навигаторе'

        if sub_name == "navi_show_ui/menu/offers" or sub_name == "navi_show_ui_menu":
            return u'Открывается экран "Скидки и Подарки" в навигаторе'

        if sub_name == "navi_show_user_position":
            return u'Показывается месторасположение пользователя'
        # Производится отцентрирование пользователя по геопозиции в навигаторе звучит корректнее, но непонятно:)

        if sub_name == "navi_traffic" or sub_name == "navi_show_traffic":
            if uri == "yandexnavi://traffic?traffic_on=1":
                return u'Отображается слой пробок в навигаторе'
            if uri == "yandexnavi://traffic?traffic_on=0":
                return u'Скрывается слой пробок в навигаторе'
            return u'Производится управление слоем пробок в навигаторе'

        if sub_name == "navi_update_flag_setting":
            return u'Производится установка части флагов настройки в навигаторе'

        if sub_name == "music_yaradio_play":
            if uri.startswith("intent://radio/user/onyourwave"):
                return u'Включается персональная подборка в Яндекс.Радио (https://radio.yandex.ru/user/onyourwave)'
            if uri.startswith("https://radio.yandex.ru/user/onyourwave"):  # yandex auto
                return u'Включается персональная подборка в Яндекс.Радио (https://radio.yandex.ru/user/onyourwave)'
            elif uri.startswith("https://radio.yandex.ru"):  # yandex auto
                if uri.find("?from=alice") != -1:
                    return u'Включается подборка ' + uri[:uri.find("?from=alice")] + u' в Яндекс.Радио'
            try:
                if uri:
                    url = unquote(re.search("browser_fallback_url=.+", uri).group(0)[21:])
                    if url.find("?from=alice") != -1:
                        url = url[:url.find("?from=alice")]
                    url = url.rstrip("/")
                    return u'Включается радио ' + url
            except Exception:
                return u'Включается музыка в Яндекс.Радио'

        if sub_name == "music_yamusic_play":
            try:
                if uri:
                    url = unquote(re.search("browser_fallback_url=.+", uri).group(0)[21:])
                    if url.find("?from=alice") != -1:
                        url = url[:url.find("?from=alice")]
                    url = url.rstrip("/")
                    return u'Включается ' + url + u' в Яндекс.Музыке'
            except Exception:
                return u'Воспроизводится музыка в Яндекс.Музыке'

        if sub_name == "music_ambient_sound":
            return u'Включаются звуки окружающей среды'

        if sub_name == "music_hardcoded_sound":
            return u'Включается музыка (это может быть подкаст)'

        if sub_name == "music_quasar_play":
            return u'Воспроизводится музыка в Яндекс.Станции'

        if sub_name == "music_snippet_play":
            try:
                url = unquote(re.search("fallback_url=.*", uri).group(0))[13:]
                if url.find("?from=alice") != -1:
                    url = url[:url.find("?from=alice")]
                url = url.rstrip("?").rstrip("/")
                return u'Включается ' + url
            except (AttributeError, NameError):
                pass
            return u'Включается указанная музыка'

        if sub_name == "translate_yandex":
            return u'Открывается Яндекс.Переводчик'

        if sub_name == "translate_google":
            return u'Открывается Google.Translate'

        if sub_name == "translate_empty":
            return u'Открывается переводчик'

        if sub_name == "phone_call":
            return u'Осуществляется звонок выбранному абоненту'

        if sub_name == "serp_gallery_call":
            return u'Осуществляется звонок по номеру, найденному в выдаче поиска'

        if sub_name == "serp_gallery_open":
            return u'Открывается сайт, найденный в выдаче поиска'

        if sub_name == "market_go_to_shop":
            return u'Осуществляется переход на страницу продукта на сайте магазина'

        if sub_name == "reminder_show":
            return u'Открываются напоминания через сайт https://calendar.yandex.ru/todo'
            # происходит в случае отмены каких-либо сценариев в напоминании

        if sub_name == "todo_reminder_show":
            return u'Открываются напоминания через сайт https://calendar.yandex.ru/todo'

        if sub_name == "video_search_video":
            return u'Производится поиск видео с последующим воспроизведением контента'
            # Если устройство не поддерживает такую функцию, то будет автоматическое открытие данного видео.
            # Такая ситуация возможна при фразах "воспроизведи" и ее производные

        if sub_name == "video_search":
            return u'Производится поиск видео. Автовоспроизведение видео не происходит'
            # Такая ситуация возможна при фразах "порекомендуй", "найди" и другие

        if sub_name == "personal_assistant.scenarios.show_route__show_route_on_map":
            return u'Необходимый маршрут отображается на карте'  # no informative url

        if sub_name == "personal_assistant.scenarios.find_poi__show_on_map":
            return u'Найденная организация показывается на карте'

        if sub_name.startswith("personal_assistant.scenarios.get_my_location"):
            return u'Отображается информация о местоположении пользователя'

        if sub_name == "personal_assistant.scenarios.find_poi__call":
            return u'Совершается звонок в найденную на карте организацию'

        if sub_name == "personal_assistant.scenarios.search__show_on_map":
            return u'Показываются результаты поиска на карте'

        if sub_name == "personal_assistant.scenarios.search":
            if uri.startswith("intent:?url=yandexmusic"):
                if "fallback_url=" in uri and "utm_referrer" in uri:
                    uri = unquote(re.search("fallback_url=.+utm_referrer", uri).group(0)[13:-12])
                elif "utm_referrer" in uri:
                    uri = unquote(re.search(".+utm_referrer", uri).group(0)[:-12])
                if "?&" in uri:
                    uri.replace("?&", "?")
                uri = uri.rstrip("&").rstrip("?").rstrip("/")
                return u'Открывается сайт ' + uri + u' (или соответствующее приложение на устройстве)'

        try:
            if uri.find('translate.ya') != -1:
                query = unquote(re.search('text=[^&]*', uri).group(0).replace('text=', ''))
                return u'Открывается перевод (' + uri.split('lang=')[-1] + u') текста <' + query + u'>'

            # sub_name == "personal_assistant.scenarios.search", "serp_search" и
            # "personal_assistant.scenarios.search__serp" обрабатываем здесь же
            elif uri.endswith('viewport_id=serp') or intent.endswith('search') \
                    or (uri.find('yandex') != -1 and uri.find('search') != -1):
                query = unquote(re.search('text=[^&]*', uri).group(0).replace('text=', ''))
                return u'Открывается поиск по запросу <' + query + u'>'

        except AttributeError:
            return u'Открывается указанное приложение/сайт'

        if intent.endswith('map'):
            return u'Открываются Яндекс.Карты'

        elif intent.find('weather') != -1:
            return u'Открывается Яндекс.Погода'

        elif uri.startswith('http:') and len(uri) < 35:
            return u'Открывается веб-страница ' + (uri[7:-1] if uri.endswith('/') else uri[7:])

        elif uri.startswith('https:') and len(uri) < 35:
            return u'Открывается веб-страница ' + (uri[8:-1] if uri.endswith('/') else uri[8:])

        elif intent.endswith('open_site_or_app'):
            return u'Открывается указанное приложение/сайт'

        elif intent.endswith('call'):
            return u'Совершается вызов по указанному номеру'

        return u'Открывается указанное приложение/сайт'

    if check_sub_name("start_bluetooth", "bluetooth_start"):
        return u'Включается bluetooth'

    if check_sub_name("stop_bluetooth", "bluetooth_stop"):
        return u'Выключается bluetooth'

    if check_sub_name("player_pause", "voiceprint_player_pause"):
        return u'Приостанавливается запись голосового отпечатка'

    if check_sub_name("open_soft", "navigation_open_app"):
        if directives_dict["open_soft"][0].get("uri"):
            uri = directives_dict["open_soft"][0]["uri"]
            if "utm_referrer" in uri:
                uri = unquote(re.search(".+utm_referrer", uri).group(0)[:-12])
            if "?&" in uri:
                uri.replace("?&", "?")
            uri = uri.rstrip("&").rstrip("?").rstrip("/")
            return u'Открывается сайт ' + uri
        return u'Открывается указанное приложение'

    if check_sub_name("sound_louder", "sound_louder"):
        return u'Увеличивается громкость звука'

    if check_sub_name("sound_quiter", "sound_quiter"):
        return u'Уменьшается громкость звука'

    if check_sub_name("sound_mute", "sound_mute"):
        return u'Включается беззвучный режим'

    if check_sub_name("sound_unmute", "sound_unmute"):
        return u'Выключается беззвучный режим'

    if check_sub_name("sound_set_level", "sound_unmute"):
        if directives_dict["sound_set_level"][0].get("new_level"):
            return u'Устанавливается уровень звука, равный ' + str(directives_dict["sound_set_level"][0]["new_level"])

    if check_sub_name("music_play", "music_yaradio_play"):
        return u'Воспроизводится музыка в Яндекс.Радио'

    if check_sub_name("music_play", "music_yamusic_play"):
        return u'Воспроизводится музыка в Яндекс.Музыке'

    if check_sub_name("music_play", "music_quasar_play"):
        if directives_dict['music_play'][0]['first_track_id']:
            return u'Включается https://music.yandex.ru/track/' + directives_dict['music_play'][0]['first_track_id']
        elif directives_dict['music_play'][0]['uri']:
            return u'Включается ' + directives_dict['music_play'][0]['uri']
        return u'Воспроизводится музыка в Яндекс.Станции'

    if check_sub_name("music_play", "music_snippet_play"):
        return u'Включается указанная музыка'

    if check_sub_name("start_image_recognizer", "navigation_open_camera") or \
            check_sub_name("start_image_recognizer", "computer_vision_camera"):
        return u'Включается камера'

    if check_sub_name("start_music_recognizer", "music_start_recognizer"):
        return u'Включается микрофон для распознавания музыки'

    if check_sub_name("set_timer", "set_timer"):
        if directives_dict["set_timer"][0].get("duration"):
            return u'Устанавливается таймер на ' + str(directives_dict["set_timer"][0]["duration"]) + "секунд"
        return u'Устанавливается таймер'

    if check_sub_name("show_timers", "show_timers"):
        return u'Отображаются таймеры'

    if check_sub_name("cancel_timer", "timer_cancel"):
        return u'Останавливается выбранный таймер'

    if check_sub_name("pause_timer", "timer_pause"):
        return u'Выбранный таймер ставится на паузу'

    if check_sub_name("resume_timer", "timer_resume"):
        return u'Выбранный таймер снимается с паузы'

    if check_sub_name("timer_stop_playing", "timer_stop_playing"):
        return u'Выключается звук срабатывания таймера'

    if check_sub_name("player_pause", "external_skill_player_pause"):
        return u'Проигрыватель во внешних навыках ставится на паузу'

    if check_sub_name("close_dialog", "external_skill_close_dialog"):
        return u'Закрывается диалог пользователя с внешним навыком'

    if check_sub_name("end_dialog_session", "external_skill_end_dialog_session"):
        return u'Завершается диалог пользователя с внешним навыком'

    if check_sub_name("player_pause", "general_conversation_player_pause") or \
            check_sub_name("player_pause", "player_pause"):
        return u'Проигрыватель ставится на паузу'

    if check_sub_name("show_alarms", "show_alarms"):
        return u'Показывается список будильников'

    if check_sub_name("alarm_new", "alarm_new"):
        return u'Устанавливается будильник на указанное время'

    if check_sub_name("alarms_update", "alarms_update"):
        all_alarms = re.findall("DTSTART:\d+T\d+Z", directives_dict["alarms_update"][0]["state"])
        if all_alarms:
            if len(all_alarms) == 1:
                return u'Установлен будильник на ' + \
                       (datetime.strptime(all_alarms[0][8:], "%Y%m%dT%H%M%SZ") +
                        timedelta(seconds=10800)).strftime("%Y-%m-%d %H:%M:%S")
            return u'Установлены будильники на : ' + \
                   u', '.join([(datetime.strptime(alarm[8:], "%Y%m%dT%H%M%SZ") +
                                timedelta(seconds=10800)).strftime("%Y-%m-%d %H:%M:%S") for alarm in all_alarms])
        return u'Обновляются параметры текущих настроек будильника'

    if check_sub_name("alarm_stop", "alarm_stop") or check_sub_name("alarm_stop", "alarm_stop_on_quasar") or \
            check_sub_name("alarms_update", "alarm_stop"):
        return u'Выключается выбранный будильник'

    if check_sub_name("alarms_update", "alarm_one_cancel"):
        return u'Отключается выбранный будильник'

    if check_sub_name("alarms_update", "alarm_many_cancel"):
        return u'Отключаются несколько выбранных будильников'

    if check_sub_name("alarms_update", "alarm_all_cancel"):
        return u'Выключаются все будильники'

    if check_sub_name("radio_play", "radio_play"):
        return u'Включается радио "' + directives_dict["radio_play"][0]["title"] + u'"'

    if check_sub_name("player_continue", "music_player_continue"):
        return u'Продолжается воспроизведение музыки после паузы'

    if check_sub_name("player_continue", "video_player_continue"):
        return u'Продолжается воспроизведение видео после паузы'

    if check_sub_name("player_rewind", "player_rewind"):
        return u'Осуществляется перемотка'

    if check_sub_name("player_rewind", "video_player_rewind"):
        return u'Осуществляется перемотка видео'

    if check_sub_name("find_contacts", "phone_find_contacts"):
        return u'Производится поиск контактов на устройстве'

    if check_sub_name("show_tv_gallery", "tv_show_gallery"):
        return u'Показывается галерея телеканалов'

    if check_sub_name("show_gallery", "video_show_gallery"):
        return u'Показывается галерея видео'

    if check_sub_name("show_season_gallery", "video_show_season_gallery"):
        return u'Показывается галерея сезона сериала'

    if check_sub_name("send_bug_report", "send_bug_report"):
        return u'Отправляется баг репорт'

    if check_sub_name("show_description", "video_show_description"):
        return u'Показывается описание видео (фильма, сериала и проч.)'

    if check_sub_name("show_pay_push_screen", "video_show_pay_push_screen"):
        return u'Показывается экран оплаты'

    if check_sub_name("debug_info", "video_show_debug_info"):
        return u'Добавляется debug информация в видео.'

    if check_sub_name("go_backward", "video_payment_rejected_and_go_backward"):
        return u'Открывается предыдущий экран после отказа покупки'

    if check_sub_name("yandexnavi", "yandexnavi_map_search"):
        if directives_dict["yandexnavi"][0].get('params', {}).get('text'):
            return u'Открывается поиск на карте по запросу "' + directives_dict["yandexnavi"][0]["params"]['text'] + u'"'
        return u'Открывается поиск на карте по указанному запросу'

    if check_sub_name("yandexnavi", "yandexnavi_build_route_on_map"):
        return u'Происходит построение маршрута на карте'

    if check_sub_name("yandexnavi", "yandexnavi_external_confirmation"):
        if directives_dict["yandexnavi"][0].get('params', {}).get('confirmed'):
            if directives_dict["yandexnavi"][0]["params"]['confirmed'] == "1":
                return u'Происходит подтверждение предыдущего действия'  # в ответ на "давай", "поехали", "да" и проч.
            if directives_dict["yandexnavi"][0]["params"]['confirmed'] == "0":
                return u'Происходит отмена предыдущего действия'  # в ответ на "нет", "отмена", "ой" и проч.
        return u'Происходит подтверждение или отмена предыдущего действия'

    if check_sub_name("yandexnavi", "yandexnavi_show_user_position"):
        return u'Показывается месторасположение пользователя'

    if check_sub_name("car", "car_launch"):
        if directives_dict["car"][0].get('params', {}):
            if directives_dict["car"][0]["params"].get('app') == 'yandexradio':
                return u'Открывается Яндекс.Радио'
            if directives_dict["car"][0]["params"].get('app') == 'navi':
                return u'Открывается Яндекс.Навигатор'
            if directives_dict["car"][0]["params"].get('app') == 'weather':
                return u'Открывается Яндекс.Погода'
            if directives_dict["car"][0]["params"].get('widget') == 'radio':
                return u'Включается радио'
        return u'Открывается указанное приложение'

    if check_sub_name("car", "car_volume_down"):
        return u'Уменьшается уровень громкости'

    if check_sub_name("car", "car_volume_up"):
        return u'Увеличивается уровень громкости'

    if check_sub_name("car", "car_media_control"):
        if directives_dict["car"][0].get('params', {}).get('action') == "pause":
            return u'Вопроизведение ставится на паузу в мультимедийной системе Яндекс.Авто'
        if directives_dict["car"][0].get('params', {}).get('action') == "play":
            return u'Вопроизведение включается в мультимедийной системе Яндекс.Авто'
        if directives_dict["car"][0].get('params', {}).get('action') == "next":
            return u'Включается следующий трек в мультимедийной системе Яндекс.Авто'
        if directives_dict["car"][0].get('params', {}).get('action') == "prev":
            return u'Включается предыдущий трек в мультимедийной системе Яндекс.Авто'
        return u'Производится управление воспроизведением в мультимедийной системе Яндекс.Авто'

    if 'open_bot' in directives_dict:
        return u'Запускается указанный навык (в новом диалоговом окне)'

    if 'open_dialog' in directives_dict:
        return u'Запускается указанный навык (в новом диалоговом окне)'

    if 'update_dialog_info' in directives_dict:
        return u'Открывается другое диалоговое окно'

    if intent.endswith('search_local'):
        return u'Выполняется поиск на компьютере'

    if intent.endswith('restart_pc'):
        return u'Перезагружается компьютер'

    if intent.endswith('power_off'):
        return u'Выключается компьютер'

    if intent.endswith('open_ya_browser'):
        return u'Открывается Яндекс.Браузер'

    if intent.endswith('open_start'):
        return u'Открывается меню Пуск'

    if intent.endswith('open_settings'):
        return u'Открывается панель управление (настройки) компьютера'

    if intent.endswith('open_folder'):
        return u'Открывается папка на компьютере'

    if intent.endswith('open_flash_card'):
        return u'Открывается подключенная флеш карта'

    if intent.endswith('open_disk'):
        return u'Открывается диск на компьютере'

    if intent.endswith('open_file'):
        return u'Открывается файл на компьютере'

    if intent.endswith('open_default_browser'):
        return u'Открывается браузер'

    if intent.endswith('hibernate'):
        return u'Включается спящий режим на компьютере'

    if intent.endswith('restore_tab'):
        return u'Открывается последняя закрытая вкладка в браузере'

    if intent.endswith('close_browser'):
        return u'Закрывается браузер'

    if intent.endswith('open_history'):
        return u'Открывается история в браузере'

    if intent.endswith('open_bookmarks_manager'):
        return u'Открывается/закрывается панель закладок'

    if intent.endswith('open_incognito_mode'):
        return u'В браузере открывается вкладка в режиме инкогнито'

    if intent.endswith('go_home'):
        return u'В браузере открывается домашняя страница'

    if intent.endswith('new_tab'):
        return u'В браузере открывается новая вкладка'

    if intent.endswith('clear_history'):
        return u'Очищается история браузера'

    if intent.endswith('song') or intent.endswith('song\tnext'):
        return u'Алиса включает песню собственного исполнения'

    return u'Ответ показан на экране'


def make_tasks(urls, index, hashsums, language):
    url_index = dict((item["initialFileName"], item["downloadUrl"]) for item in urls)
    tasks = []
    for item in index:
        tasks.append({
            "hashsum": hashsums[item["key"] + ".png"],
            "url": url_index['screenshots/' + item["key"] + '.png'],
            "action": get_action(item["intent"], item["directives_dict"], language),
            "intent": item["intent"],
            "info": item["info"]
        })
    return tasks


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-u', '--urls', dest='urls', help='MDS upload results')
    parser.add_argument('-i', '--index', dest='index', help='tasks index')
    parser.add_argument('-s', '--hashsums', dest='hashsums', help='images hashsums')
    parser.add_argument('-o', '--output', dest='output', help='file with tasks for toloka')
    parser.add_argument('-lang', '--language', dest='language', help='language (ru|tr)', default='ru')
    context = parser.parse_args(sys.argv[1:])

    urls = json.loads(open(context.urls).read())
    index = json.loads(open(context.index).read())
    hashsums = json.loads(open(context.hashsums).read())

    tasks = make_tasks(urls, index, hashsums, context.language)
    with codecs.open(context.output, 'w', encoding='utf8') as g:
        g.write(json.dumps(tasks, ensure_ascii=False, indent=4))


if __name__ == '__main__':
    main()
