cdef _hide_half(str value):
    cdef unsigned value_part_size = len(value) // 2
    return value[:value_part_size] + "*" * (len(value) - value_part_size)


# Preventing oauth_tokens from being logged: VOICESERV-310 + VOICESERV-1048
def obscure_oauth_token_and_cookies(d):
    if not d or type(d) is not dict:
        return

    upd = {}
    for key, value in d.items():
        if key in ("oauth_token", "serviceticket") and value and isinstance(value, str):
            upd[key] =  _hide_half(value)
        elif key.lower() in ("cookie", "cookies"):
            upd[key] = "..."
        if type(value) is dict:
            obscure_oauth_token_and_cookies(value)
    d.update(upd)
