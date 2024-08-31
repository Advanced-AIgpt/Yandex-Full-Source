# coding: utf-8

# support several variant of UUID format:
# 123e4567-e89b-12d3-a456-426655440000
# 123E4567-E89B-12D3-A456-426655440000
# 123e4567e89b12d3a456426655440000
# {123e4567-e89b-12d3-a456-426655440000}
# {123e4567e89b12d3a456426655440000}
UUID_PATTERN = r'^([0-9a-fA-F]{8}\-?([0-9a-fA-F]{4}\-?){3}[0-9a-fA-F]{12})|(\{[0-9a-fA-F]{8}\-?([0-9a-fA-F]{4}\-?){3}[0-9a-fA-F]{12}\})$'  # noqa


header_schema = {
    'type': 'object',
    'properties': {
        'request_id': {
            'type': 'string'
        },
        'sequence_number': {
            'type': ['integer', 'null'],
            'minimum': 0
        },
    },
    'required': ['request_id'],

    'dependencies': {
        'sequence_number': {
            'properties': {
                'prev_req_id': {
                    'type': ['string', 'null'],
                },
            },
            'required': [
                'prev_req_id',
                'sequence_number',
            ],
        }
    }
}


application_schemas = {
    'type': 'object',
    'properties': {
        'app_id': {
            'type': 'string'
        },
        'app_version': {
            'type': 'string'
        },
        'os_version': {
            'type': 'string'
        },
        'platform': {
            'type': 'string',
        },
        'uuid': {
            'type': 'string',
            'pattern': UUID_PATTERN,
        },
        'device_id': {
            'type': 'string'
        },
        'lang': {
            'type': 'string'
        },
        'client_time': {
            'type': 'string'
        },
        'timezone': {
            'type': 'string'
        },
        'timestamp': {
            'type': 'string',
            # TODO: don't forget to change this regexp in 2334 year
            'pattern': r'^\d{10}$',
        },
        'device_manufacturer': {
            'type': 'string'
        },
        'device_model': {
            'type': 'string'
        },
    },
    'required': [
        'app_id',
        'app_version',
        'os_version',
        'platform',
        'uuid',
        'lang',
        'client_time',
        'timezone',
        'timestamp'
    ]
}

text_input_event_schema = {
    'type': 'object',
    'properties': {
        'type': {
            'type': 'string',
            'enum': [
                'text_input',
                'suggested_input',
            ]
        },
        'text': {
            'type': 'string'
        }
    },
    'required': [
        'type',
        'text'
    ]
}

voice_input_event_schema = {
    'type': 'object',
    'properties': {
        'type': {
            'type': 'string',
            'enum': [
                'voice_input'
            ]
        },
        'asr_result': {
            'type': 'array',
            'minItems': 1,
            'items': {
                'type': 'object',
                'properties': {
                    'utterance': {
                        'type': 'string'
                    },
                    'confidence': {
                        'type': 'number',
                    },
                    'words': {
                        'type': 'array',
                        'items': {
                            'type': 'object',
                            'properties': {
                                'confidence': {'type': 'number'},
                                'value': {'type': 'string'},
                            },
                            'required': [
                                'value',
                                'confidence',
                            ],
                        },
                    },
                },
                'required': [
                    'utterance',
                    'confidence',
                ]
            }
        },
        'biometry_scoring': {
            'type': 'object',
            'properties': {
                'scores': {
                    'type': 'array',
                    'minItems': 0,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'user_id': {
                                'type': 'string',
                            },
                            'score': {
                                'type': 'number',
                            },
                        },
                        'required': [
                            'user_id',
                            'score',
                        ]
                    }
                },
                'scores_with_mode': {
                    'type': 'array',
                    'minItems': 0,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'mode': {
                                'type': 'string',
                            },
                            'scores': {
                                'type': 'array',
                                'minItems': 0,
                                'items': {
                                    'type': 'object',
                                    'properties': {
                                        'user_id': {
                                            'type': 'string',
                                        },
                                        'score': {
                                            'type': 'number',
                                        },
                                    },
                                    'required': [
                                        'user_id',
                                        'score',
                                    ]
                                }
                            },
                        },
                        'required': [
                            'mode',
                            'scores',
                        ]
                    }
                },
                'status': {
                    'type': 'string',
                },
                'request_id': {
                    'type': 'string',
                },
                'group_id': {
                    'type': 'string',
                },
            },
            'required': [
                'status'
            ]
        },
        'end_of_utterance': {
            'type': 'boolean',
        },
        'hypothesis_number': {
            'type': 'integer',
            'minimum': 0,
        },
    },
    'required': [
        'type',
        'asr_result'
    ]
}

