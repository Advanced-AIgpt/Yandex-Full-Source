# coding: utf-8
session_schema = {
    'type': 'object',
    'properties': {
        'new': {
            'type': 'boolean',
        },
        'message_id': {
            'type': 'integer',
        },
        'session_id': {
            'type': 'string',
            'maxLength': 64,
        },
        'user_id': {
            'type': 'string',
            'maxLength': 64,
        },
        'skill_id': {
            'type': 'string',
            'maxLength': 255,
        },
    },
    'required': [
        'new',
        'message_id',
        'session_id',
        'user_id',
        'skill_id',
    ],
    'additionalProperties': True,
}

meta_schema = {
    'type': 'object',
    'properties': {
        'locale': {
            'type': 'string',
            'maxLength': 64,
            'pattern': r'^\w+(-\w+)?$',
        },
        'timezone': {
            'type': 'string',
            'maxLength': 64,
        },
        'client_id': {
            'type': 'string',
            'maxLength': 1024,
        },
    },
    'required': [
        'locale',
        'timezone',
        'client_id',
    ],
    'additionalProperties': True,
}

request_schema = {
    'type': 'object',
    'properties': {
        'type': {
            'type': 'string',
            'enum': ['SimpleUtterance', 'ButtonPressed']
        },
        'command': {
            'type': 'string',
            'maxLength': 1024,
        },
        'original_utterance': {
            'type': 'string',
            'maxLength': 1024,
        },
        'payload': {
            'type': 'object'
        },
    },
    'required': [
        'type',
        'command',
        'original_utterance',
    ],
    'additionalProperties': True,
}


external_skill_schema = {
    '$schema': 'http://json-schema.org/draft-04/schema#',
    'type': 'object',
    'properties': {
        'version': {
            'type': 'string',
            'enum': ['1.0'],
        },
        'session': session_schema,
        'meta': meta_schema,
        'request': request_schema,
    },
    'required': [
        'version',
        'session',
        'meta',
        'request',
    ],
    'additionalProperties': True,
}
