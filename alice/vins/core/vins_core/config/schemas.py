# coding: utf-8

form_schema = {
    "$schema": "http://json-schema.org/draft-04/schema#",
    "definitions": {

        "slot_name": {
            "type": "string",
            "pattern": r"^[\w\d_\-\.]+$"
        },

        # --- handlers definitions ---

        # - handler_nlg definition -

        "handler_nlg": {
            "type": "object",
            "properties": {
                "handler": {
                    "type": "string",
                    "pattern": "^nlg$"
                },
                "phrase_id": {
                    "type": "string",
                    "pattern": r"^[\w\d_\-_\.]+$"
                }
            },
            "required": ["handler", "phrase_id"]
        },

        # - handler_clear_slot definition -

        "handler_clear_slot": {
            "type": "object",
            "properties": {
                "handler": {
                    "type": "string",
                    "pattern": "^clear_slot$"
                },
                "slot": {
                    "$ref": "#/definitions/slot_name"
                }
            },
            "required": ["handler", "slot"]
        },

        # - handler_change_slot_properties definition -

        "handler_change_slot_properties": {
            "type": "object",
            "properties": {
                "handler": {
                    "type": "string",
                    "pattern": "^change_slot_properties$"
                },
                "slot": {
                    "$ref": "#/definitions/slot_name"
                },
                "optional": {
                    "type": "boolean"
                }
            },
            "required": ["handler", "slot"]
        },

        # - handler_restore_prev_form definition -

        "handler_restore_prev_form": {
            "type": "object",
            "properties": {
                "handler": {
                    "type": "string",
                    "pattern": "^restore_prev_form$"
                }
            },
            "required": ["handler"]
        },

        # - handler_callback definition -

        "handler_callback": {
            "type": "object",
            "properties": {
                "handler": {
                    "type": "string",
                    "pattern": "^callback$"
                },
                "sync": {
                    "enum": [False]
                },
                "name": {
                    "type": "string",
                }
            },
            "required": ["handler", "name"]
        },

        # - handler_general_conversation definition -

        "handler_general_conversation": {
            "type": "object",
            "properties": {
                "handler": {
                    "type": "string",
                    "pattern": "^general_conversation$",
                },
                "max_context_len": {
                    "type": "integer",
                },
                "gc_host": {
                    "type": "string",
                },
                "gc_port": {
                    "type": "integer",
                },
                "gc_model": {
                    "type": "string",
                },
                "gc_temperature": {
                    "type": "number",
                },
                "gc_max_response_len": {
                    "type": "integer",
                },
            },
            "required": ["handler"]
        },

        # - handler definition -

        "handler": {
            "oneOf": [
                {"$ref": "#/definitions/handler_nlg"},
                {"$ref": "#/definitions/handler_callback"},
                {"$ref": "#/definitions/handler_general_conversation"},
                {"$ref": "#/definitions/handler_clear_slot"},
                {"$ref": "#/definitions/handler_change_slot_properties"},
                {"$ref": "#/definitions/handler_restore_prev_form"},
            ]
        },

        # --- event definition ---

        "event": {
            "type": "object",
            "properties": {
                "event": {
                    "enum": [
                        "init", "fill", "ask", "confirm",
                        "select", "change", "submit"
                    ]
                },
                "handlers": {
                    "type": "array",
                    "items": {"$ref": "#/definitions/handler"}
                }
            },
            "required": ["event", "handlers"]
        },

        # --- slot definition ---

        "slot": {
            "type": "object",
            "properties": {
                "slot": {
                    "$ref": "#/definitions/slot_name"
                },
                "type": {
                    "type": "string",
                    "pattern": r"^[\w\d_\-\.]+$"
                },
                "optional": {
                    "enum": [True, False]
                },
                "matching_type": {
                    "enum": ["exact", "inside", "overlap"]
                },
                "events": {
                    "type": "array",
                    "items": {"$ref": "#/definitions/event"}
                },
                "share_tags": {
                    "type": "array",
                    "items": {"type": "string"}
                },
                "expected_values": {
                    "type": "array",
                    "items": {"type": "string"}
                },
                "import_tags": {
                    "type": "array",
                    "items": {"type": "string"}
                }
            },
            "required": ["slot", "type", "optional"]
        },

        # --- required_slot_group definition ---

        "required_slot_group": {
            "type": "object",
            "properties": {
                "slot_to_ask": {
                    "$ref": "#/definitions/slot_name"
                },
                "slots": {
                    "type": "array",
                    "items": {
                        "$ref": "#/definitions/slot_name"
                    },
                }
            },
            "required": ["slot_to_ask", "slots"]
        },

    },
    "type": "object",
    "properties": {
        "form": {
            "type": "string",
            "pattern": r"^[\w\d_\-]+$"
        },
        "events": {
            "type": "array",
            "items": {"$ref": "#/definitions/event"}
        },
        "slots": {
            "type": "array",
            "items": {"$ref": "#/definitions/slot"}
        },
        "required_slot_groups": {
            "type": "array",
            "items": {"$ref": "#/definitions/required_slot_group"}
        }
    },
    "required": ["form"],
    "additionalProperties": False
}

