#-*- coding: utf-8 -*-

import json
import requests

external_token = 
oauth_token = external_token

testpalm_header = {
    'Authorization': f'OAuth {oauth_token}',
    "Content-Type": "application/json"
}

oauth_header = {'Authorization': f'OAuth {oauth_token}'}

alice_url = 'https://testpalm.yandex-team.ru:443/api/testcases/alice'
quasar_url = 'https://testpalm.yandex-team.ru:443/api/testcases/quasar'
elari_url = 'https://testpalm.yandex-team.ru:443/api/testcases/elari'


def post_case(url, body):
    body = json.dumps(body, ensure_ascii=False)
    body = body.encode('utf-8')
    resp = requests.post(url, data=body, headers=testpalm_header)
    #print(resp.text)
    return resp

def put_case(url, body):
    body = json.dumps(body, ensure_ascii=False)
    body = body.encode('utf-8')
    resp = requests.put(url, data=body, headers=testpalm_header)
    #print(resp.text)
    return resp

def get_case_body(url, case_id):
    resp = requests.get(f'{url}/?id={case_id}', headers=oauth_header)
    body = resp.json()
    return body

#отдает список id-шников кейсов
def get_cases(url):
    case_resp = requests.get(f'{url}/?includeFields=id', headers=testpalm_header)
    if not case_resp.json():
        print('[WARNING] Не достал кейсы') # проверка на пустой массив
        return []
    else:
        return [c['id'] for c in case_resp.json()]

# номера тэгов:

# -----------------
# для всех кейсов: поверхности, линк на изначальный кейс - временный костыль (не можем ставить нормальные линки)
alice_surface = '5baba29ba5e5edef3427a69a' # пример значения: elari, station
alice_link = '5bae53b1a5e5ed4c6bd42388'  # пример значения ['https://testpalm.yandex-team.ru/testcase/elari-1']
# -----------------

# -----------------
# elari <-> 
elari_tags =  '5b587f91e0368c06d57bfab8' # примеры: 18+ и тд -> alise_tags_key 
elari_innerSkillName = '5b5892bbe0368c06d57c85d7' # из вики -> alice_innerSkillName

alise_elari_tags = '5bab7b94a632eaf868c86723'
alice_innerSkillName = '5bab7c3a2afa3a9a9b9478b2'
# -----------------

# -----------------
# quasar <-> alice

quasar_platform = '5ba0c742a5e5ed194f500b24' # есть navi и searchapp -> alice_surface

quasar_tags = '5b311da8643dc77a9d8a9e31' # тэги, start, fairytale и тд -> alice_quasar_tags
quasar_testing = '5b87e0c7ac472fbe910e8ad5'  # это smoke, no_assessors -> alice_testing
quasar_flags = '5ba4f572a632eac1a217001a' # флаги -> alice_flags
quasar_class = '5b5ed63c548922dd9ce46587' # музыка, видео и тд -> alice_quasar_class
quasar_component = '5b8e484bac472fbe9120d422' # винс, басс и тд -> alice_component

alice_quasar_tags = '5bb1136ae0368c41a9fa0328'
alice_testing = '5bb1136a798633db7f1b5dd2'
alice_flags = '5bace2667986336dc2335a2d'
alice_quasar_class = '5bb1136aac472f6a2ce9a841'
alice_component = '5bb1136aa632eadc848adccd'
# -----------------


def elari_to_alice():
    for case_id in get_cases(elari_url):
        case = get_case_body(elari_url, case_id)
        if not case:
            print ('no case with id' + str(case_id))
        else:
            tags = ''
            innerSkillName = ''
            if elari_tags in case[0]['attributes']:
                tags = case[0]['attributes'][elari_tags]
            if elari_innerSkillName in case[0]['attributes']:
                innerSkillName = case[0]['attributes'][elari_innerSkillName]
            case[0]['attributes'] = {}
            if tags:
                case[0]['attributes'][alise_elari_tags] = tags
            if innerSkillName:
                case[0]['attributes'][alice_innerSkillName] = innerSkillName
            case[0]['attributes'][alice_surface] = ['elari']
            link = 'https://testpalm.yandex-team.ru/testcase/elari-'+str(case_id)
            case[0]['attributes'][alice_link] = [link]
            post_case(alice_url, case[0])
            print ('case ' + str(case_id))
    return

def quasar_to_alice():
    new_id = 336
    f = open('alice - quasar.txt', 'a')

    for case_id in get_cases(quasar_url):
        case = get_case_body(quasar_url, case_id)
        if not case:
            print ('no case with id' + str(case_id))
        else:
            new_id += 1
            case[0]['id'] = new_id

            platform = []
            tags = []
            testing = []
            flags = []
            clas = []
            component = []

            if quasar_platform in case[0]['attributes']:
                platform = case[0]['attributes'][quasar_platform]
            else: platform = ['station']

            if quasar_tags in case[0]['attributes']:
                tags = case[0]['attributes'][quasar_tags]

            if quasar_testing in case[0]['attributes']:
                testing = case[0]['attributes'][quasar_testing]

            if quasar_flags in case[0]['attributes']:
                flags = case[0]['attributes'][quasar_flags]

            if quasar_class in case[0]['attributes']:
                clas = case[0]['attributes'][quasar_class]

            if quasar_component in case[0]['attributes']:
                component = case[0]['attributes'][quasar_component]

            case[0]['attributes'] = {}
            if tags:
                case[0]['attributes'][alice_quasar_tags] = tags
            if testing:
                case[0]['attributes'][alice_testing] = testing
            if flags:
                case[0]['attributes'][alice_flags] = flags
            if clas:
                case[0]['attributes'][alice_quasar_class] = clas
            if component:
                case[0]['attributes'][alice_component] = component

            case[0]['attributes'][alice_surface] = platform
            link = 'https://testpalm.yandex-team.ru/testcase/quasar-'+str(case_id)
            case[0]['attributes'][alice_link] = [link]
            post_case(alice_url, case[0])
            #print(post_case(alice_url, case[0]))
            print ('quasar-' + str(case_id) + ' -> alice-' + str(new_id))
            f.write('alice-' + str(new_id) + ' - quasar-' + str(case_id) + '\n')
    f.close()
    return

quasar_to_alice()

