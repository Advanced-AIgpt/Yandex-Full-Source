VIDEO_CALL_STATE_UPDATE_FRAME = {
    'typed_semantic_frame': {
        'endpoint_state_updates_semantic_frame': {
            'request': {
                'request_value': {
                    'endpoint_updates': [{
                        'id': 'feedface-e8a2-4439-b2e7-689d95f277b7',
                        'meta': {
                            'type': 'SpeakerEndpointType',
                            'device_info': {
                                'manufacturer': '',
                                'model': '',
                                'hw_version': '',
                                'sw_version': ''
                            }
                        },
                        'status': {
                            'status': 'Unknown'
                        },
                        'capabilities': [{
                            'meta': {
                                'retrievable': 'false',
                                'reportable': 'false',
                                'supported_events': [],
                                'supported_directives': [
                                    'StartVideoCallLoginDirectiveType'
                                ]
                            },
                            '@type': 'type.googleapis.com/NAlice.TVideoCallCapability',
                            'state': {
                                'provider_states': [
                                    {
                                        'telegram_provider_state': {
                                            'login': {
                                                'full_contacts_upload_finished': 'false',
                                                'user_id': '',
                                                'state': 'InProgress'
                                            }
                                        }
                                    }
                                ]
                            }
                        }]
                    }]
                },
            }
        }
    },
    'analytics': {
        'origin': 'SmartSpeaker',
        'purpose': 'endpoint_state_updates'
    }
}

IOT_STATE_UPDATE_FRAME = {
    'typed_semantic_frame': {
        'endpoint_state_updates_semantic_frame': {
            'request': {
                'request_value': {
                    'endpoint_updates': [{
                        'id': 'feedface-e8a2-4439-b2e7-689d95f277b7',
                        'meta': {
                            'type': 'SpeakerEndpointType',
                            'device_info': {
                                'manufacturer': '',
                                'model': '',
                                'hw_version': '',
                                'sw_version': ''
                            }
                        },
                        'status': {
                            'status': 'Unknown'
                        },
                        'capabilities': [{
                            '@type': 'type.googleapis.com/NAlice.TOnOffCapability',
                            'state': {'on': 'true'}
                        }]
                    }]
                },
            }
        }
    },
    'analytics': {
        'origin': 'SmartSpeaker',
        'purpose': 'endpoint_state_updates'
    }
}