project_schema = {
    "$schema": "http://json-schema.org/draft-04/schema#",
    "definitions": {
        "entity": {
            "type": "object",
            "properties": {
                "entity": {
                    "type": "string",
                    "pattern": r"^[\w\d_\-]+$"
                },
                "path": {"type": "string"}
            },
            "required": ["path", "entity"]
        },
        "intent": {
            "type": "object",
            "properties": {
                "intent": {
                    "type": "string",
                    "pattern": r"^[\w\d_\-.]+$"
                },
                "parent": {
                    "type": "string",
                    "pattern": r"^[\w\d_\-]+$"
                },
                "parent_examples_in_tagger": {
                    "type": "boolean"
                },
                "prior": {
                    "type": "number"
                },
                "fallback": {
                    "type": "boolean"
                },
                "total_fallback": {
                    "type": "boolean"
                },
                "fallback_threshold": {
                    "type": "number"
                },
                "trainable_classifiers": {
                    "type": "array", "items": {"type": "string"}
                },
                "nlu": {
                    "type": "object",
                    "properties": {
                        "path": {"type": "string"},
                        "data": {"type": "object"},
                        "paths": {"type": "array", "items": {"type": "string"}}
                    }
                },
                "dm": {
                    "type": "object",
                    "properties": {
                        "path": {"type": "string"},
                        "data": {"type": "object"}
                    }
                },
                "nlg": {
                    "type": "object",
                    "properties": {
                        "path": {"type": "string"},
                        "data": {"type": "object"}
                    }
                }
            },
            "required": ["intent"],
            "additionalProperties": False
        },
        "include": {
            "type": "object",
            "properties": {
                "type": {"type": "string"},
                "path": {"type": "string"}
            },
            "required": ["type", "path"],
            "additionalProperties": False
        }
    },
    "type": "object",
    "properties": {
        "name": {
            "type": "string",
            "pattern": r"^[\w\d_\-]+$"
        },
        "includes": {
            "type": "array",
            "items": {"$ref": "#/definitions/include"}
        },
        "entities": {
            "type": "array",
            "items": {"$ref": "#/definitions/entity"}
        },
        "intents": {
            "type": "array",
            "items": {"$ref": "#/definitions/intent"}
        },
        "microintents": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "path": {"type": "string"},
                    "trainable_classifiers": {
                        "type": "array", "items": {"type": "string"}
                    },
                    "nlg_phrase_id": {"type": "string"},
                    "nlg_includes": {
                        "type": "array", "items": {"type": "string"}
                    },
                    "form_submit_handler": {
                        "type": "object",
                        "properties": {
                            "handler": {"type": "string"},
                            "name": {"type": "string"},
                        }
                    }
                }
            }
        },
    },
    "required": ["name"],
    "additionalProperties": False,
    "anyOf": [{
        "required": ["includes"],
    }, {
        "required": ["intents"],
    }]
}

