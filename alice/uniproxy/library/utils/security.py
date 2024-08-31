import copy


def _hide_cgi_secret(url, secret_key):
    secret_head = secret_key + '='
    secret_head_pos = url.find(secret_head)
    if secret_head_pos != -1:
        secret_pos = secret_head_pos + len(secret_head)
        secret_len = url[secret_pos:].find('&')
        if secret_len == -1:
            secret_len = len(url) - secret_pos

        url = url[:secret_pos] + 'X' * secret_len + url[secret_pos + secret_len:]

    return url


def hide_cgi_secrets(url):
    secret_keys = ['token', 'sessionid']
    for secret_key in secret_keys:
        url = _hide_cgi_secret(url, secret_key)
    return url


def _hide_dict_tokens_impl(name, data):
    if isinstance(data, str) and ('token' in name):
        return 'X' * len(data)

    if isinstance(data, dict):
        for key, value in data.items():
            data[key] = _hide_dict_tokens_impl(key, value)

    return data


def hide_dict_tokens(data):
    return _hide_dict_tokens_impl('', copy.deepcopy(data))
