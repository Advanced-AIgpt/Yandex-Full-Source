# -*- coding: utf-8 -*-
import json
import sys

import werkzeug

# dirty hack to allow unicode exceptions
reload(sys)
sys.setdefaultencoding('utf8')


class WrongGrammar(werkzeug.exceptions.HTTPException):
    code = 400
    description = u'Wrong grammar'

    def __init__(self, description=u''):
        super(WrongGrammar, self).__init__()
        self.description = description


def handle_wrong_grammar(e):
    response = e.get_response()
    # replace the body with JSON
    response.data = json.dumps({
        'code': e.code,
        'name': e.name,
        'description': e.description
    })
    response.content_type = 'application/json'
    return response
