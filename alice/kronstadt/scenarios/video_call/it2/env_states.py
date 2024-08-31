TEST_DEVICE_ID = 'feedface-e8a2-4439-b2e7-689d95f277b7'

ENV_STATE_WITH_LOGIN = {
    'endpoints': [
        {
            'id': TEST_DEVICE_ID,
            'capabilities': [
                {
                    'meta': {
                        'reportable': False,
                        'retrievable': False,
                        'supported_directives': [
                            'StartVideoCallLoginDirectiveType',
                            'StartVideoCallDirectiveType',
                            'AcceptVideoCallDirectiveType',
                            'DiscardVideoCallDirectiveType'
                        ],
                        'supported_events': []
                    },
                    'state': {
                        'provider_states': [
                            {
                                'telegram_provider_state': {
                                    'login': {
                                        'full_contacts_upload_finished': True,
                                        'state': 'Success',
                                        'user_id': '1111'
                                    }
                                }
                            }
                        ]
                    },
                    '@type': 'type.googleapis.com/NAlice.TVideoCallCapability'
                },
            ]
        },
    ],
}

ENV_STATE_WITHOUT_SUPPORTED_DIRECTIVES_AND_LOGIN = {
    'endpoints': [
        {
            'id': TEST_DEVICE_ID,
            'capabilities': [
                {
                    'meta': {
                        'reportable': False,
                        'retrievable': False,
                        'supported_directives': [],
                        'supported_events': []
                    },
                    'state': {
                        'provider_states': [
                            {
                                'telegram_provider_state': {}
                            }
                        ]
                    },
                    '@type': 'type.googleapis.com/NAlice.TVideoCallCapability'
                },
            ]
        },
    ],
}

ENV_STATE_WITHOUT_SUPPORTED_DIRECTIVES = {
    'endpoints': [
        {
            'id': TEST_DEVICE_ID,
            'capabilities': [
                {
                    'meta': {
                        'reportable': False,
                        'retrievable': False,
                        'supported_directives': [],
                        'supported_events': []
                    },
                    'state': {
                        'provider_states': [
                            {
                                'telegram_provider_state': {
                                    'login': {
                                        'full_contacts_upload_finished': True,
                                        'state': 'Success',
                                        'user_id': '1111'
                                    }
                                }
                            }
                        ],
                        'incoming': [
                            {
                                'state': 'Ringing',
                                'telegram_call_data': {
                                    'call_owner_data': {
                                        'call_id': 'incoming_video_call_id',
                                        'user_id': '1111111111'
                                    },
                                    'recipient': {
                                        'user_id': '2222222222',
                                        'display_name': 'Маша Иванова'
                                    }
                                }
                            }
                        ],
                        'current': {
                            'state': 'Established',
                            'telegram_call_data': {
                                'call_owner_data': {
                                    'call_id': 'outgoing_video_call_id',
                                    'user_id': '1111111111'
                                },
                                'recipient': {
                                    'user_id': '2222222222',
                                    'display_name': 'Маша Иванова'
                                }
                            }
                        }
                    },
                    '@type': 'type.googleapis.com/NAlice.TVideoCallCapability'
                },
            ]
        },
    ],
}

ENV_STATE_WITHOUT_LOGIN = {
    'endpoints': [
        {
            'id': TEST_DEVICE_ID,
            'capabilities': [
                {
                    'meta': {
                        'reportable': False,
                        'retrievable': False,
                        'supported_directives': [
                            'StartVideoCallLoginDirectiveType',
                            'StartVideoCallDirectiveType',
                            'AcceptVideoCallDirectiveType',
                            'DiscardVideoCallDirectiveType'
                        ],
                        'supported_events': []
                    },
                    'state': {},
                    '@type': 'type.googleapis.com/NAlice.TVideoCallCapability'
                },
            ]
        },
    ],
}