app_schema = {
    "$schema": "http://json-schema.org/draft-04/schema#",
    "definitions": {
        "intent_classifier": {
            "type": "object",
            "properties": {
                "model": {"type": "string"},
                "name": {"type": "string"},
                "params": {"type": "object"},
                "features": {
                    "type": "array",
                    "items": {
                        "type": "string"
                    }
                }
            },
            "required": ["model"],
        }
    },
    "type": "object",
    "properties": {
        "project": project_schema,
        "nlu": {
            "type": "object",
            "properties": {
                "feature_extractors": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "type": {"type": "string"},
                            "id": {"type": "string"},
                        },
                        "additionalProperties": True,
                        "required": ["type", "id"]
                    }
                },
                "intent_classifiers": {
                    "type": "array",
                    "items": {
                        "oneOf": [
                            {
                                "$ref": "#/definitions/intent_classifier"
                            }, {
                                "type": "array",
                                "items": {"$ref": "#/definitions/intent_classifier"}
                            }
                        ]
                    }
                },
                "transition_model": {
                    "type": "object",
                    "properties": {
                        "model_name": {"type": "string"},
                        "custom_rules": {
                            "type": "object",
                            "properties": {
                                "enable": {"type": "boolean"},
                                "path": {"type": "string"}
                            },
                            "additionalProperties": False,
                            "required": ["enable", "path"]
                        }
                    },
                    "additionalProperties": True
                },
                "fst": {
                    "type": "object",
                    "properties": {
                        "resource": {"type": "string"},
                        "parsers": {"type": "array", "items": {"type": "string"}}
                    },
                }
            },
            "required": ["intent_classifiers"],
            "additionalProperties": True,
        },
        "nlg": {
            "type": "object",
            "properties": {
                "includes": {"type": "array"},
            },
            "additionalProperties": True
        },
        "samples_extractor": {
            "type": "object",
            "additionalProperties": True
        },
        "form_filling": {
            "type": "object",
            "additionalProperties": True
        },
        "post_classifier": {
            "type": "object",
            "additionalProperties": True
        }
    },
    "required": ["project"],
    "additionalProperties": False
}

app_schema['definitions'].update(project_schema['definitions'])

