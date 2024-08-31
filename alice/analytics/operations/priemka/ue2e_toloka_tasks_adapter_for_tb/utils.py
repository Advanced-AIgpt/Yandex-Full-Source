import re
import json


APP_NAME_MAPPING = {
    'auto': 'Авто',
    'auto_old': 'Авто',
    'navigator': 'Навигатор',
    'maps': 'Яндекс.Карты',

    'yabro_prod': 'Десктопный браузер',
    'yabro_beta': 'Десктопный браузер',
    'browser_alpha': 'Мобильный браузер',
    'browser_beta': 'Мобильный браузер',
    'browser_prod': 'Мобильный браузер',
    'browser_prod_ios': 'Мобильный браузер',
    'search_app_prod': 'Поисковое приложение Яндекса',
    'search_app_beta': 'Поисковое приложение Яндекса',
    'search_app_ios': 'Поисковое приложение Яндекса',
    'search_app_ipad': 'Поисковое приложение Яндекса',

    'quasar': 'Большая Яндекс.Станция',
    'yandexmax': 'Яндекс.Станция Макс',
    'yandexmini': 'Яндекс.Станция Мини',
    'small_smart_speakers': 'Аудиоколонка с Алисой',

    'tv': 'Телевизор с Алисой',

    'elariwatch': 'Часы Elari',
    'stroka': 'Яндекс.Строка',
}


def has_cyrillic(text):
    if not text:
        return False
    return bool(re.search('[а-яА-Я]', text))


def convert_toloka_task_to_tb_format(item, need_remove_scenario=False, need_append_app=False, override_app=""):
    if 'inputValues' not in item or 'input' not in item['inputValues']:
        raise Exception('Ошибка, в данных нет полей inputValues.input.XXX: {}'.format(item))

    if isinstance(item['inputValues']['input'], list) and len(item['inputValues']['input']) and (
            'query' in item['inputValues']['input'][0]
            or 'answer' in item['inputValues']['input'][0]
            or 'action' in item['inputValues']['input'][0]
            or 'state' in item['inputValues']['input'][0]
            or 'time' in item['inputValues']['input'][0]
    ):
        # если данные пришли уже в новом формате
        return item

    # массив из запросов в сессии
    toloka_session_tasks_list = []

    # Идем от последнего к первому экшену, что бы соблюсти правильный порядок отображения в файле
    for i in range(15, -1, -1):
        # TODO: поддержать экшны -1, -2, -3 "в будущее"
        action = 'action' + str(i)
        state = 'state' + str(i)

        # Находим ненулевые экшены и работаем с ними
        if not item['inputValues']['input'].get(action):
            continue

        # 1) Формируем взаимодействие
        action_data = item['inputValues']['input'].get(action)

        # язык определяем по наличию русских букв в query, answer, scenario
        texts = json.dumps([action_data.get(x) for x in ['query', 'answer', 'scenario']], ensure_ascii=False)
        is_russian_lang = has_cyrillic(texts)

        if need_remove_scenario and 'scenario' in action_data:
            del action_data['scenario']

        # 2) Служебные поля
        for key in item['inputValues']['input'].keys():
            if 'action' not in key and 'state' not in key and key != 'session':
                action_data[key] = item['inputValues']['input'][key]

        # 3) Состояние девайса
        state_data = {
            'extra': []
        }

        # 3.1) Название девайса
        if need_append_app and is_russian_lang:
            app_to_use = override_app if override_app else item['inputValues']['input'].get('app')
            if app_to_use:
                state_data['extra'].append({
                    'type': 'Устройство с Алисой',
                    'content': APP_NAME_MAPPING.get(app_to_use, app_to_use)
                })

        # Для каждого экшена обрабатываем стейт
        if state in item['inputValues']['input']:
            for st in item['inputValues']['input'][state].get('extra', []):
                if st['type'] in ['Устройства умного дома', 'Smart Home Devices', 'الأجهزة المنزلية الذكية']:
                    state_data['devices'] = st['content']
                    for dvs in st['content']:
                        for dvs_property in dvs:
                            if dvs_property.get('col_content') and dvs_property.get('col_name'):
                                dvs_property['content'] = dvs_property['col_content']
                                del dvs_property['col_content']

                                dvs_property['type'] = dvs_property['col_name']
                                del dvs_property['col_name']

                                if isinstance(dvs_property['content'], list):
                                    dvs_property['content'] = ', '.join(dvs_property['content'])

                elif st.get('playback'):
                    st['content'] = st.get('content', '') + ' (' + st['playback'] + ')'
                    del st['playback']
                    state_data['extra'].append(st)
                elif isinstance(st.get('content'), list):
                    st['content'] = ', '.join(st['content'])
                    state_data['extra'].append(st)
                elif isinstance(st.get('content'), dict):
                    st['content'] = json.dumps(st['content']).replace('}', '').replace('{', '')
                    state_data['extra'].append(st)
                elif st.get('content') is not None and st.get('type') is not None:
                    state_data['extra'].append(st)
                else:
                    print('strange state extra data:', st)

            if item['inputValues']['input'][state].get('screen'):
                state_data['extra'].append({
                    'type': 'Экран' if is_russian_lang else 'Screen',
                    'content': item['inputValues']['input'][state]['screen']
                })

            if item['inputValues']['input'][state].get('volume'):
                state_data['extra'].append({
                    'type': 'Уровень громкости' if is_russian_lang else 'Volume level',
                    'content': str(item['inputValues']['input'][state]['volume'])
                })

            if item['inputValues']['input'][state].get('time'):
                state_data['time'] = item['inputValues']['input'][state]['time']

        # Дальше подкладываем state в текущий action
        action_data['state'] = state_data
        toloka_session_tasks_list.append(action_data)

    item['inputValues']['input'] = toloka_session_tasks_list
    return item
