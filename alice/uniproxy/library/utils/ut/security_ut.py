from alice.uniproxy.library.utils.security import hide_cgi_secrets, hide_dict_tokens


def test_hide_cgi_secrets1():
    unsafe_url = '/blackbox?method=oauth&format=json&oauth_token=vDhYw&userip=128.72.10.81&aliases=13'
    secure_url = '/blackbox?method=oauth&format=json&oauth_token=XXXXX&userip=128.72.10.81&aliases=13'
    assert hide_cgi_secrets(unsafe_url) == secure_url


def test_hide_cgi_secrets2():
    unsafe_url = '/blackbox?method=oauth&format=json&get_user_ticket=yes&oauth_token=AgAA&userip=46.39.54.123'
    secure_url = '/blackbox?method=oauth&format=json&get_user_ticket=yes&oauth_token=XXXX&userip=46.39.54.123'
    assert hide_cgi_secrets(unsafe_url) == secure_url


def test_hide_cgi_secrets3():
    url = '/blackbox?method=oauth&format=json&get_user_ticket=yes'
    assert hide_cgi_secrets(url) == url


def test_hide_cgi_secrets4():
    unsafe_url = 'https://ya.ru/api?token=12345&userip=46.39.54.123&sessionid=lol-korvalol'
    secure_url = 'https://ya.ru/api?token=XXXXX&userip=46.39.54.123&sessionid=XXXXXXXXXXXX'
    assert hide_cgi_secrets(unsafe_url) == secure_url


def test_hide_cgi_secrets5():
    unsafe_url = 'https://ya.ru/api?token=12345'
    secure_url = 'https://ya.ru/api?token=XXXXX'
    assert hide_cgi_secrets(unsafe_url) == secure_url


def test_hide_dict_tokens():
    data = {
        'key': 'value',
        'my_token': '!@#$%',
        'subdata': {
            'key': 'value',
            'token_storage': {
                'hi': 'alice',
                'their_token': 'ABC',
                'this_could_be_your_token': 'F' * 101,
            }
        }
    }
    secure_data = {
        'key': 'value',
        'my_token': 'XXXXX',
        'subdata': {
            'key': 'value',
            'token_storage': {
                'hi': 'alice',
                'their_token': 'XXX',
                'this_could_be_your_token': 'X' * 101,
            }
        }
    }
    assert hide_dict_tokens(data) == secure_data
    assert data['my_token'] == '!@#$%'
    assert data['subdata']['token_storage']['their_token'] == 'ABC'