transition_rules_schema = {
    "$schema": "http://json-schema.org/draft-04/schema#",
    "definitions": {
        "unconditional": {
            "type": "object",
            "properties": {
                "boost": {"type": "number"},
                "curr_intent": {"type": "string"},
                "type": {"type": "string", "pattern": "^unconditional"}
            },
            "required": ["curr_intent", "type"]
        },
        "check_form_active_slots_rule": {
            "type": "object",
            "properties": {
                "boost": {"type": "number"},
                "prev_intent": {"type": "string"},
                "curr_intent": {"type": "string"},
                "type": {"type": "string", "pattern": "^check_form_active_slots"}
            },
            "required": ["prev_intent", "curr_intent", "type"]
        },
        "check_prev_curr_intent_rule": {
            "type": "object",
            "properties": {
                "boost": {"type": "number"},
                "prev_intent": {
                    "oneOf": [
                        {
                            "type": "array",
                            "items": {"type": "string"}
                        },
                        {"type": "string"}
                    ]
                },
                "curr_intent": {"type": "string"},
                "type": {"type": "string", "pattern": "^check_prev_curr_intent"}
            },
            "required": ["prev_intent", "curr_intent", "type"]
        },
        "check_form_slot_value_rule": {
            "type": "object",
            "properties": {
                "boost": {"type": "number"},
                "slot": {"type": "string"},
                "slot_value_key": {"type": "string"},
                "value": {},
                "type": {"type": "string"},
            },
            "required": ["type", "slot", "value"]
        },
        "logic_rule": {
            "type": "object",
            "properties": {
                "operation": {"type": "string"},
                "boost": {"type": "number"},
                "type": {"type": "string", "pattern": "^logic_rule"},
                "children": {
                    "type": "array",
                    "items": {
                        "oneOf": [
                            {
                                "$ref": "#/definitions/check_form_active_slots_rule"
                            },
                            {
                                "$ref": "#/definitions/check_prev_curr_intent_rule"
                            },
                            {
                                "$ref": "#/definitions/check_form_slot_value_rule"
                            },
                            {
                                "$ref": "#/definitions/logic_rule"
                            },
                            {
                                "$ref": "#/definitions/allow_prev_intents"
                            },
                            {
                                "$ref": "#/definitions/unconditional"
                            },
                        ]
                    }
                }
            },
            "required": ["operation", "type", "children"]
        },
        "allow_prev_intents": {
            "type": "object",
            "properties": {
                "boost": {"type": "number"},
                "prev_intents": {
                    "type": "array",
                    "items": {"type": "string"},
                },
                "curr_intent": {"type": "string"},
                "type": {"type": "string", "pattern": "^allow_prev_intents"}
            },
            "required": ["prev_intents", "curr_intent", "type"]
        },
    },
    "type": "array",
    "items": {
        "oneOf": [
            {
                "allOf": [
                    {"required": ["boost"]},
                    {"$ref": "#/definitions/unconditional"}
                ]
            },
            {
                "allOf": [
                    {"required": ["boost"]},
                    {"$ref": "#/definitions/check_form_active_slots_rule"}
                ]
            },
            {
                "allOf": [
                    {"required": ["boost"]},
                    {"$ref": "#/definitions/check_prev_curr_intent_rule"}
                ]
            },
            {
                "allOf": [
                    {"required": ["boost"]},
                    {"$ref": "#/definitions/check_form_slot_value_rule"}
                ]
            },
            {
                "allOf": [
                    {"required": ["boost"]},
                    {"$ref": "#/definitions/logic_rule"}
                ]
            },
            {
                "allOf": [
                    {"required": ["boost"]},
                    {"$ref": "#/definitions/allow_prev_intents"}
                ]
            }
        ]
    },
    "additionalProperties": False
}

special_buttons_schema = {
    "$schema": "http://json-schema.org/draft-04/schema#",
    "definitions": {
        "button": {
            "type": "object",
            "properties": {
                "type": {"enum": ['like_button', 'dislike_button']},
                "text": {"type": "string"},
                "name": {"type": "string"},
                "title": {"type": "string"},
                "sub_list": {
                    "type": "object",
                    "properties": {
                        "title": {"type": "string"},
                        "default_buttons": {
                            "type": "array",
                            "items": {"$ref": "#/definitions/button"},
                            "minItems": 1
                        },
                        "default": {"$ref": "#/definitions/button"}
                    },
                    "required": ["title", "default_buttons", "default"]
                }
            },
            "required": ["text", "name"]
        }
    },
    "type": "object",
    "properties": {
        "special_buttons": {
            "type": "array",
            "items": {"$ref": "#/definitions/button"}
        },
        "feedback_negative_buttons": {
            "type": "array",
            "items": {"$ref": "#/definitions/button"}
        }
    },
    "required": ["special_buttons"],
    "additionalProperties": False
}

microintent_item_schema = {
    'type': 'object',
    'required': ['nlu'],
    'properties': {
        'nlu': {'type': 'array', 'items': {'type': 'string'}},
        'nlg': {
            'oneOf': [
                {
                    'type': 'array',
                    'items': {'type': 'string'},
                },
                {
                    'type': 'object',
                    'required': ['else']  # list of allowed checks is added later
                }
            ]
        },
        'suggests': {'type': 'array', 'items': {'type': 'string'}},
        'gc_fallback': {'type': 'boolean'},
        'allow_repeats': {'type': 'boolean'}
    }
}
