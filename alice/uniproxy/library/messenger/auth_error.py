
# Fanout, NoToken, NoCookie, NoCsrf
MSSNGR_CLIENT_AUTH_ERROR_TEXT = {
    (False, True, False, False): {
        'scope': 'client',
        'code': 'no_auth_data',
        'text': 'expected yamb cookie auth, something went wrong',
    },

    (False, True, False, True): {
        'scope': 'client',
        'code': 'no_auth_data',
        'text': 'expected yamb cookie auth, no csrf found',
    },

    (False, True, True, False): {
        'scope': 'client',
        'code': 'no_auth_data',
        'text': 'expected yamb cookie auth, no cookie found',
    },

    (False, True, True, True): {
        'scope': 'client',
        'code': 'no_auth_data',
        'text': 'yamb auth, neither token nor cookie/csrf found',
    },

    (False, False, True, True): {
        'scope': 'client',
        'code': 'no_auth_data',
        'text': 'expected yamb token auth',
    },

    (False, False, False, True): {
        'scope': 'client',
        'code': 'no_auth_data',
        'text': 'expected yamb token auth, no csrf found',
    },

    (False, False, True, False): {
        'scope': 'client',
        'code': 'no_auth_data',
        'text': 'expected yamb token auth, no cookie found',
    },

    (False, False, False, False): {
        'scope': 'client',
        'code': 'no_auth_data',
        'text': 'expected yamb token auth, cookie and csrf token both are provided',
    },


    (True, True, False, False): {
        'scope': 'client',
        'code': 'no_auth_data',
        'text': 'expected fanout cookie auth, something went wrong',
    },

    (True, True, False, True): {
        'scope': 'client',
        'code': 'no_auth_data',
        'text': 'expected fanout cookie auth, no csrf found, but something went wrong',
    },

    (True, True, True, False): {
        'scope': 'client',
        'code': 'no_auth_data',
        'text': 'fanout cookie auth, no cookie found',
    },

    (True, True, True, True): {
        'scope': 'client',
        'code': 'no_auth_data',
        'text': 'fanout auth, neither token nor cookie/csrf found',
    },

    (True, False, True, True): {
        'scope': 'client',
        'code': 'no_auth_data',
        'text': 'expected fanout token auth, something went wrong',
    },

    (True, False, False, True): {
        'scope': 'client',
        'code': 'no_auth_data',
        'text': 'expeceted fanout token auth, cookie was found too',
    },

    (True, False, True, False): {
        'scope': 'client',
        'code': 'no_auth_data',
        'text': 'expected fanout token auth, no cookie found',
    },

    (True, False, False, False): {
        'scope': 'client',
        'code': 'no_auth_data',
        'text': 'expected fanout token auth ok, cookie and csrf token both are provided',
    },
}
