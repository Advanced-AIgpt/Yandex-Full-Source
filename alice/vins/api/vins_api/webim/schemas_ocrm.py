# coding: utf-8

string = {"type": "string"}

string_or_null = {"type": ["string", "null"]}

object_t = {"type": ["object", "null"]}

text = {
    "type": "object",
    "properties": {
        "MessageText": string
    },
    "required": ["MessageText"],
    "additionalProperties": False
}

plain = {
    "type": "object",
    "properties": {
        "ChatId": string,
        "Text": text,
        "Image": object_t,
        "MiscFile": object_t,
        "Sticker": object_t,
        "Card": object_t
    },
    "oneOf": [
        {"required": ["ChatId", "Text"]},
        {"required": ["ChatId", "Image"]},
        {"required": ["ChatId", "MiscFile"]},
        {"required": ["ChatId", "Sticker"]},
        {"required": ["ChatId", "Card"]}
    ]
}

message = {
    "type": "object",
    "properties": {
        "Plain": plain
    },
    "required": [
        "Plain"
    ]
}

ocrm_message = {
    "type": "object",
    "properties": {
        "Message": message
    },
    "required": [
        "Message"
    ]
}
