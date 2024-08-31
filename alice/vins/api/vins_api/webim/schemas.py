# coding: utf-8

from vins_api.speechkit.schemas import UUID_PATTERN

string = {"type": "string"}

chat = {
    "type": "object",
    "properties": {
        "id": {
            "type": "string",
            "pattern": UUID_PATTERN
        }
    },
    "required": [
        "id"
    ],
    "additionalProperties": False
}

visitor = {
    "type": "object",
    "properties": {
        "id": {
            "type": "string",
            "pattern": UUID_PATTERN
        }
    },
    "required": [
        "id"
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

response = {
    "type": "object",
    "properties": {
        "button": button
    },
    "required": ["button"],
    "additionalProperties": False
}

message = {
    "type": "object",
    "properties": {
        "text": string,
        "response": response,
        "kind": {
            "enum": [
                "visitor",
                "keyboard_response"
            ]
        }
    },
    "required": [
        "kind"
    ],
    "oneOf": [
        {
            "properties": {
                "kind": {"const": "visitor"}
            },
            "required": ["text"]
        },
        {
            "properties": {
                "kind": {"const": "keyboard_response"}
            },
            "required": ["response"]
        }
    ],
    "additionalProperties": False
}

messages = {
    "type": "array",
    "items": message
}


webim_schema = {
    "type": "object",
    "properties": {
        "event": {
            "enum": ["new_chat", "new_message"]
        },
        "chat": chat,
        "visitor": visitor,
        "text": string,
        "kind": {
            "enum": ["visitor", "keyboard_response"]
        },
        "response": response,
        "messages": messages
    },
    "required": [
        "event",
        "chat"
    ],
    "oneOf": [
        {
            "properties": {
                "event": {"const": "new_chat"}
            },
            "required": ["visitor"]
        },
        {
            "properties": {
                "event": {"const": "new_message"}
            },
            "required": ["kind"],
            "oneOf": [
                {
                    "properties": {
                        "kind": {"const": "visitor"}
                    },
                    "required": ["text"]
                },
                {
                    "properties": {
                        "kind": {"const": "keyboard_response"}
                    },
                    "required": ["response"]
                }
            ]
        }
    ],
    "additionalProperties": False
}