server_action_event_schema = {
    'type': 'object',
    'properties': {
        'type': {
            'type': 'string',
            'enum': [
                'server_action'
            ]
        },
        'name': {
            'type': 'string'
        },
        'payload': {
            'type': 'object',
            'properties': {}
        },
        'ignore_answer': {
            'type': 'boolean',
        },
    },
    'required': [
        'type',
        'name',
    ]
}

music_input_event_schema = {
    'type': 'object',
    'properties': {
        'type': {
            'type': 'string',
            'enum': [
                'music_input',
            ]
        },
        'music_result': {
            'type': 'object',
            'properties': {
                'data': {
                    'type': 'object',
                    'engine': {
                        'type': 'string',
                    },
                    'match': {
                        'type': 'object',
                    },
                    'recognition-id': {
                        'type': 'string',
                    },
                    'url': {
                        'type': 'string',
                    },
                },
                'result': {
                    'type': 'string',
                },
                'error_text': {
                    'type': 'string',
                }
            }
        }
    },
    'required': [
        'type',
        'music_result',
    ]
}

image_input_event_schema = {
    'type': 'object',
    'properties': {
        'type': {
            'type': 'string',
            'enum': [
                'image_input'
            ]
        },
        'payload': {
            'type': 'object',
            'properties': {
                'img_url': {
                    'type': 'string',
                },
            }
        }
    },
    'required': [
        'type',
        'payload',
    ]
}

request_schema = {
    'type': 'object',
    'properties': {
        'header': header_schema,
        'application': application_schemas,
        'request': {
            'type': 'object',
            'properties': {
                'event': {
                    'oneOf': [
                        text_input_event_schema,
                        voice_input_event_schema,
                        server_action_event_schema,
                        music_input_event_schema,
                        image_input_event_schema,
                    ]
                },
                'voice_session': {'type': 'boolean'},
                'location': {
                    'type': 'object',
                    'properties': {
                        'accuracy': {'type': 'number'},
                        'recency': {'type': 'integer'},
                        'lat': {'type': 'number'},
                        'lon': {'type': 'number'},
                    },
                    'required': [
                        'lat',
                        'lon',
                    ],
                },
                'additional_options': {
                    'type': 'object',
                    'properties': {
                        'supported_features': {
                            'type': 'array',
                            'items': {'type': 'string'}
                        },
                        'unsupported_features': {
                            'type': 'array',
                            'items': {'type': 'string'}
                        },
                        'oauth_token': {'type': 'string'},
                        'bass_options': {'type': 'object'},
                    },
                },
                'laas_region': {
                    'properties': {
                        'city_id': {
                            'type': 'integer'
                        },
                        'is_user_choice': {
                            'type': 'boolean'
                        },
                        'latitude': {
                            'type': 'number'
                        },
                        'location_accuracy': {
                            'type': 'integer'
                        },
                        'location_unixtime': {
                            'type': 'integer'
                        },
                        'longitude': {
                            'type': 'number'
                        },
                        'precision': {
                            'type': 'integer'
                        },
                        'probable_regions': {
                            'items': {
                                'properties': {
                                    'region_id': {
                                        'type': 'integer'
                                    },
                                    'weight': {
                                        'type': 'number'
                                    }
                                },
                                'type': 'object'
                            },
                            'type': 'array'
                        },
                        'probable_regions_reliability': {
                            'type': 'number'
                        },
                        'region_by_ip': {
                            'type': 'integer'
                        },
                        'region_id': {
                            'type': 'integer'
                        },
                        'should_update_cookie': {
                            'type': 'boolean'
                        },
                        'suspected_latitude': {
                            'type': 'number'
                        },
                        'suspected_location_accuracy': {
                            'type': 'integer'
                        },
                        'suspected_location_unixtime': {
                            'type': 'integer'
                        },
                        'suspected_longitude': {
                            'type': 'number'
                        },
                        'suspected_precision': {
                            'type': 'integer'
                        },
                        'suspected_region_city': {
                            'type': 'integer'
                        },
                        'suspected_region_id': {
                            'type': 'integer'
                        }
                    },
                    'type': 'object'
                },
                'features': {
                    'type': 'object'
                },
                'ensure_purity': {'type': 'boolean'},
            },
            'required': [
                'event'
            ]
        },
        'session': {
            'type': ['string', 'null']
        }
    },
    'required': [
        'header',
        'application',
        'request'
    ]
}
