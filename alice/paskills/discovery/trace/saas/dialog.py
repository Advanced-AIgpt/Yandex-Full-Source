import requests


def get_skill_info(skill_id):
    url = 'https://paskills.voicetech.yandex.net/api/external/v1'
    headers = {'Content-Type': 'application/json'}
    data = '{"method": "getSkill", "jsonrpc": "2.0", "id": 111, "params": ["%s"]}' % skill_id
    req = requests.post(
        url,
        headers=headers,
        data=data,
    )
    info = req.json().get('result', {})
    return {
        'id': info.get('id'),
        'name': info.get('name'),
        'onAir': info.get('onAir'),
        'isRecommended': info.get('isRecommended'),
    }


if __name__ == "__main__":
    print(get_skill_info('8232d1d7-bb06-452e-b91d-3dca8aa854a6'))
