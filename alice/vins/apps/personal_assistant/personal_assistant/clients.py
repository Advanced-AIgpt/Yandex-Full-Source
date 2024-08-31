def is_stroka(app_info):
    app_id = app_info.app_id or ''
    return app_id.startswith('winsearchbar')


def is_yabro_windows(app_info):
    app_id = app_info.app_id or ''
    platform = app_info.platform or ''
    return app_id.lower().startswith('yabro') and platform.lower().startswith('windows')


def is_yabro_desktop(app_info):
    app_id = app_info.app_id or ''
    return app_id.lower().startswith('yabro')


def is_searchapp_android(app_info):
    app_id = app_info.app_id or ''
    return (
        app_id.lower().startswith('ru.yandex.searchplugin') or
        app_id.lower().startswith('ru.yandex.weatherplugin')
    )


def is_searchapp_ios(app_info):
    app_id = app_info.app_id.lower() or ''
    return (
        app_id == 'ru.yandex.mobile' or
        app_id == 'ru.yandex.mobile.inhouse' or
        app_id == 'ru.yandex.mobile.dev'
    )


def is_searchapp(app_info):
    return is_searchapp_android(app_info) or is_searchapp_ios(app_info)


def is_alicekit_android(app_info):
    app_id = app_info.app_id or ''
    return app_id.lower().startswith('com.yandex.alicekit')


def is_alicekit_ios(app_info):
    app_id = app_info.app_id or ''
    return app_id.lower().startswith('ru.yandex.mobile.alice')


def is_alicekit(app_info):
    return is_alicekit_android(app_info) or is_alicekit_ios(app_info)


def is_webtouch(app_info):
    app_id = app_info.app_id or ''
    return app_id.lower().startswith('ru.yandex.webtouch')


def is_yabro_mobile_android(app_info):
    app_id = app_info.app_id or ''
    return app_id.lower().startswith('com.yandex.browser')


def is_yabro_mobile_ios(app_info):
    app_id = app_info.app_id or ''
    return app_id.lower().startswith('ru.yandex.mobile.search')


def is_telegram(app_info):
    app_id = app_info.app_id or ''
    return app_id.startswith('telegram')


def is_quasar(app_info):
    app_id = app_info.app_id or ''
    return app_id.startswith('ru.yandex.quasar')


def is_mini_speaker(app_info):
    app_id = app_info.app_id or ''
    return app_id.startswith('aliced')


def is_legatus(app_info):
    app_id = app_info.app_id or ''
    return app_id.startswith('legatus')


def is_mini_speaker_dexp(app_info):
    manufacturer = app_info.device_manufacturer or ''
    return is_mini_speaker(app_info) and manufacturer.lower() == 'dexp'


def is_mini_speaker_lg(app_info):
    manufacturer = app_info.device_manufacturer or ''
    return is_mini_speaker(app_info) and manufacturer.lower() == 'lg'


def is_mini_speaker_elari(app_info):
    manufacturer = app_info.device_manufacturer or ''
    return is_mini_speaker(app_info) and manufacturer.lower() == 'elari'


def has_uncontrollable_updates(app_info):
    return is_mini_speaker_lg(app_info) or is_mini_speaker_elari(app_info)


def is_smart_speaker(app_info):
    return is_quasar(app_info) or is_mini_speaker(app_info)


def is_smart_speaker_without_screen(req_info):
    if not is_smart_speaker(req_info.app_info):
        return False
    device_state = req_info.device_state or {}
    return not device_state.get('is_tv_plugged_in', False)


def is_ya_music_app(app_info):
    app_id = app_info.app_id or ''
    return (
        app_id.startswith('ru.yandex.mobile.music') or
        app_id.startswith('ru.yandex.music') or
        app_id.startswith('ns.mobile.music')
    )


def is_elari_watch(app_info):
    app_id = app_info.app_id or ''
    return app_id.startswith('ru.yandex.iosdk.elariwatch')


def is_sdg(app_info):
    app_id = app_info.app_id or ''
    return app_id.startswith('ru.yandex.sdg')


def is_desktop(app_info):
    return is_stroka(app_info) or is_telegram(app_info)


def is_android(app_info):
    return app_info.platform == 'android'


def is_ios(app_info):
    return app_info.platform == 'iphone'


def is_navigator(app_info):
    app_id = app_info.app_id or ''
    return (
        app_id.startswith('ru.yandex.mobile.navigator') or
        app_id.startswith('ru.yandex.yandexnavi')
    )


def is_auto(app_info):
    app_id = app_info.app_id or ''
    return app_id.startswith('yandex.auto')


def is_client_with_navigator(req_info):
    app_info = req_info.app_info or {}
    supported_features = req_info.additional_options.get('supported_features') or []
    return is_navigator(app_info) or is_auto(app_info) or 'navigator' in supported_features


def is_auto_without_suggests(app_info):
    app_version = app_info.app_version or ''
    return is_auto(app_info) and app_version != '' and app_version < '1.5'


# TODO(autoapp): remove this method when autoapp's dead
def is_autoapp(app_info):
    app_id = app_info.app_id or ''
    return app_id.startswith('yandex.auto.old')


def is_yandex_drive(req_info):
    device_state = req_info.device_state or {}
    car_options = device_state.get('car_options', {})
    car_type = car_options.get('type', '')
    return is_auto(req_info.app_info) and car_type.startswith('carsharing')


def is_auto_kaptur(req_info):
    device_state = req_info.device_state or {}
    car_options = device_state.get('car_options', {})
    car_type = car_options.get('model', '')
    return is_auto(req_info.app_info) and car_type.startswith('captur')  # Yes, it is Captur. Typos are fun!


def is_tv_device(app_info):
    app_id = app_info.app_id or ''
    return app_id.startswith('com.yandex.tv.alice')


def supports_buttons(app_info):
    if is_navigator(app_info):
        return False
    return True


def has_alicesdk_player(req_info):
    app_info = req_info.app_info or {}
    supported_features = req_info.additional_options.get('supported_features') or []

    if 'music_sdk_client' in supported_features:
        return True

    if req_info.experiments['internal_music_player'] is not None:
        return (
            is_searchapp(app_info) or
            is_yabro_mobile_android(app_info) or
            is_navigator(app_info) or
            is_alicekit(app_info)
        )

    return False


def has_player(req_info):
    app_info = req_info.app_info or {}
    supported_features = req_info.additional_options.get('supported_features') or []

    return (
        is_smart_speaker(app_info) or is_auto(app_info) or is_ya_music_app(app_info) or
        has_alicesdk_player(req_info) or 'music_quasar_client' in supported_features
    )


def has_led_display(req_info):
    supported_features = req_info.additional_options.get('supported_features') or []
    return 'led_display' in supported_features


def has_mordovia_webview(req_info):
    supported_features = req_info.additional_options.get('supported_features') or []
    return is_quasar(req_info.app_info) or 'mordovia_webview' in supported_features


def supports_vertical_screen_navigation(req_info):
    supported_features = req_info.additional_options.get('supported_features') or []
    return 'vertical_screen_navigation' in supported_features