ENV_STATE_WITH_INCOMING_CALL = {
    'endpoints': [
        {
            'id': TEST_DEVICE_ID,
            'capabilities': [
                {
                    'meta': {
                        'reportable': False,
                        'retrievable': False,
                        'supported_directives': [
                            'StartVideoCallLoginDirectiveType',
                            'StartVideoCallDirectiveType',
                            'AcceptVideoCallDirectiveType',
                            'DiscardVideoCallDirectiveType'
                        ],
                        'supported_events': []
                    },
                    'state': {
                        'provider_states': [
                            {
                                'telegram_provider_state': {
                                    'login': {
                                        'full_contacts_upload_finished': True,
                                        'state': 'Success',
                                        'user_id': '1111111111'
                                    }
                                }
                            }
                        ],
                        'incoming': [
                            {
                                'state': 'Ringing',
                                'telegram_call_data': {
                                    'call_owner_data': {
                                        'call_id': 'incoming_video_call_id',
                                        'user_id': '1111111111'
                                    },
                                    'recipient': {
                                        'user_id': '2222222222',
                                        'display_name': 'Маша Иванова'
                                    },
                                    'mic_muted': False,
                                    'video_enabled': True
                                }
                            }
                        ]
                    },
                    '@type': 'type.googleapis.com/NAlice.TVideoCallCapability'
                },
            ]
        },
    ],
}


def env_state_with_current_call(mic_muted: False, video_enabled: True):
    return {
        'endpoints': [
            {
                'id': TEST_DEVICE_ID,
                'capabilities': [
                    {
                        'meta': {
                            'reportable': False,
                            'retrievable': False,
                            'supported_directives': [
                                'StartVideoCallLoginDirectiveType',
                                'StartVideoCallDirectiveType',
                                'AcceptVideoCallDirectiveType',
                                'DiscardVideoCallDirectiveType'
                            ],
                            'supported_events': []
                        },
                        'state': {
                            'provider_states': [
                                {
                                    'telegram_provider_state': {
                                        'login': {
                                            'full_contacts_upload_finished': True,
                                            'state': 'Success',
                                            'user_id': '1111111111'
                                        }
                                    }
                                }
                            ],
                            'current': {
                                'state': 'Established',
                                'telegram_call_data': {
                                    'call_owner_data': {
                                        'call_id': 'outgoing_video_call_id',
                                        'user_id': '1111111111'
                                    },
                                    'recipient': {
                                        'user_id': '2222222222',
                                        'display_name': 'Маша Иванова'
                                    },
                                    'mic_muted': mic_muted,
                                    'video_enabled': video_enabled
                                }
                            }
                        },
                        '@type': 'type.googleapis.com/NAlice.TVideoCallCapability'
                    },
                ]
            },
        ],
    }


def env_state_with_outgoing_call(mic_muted: False, video_enabled: True):
    return {
        'endpoints': [
            {
                'id': TEST_DEVICE_ID,
                'capabilities': [
                    {
                        'meta': {
                            'reportable': False,
                            'retrievable': False,
                            'supported_directives': [
                                'StartVideoCallLoginDirectiveType',
                                'StartVideoCallDirectiveType',
                                'AcceptVideoCallDirectiveType',
                                'DiscardVideoCallDirectiveType'
                            ],
                            'supported_events': []
                        },
                        'state': {
                            'provider_states': [
                                {
                                    'telegram_provider_state': {
                                        'login': {
                                            'full_contacts_upload_finished': True,
                                            'state': 'Success',
                                            'user_id': '1111111111'
                                        }
                                    }
                                }
                            ],
                            'outgoing': {
                                'state': 'Established',
                                'telegram_call_data': {
                                    'call_owner_data': {
                                        'call_id': 'outgoing_video_call_id',
                                        'user_id': '1111111111'
                                    },
                                    'recipient': {
                                        'user_id': '2222222222',
                                        'display_name': 'Маша Иванова'
                                    },
                                    'mic_muted': mic_muted,
                                    'video_enabled': video_enabled
                                }
                            }
                        },
                        '@type': 'type.googleapis.com/NAlice.TVideoCallCapability'
                    },
                ]
            },
        ],
    }
