# coding: utf-8

from vins_api.speechkit.schemas import UUID_PATTERN

string = {"type": "string"}

string_or_null = {"type": ["string", "null"]}

chat = {
    "type": "object",
    "properties": {
        "id": {"type": "integer"}
    },
    "required": [
        "id"
    ],
    "additionalProperties": False
}

visitor_fields = {
    "type": "object",
    "properties": {
        "id": string,
        "display_name": string_or_null,
        "phone": string_or_null,
        "email": string_or_null,
        "profile_url": string_or_null,
        "avatar_url": string_or_null,
        "login": string_or_null,
        "comment": string_or_null,
        "info": string_or_null
    },
    "additionalProperties": True
}

visitor = {
    "type": "object",
    "properties": {
        "id": {
            "type": "string",
            "pattern": UUID_PATTERN
        },
        "fields": visitor_fields
    },
    "required": [
        "id", "fields"
    ],
    "additionalProperties": False
}

button = {
    "type": "object",
    "properties": {
        "id": string,
        "text": string
    },
    "required": ["id", "text"],
    "additionalProperties": False
}

button_data = {
    "type": "object",
    "properties": {
        "button": button,
        "request": {
            "type": "object",
            "properties": {
                "messageId": string
            },
            "required": ["messageId"],
            "additionalProperties": False
        }
    },
    "required": ["button", "request"],
    "additionalProperties": False
}

file_data = {
    "type": "object",
    "properties": {
        "url": {"type": "string", "format": "uri"},
        "name": string,
        "media_type": string,
        "size": {"type": "integer"}
    },
    "required": ["url", "name", "media_type", "size"],
    "additionalProperties": False
}

message = {
    "type": "object",
    "properties": {
        "id": {
            "type": "string",
            "pattern": UUID_PATTERN
        },
        "kind": {
            "enum": [
                "visitor",
                "file_visitor",
                "keyboard_response"
            ]
        },
        "text": string,
        "data": {"type": "object"}
    },
    "required": ["id", "kind"],
    "oneOf": [
        {
            "properties": {
                "kind": {"const": "visitor"}
            },
            "required": ["id", "kind", "text"]
        },
        {
            "properties": {
                "kind": {"const": "file_visitor"}
            },
            "required": ["id", "kind", "data"],
            "allOf": [
                {
                    "properties": {
                        "data": file_data
                    }
                }
            ]
        },
        {
            "properties": {
                "kind": {"const": "keyboard_response"}
            },
            "required": ["id", "kind", "data"],
            "allOf": [
                {
                    "properties": {
                        "data": button_data
                    }
                }
            ]
        }
    ],
    "additionalProperties": False
}

webim_schema = {
    "type": "object",
    "properties": {
        "event": {
            "enum": ["new_message", "new_chat"]
        },
        "message": message,
        "messages": {"type": "array", "items": message},
        "chat_id": {"type": "integer"},
        "chat": chat,
        "visitor": visitor
    },
    "oneOf": [
        {
            "properties": {
                "event": {"const": "new_chat"}
            },
            "required": ["chat", "visitor"]
        },
        {
            "properties": {
                "event": {"const": "new_message"}
            },
            "required": ["chat_id", "message"]
        }
    ],
    "required": ["event"],
    "additionalProperties": False
}
