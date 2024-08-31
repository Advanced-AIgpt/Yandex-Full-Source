CONTACTS_UPDATE_FRAME = {
    'typed_semantic_frame': {
        "update_contacts_request_semantic_frame": {
            "update_request": {
                "request_value": {
                    "updated_contacts": [
                        {
                            "telegram_contact_info": {
                                "user_id": "44062825",
                                "provider": "telegram",
                                "contact_id": "310968595",
                                "first_name": "Иван",
                                "second_name": "Иванов"
                            }
                        }
                    ],
                    "removed_contacts": [
                        {
                            "telegram_contact_info": {
                                "user_id": "44062825",
                                "provider": "telegram",
                                "contact_id": "310968777"
                            }
                        }
                    ]
                }
            }
        }
    },
    "analytics": {
        "product_scenario": "contacts",
        "purpose": "update_contacts",
        "origin": "Scenario"
    },
    "origin": {
        "device_id": "another-device-id",
        "uuid": "another-uuid"
    }
}

CONTACTS_UPLOAD_FRAME = {
    'typed_semantic_frame': {
        "upload_contacts_request_semantic_frame": {
            "upload_request": {
                "request_value": {
                    "added_contacts": [
                        {
                            "telegram_contact_info": {
                                "user_id": "44062825",
                                "provider": "telegram",
                                "contact_id": "310968595",
                                "first_name": "Иван",
                                "second_name": "Иванов"
                            }
                        },
                        {
                            "telegram_contact_info": {
                                "user_id": "44062825",
                                "provider": "telegram",
                                "contact_id": "310961111",
                                "first_name": "Петр",
                                "second_name": "Петров"
                            }
                        }
                    ]
                }
            }
        }
    },
    "analytics": {
        "product_scenario": "contacts",
        "purpose": "upload_contacts",
        "origin": "Scenario"
    },
    "origin": {
        "device_id": "another-device-id",
        "uuid": "another-uuid"
    }
}
