#!/usr/bin/env python

from google.protobuf.pyext._message import FieldDescriptor
import base64
import json


PROTOBUF_NUMBER_TYPES = [
    FieldDescriptor.TYPE_INT32,
    FieldDescriptor.TYPE_INT64,
    FieldDescriptor.TYPE_UINT32,
    FieldDescriptor.TYPE_UINT64,
    FieldDescriptor.TYPE_DOUBLE,
    FieldDescriptor.TYPE_FLOAT
]


# ====================================================================================================================
def MessageToDict(message, use_integers_for_enums=True, json_content_fields=[], skip_fields=[], bytes_as_json=False):
    Data = {}

    ListFields = message.ListFields()
    for descriptor, value in ListFields:
        if descriptor.name in skip_fields:
            continue

        if descriptor.label == FieldDescriptor.LABEL_REPEATED:
            Item = []

            if descriptor.type == FieldDescriptor.TYPE_MESSAGE:
                for x in value:
                    Item.append(MessageToDict(
                        x,
                        use_integers_for_enums,
                        json_content_fields,
                        bytes_as_json=bytes_as_json
                    ))
            elif descriptor.type == FieldDescriptor.TYPE_BYTES:
                if bytes_as_json or descriptor.name in json_content_fields:
                    for x in value:
                        Item.append(json.loads(x.decode('utf-8')))
                else:
                    for x in value:
                        Item.append(base64.b64encode(x).decode('utf-8'))
            else:
                for x in value:
                    Item.append(x)

            Data[descriptor.name] = Item
        else:
            if descriptor.type == FieldDescriptor.TYPE_MESSAGE:
                Data[descriptor.name] = MessageToDict(
                    value,
                    use_integers_for_enums,
                    json_content_fields,
                    bytes_as_json=bytes_as_json
                )
            elif descriptor.type == FieldDescriptor.TYPE_BYTES:
                if bytes_as_json or descriptor.name in json_content_fields:
                    Data[descriptor.name] = json.loads(value.decode('utf-8'))
                else:
                    Data[descriptor.name] = base64.b64encode(value).decode('utf-8')
            else:
                Data[descriptor.name] = value

    return Data


# ====================================================================================================================
def MessageToDict2(message, json_fields={}, skip_fields=[]):
    Data = {}

    JsonFields = json_fields.get(message.DESCRIPTOR.name, [])

    ListFields = message.ListFields()

    for descriptor, value in ListFields:
        if descriptor.name in skip_fields:
            continue

        if descriptor.label == FieldDescriptor.LABEL_REPEATED:
            Item = []

            if descriptor.type == FieldDescriptor.TYPE_BYTES:
                if JsonFields and descriptor.name in JsonFields:
                    for x in value:
                        Item.append(json.loads(x.decode('utf-8')))
                else:
                    for x in value:
                        Item.append(base64.b64encode(x).decode('utf-8'))

            elif descriptor.type == FieldDescriptor.TYPE_MESSAGE:
                for x in value:
                    Item.append(MessageToDict2(x, json_fields, skip_fields))

            else:
                for x in value:
                    Item.append(x)

            Data[descriptor.name] = Item
        else:
            if descriptor.type == FieldDescriptor.TYPE_BYTES:
                if JsonFields and descriptor.name in JsonFields:
                    Data[descriptor.name] = json.loads(value.decode('utf-8'))
                else:
                    Data[descriptor.name] = base64.b64encode(value).decode('utf-8')
            elif descriptor.type == FieldDescriptor.TYPE_MESSAGE:
                Data[descriptor.name] = MessageToDict2(value, json_fields, skip_fields)
            else:
                Data[descriptor.name] = value

    return Data


# ====================================================================================================================
def MessageToJsonEx(message, use_integers_for_enums=True, json_content_fields=[]):
    data = '{'

    ListFields = message.ListFields()
    ListFieldsLast = len(ListFields)
    Index = 0
    for descriptor, value in ListFields:
        data += '"'
        data += descriptor.name
        data += '":'

        if descriptor.label == FieldDescriptor.LABEL_REPEATED:
            data += '['

            if descriptor.type in PROTOBUF_NUMBER_TYPES:
                data += ','.join([str(x) for x in value])

            elif descriptor.type == FieldDescriptor.TYPE_STRING:
                data += ','.join(['"' + x + '"' for x in value])

            elif descriptor.type == FieldDescriptor.TYPE_BYTES:
                if descriptor.name in json_content_fields:
                    data += ','.join(['"' + x.decode('utf-8') + '"' for x in value])
                else:
                    data += ','.join(['"' + base64.b64encode(value).decode('utf-8') + '"' for x in value])

            elif descriptor.type == FieldDescriptor.TYPE_MESSAGE:
                data += ','.join([
                    MessageToJsonEx(msg, use_integers_for_enums, json_content_fields)
                    for msg in value
                ])

            elif descriptor.type == FieldDescriptor.TYPE_BOOL:
                data += ','.join([
                    'true' if x else 'false' for x in value
                ])

            data += ']'
        else:
            if descriptor.type in PROTOBUF_NUMBER_TYPES:
                data += str(value)

            elif descriptor.type == FieldDescriptor.TYPE_STRING:
                data += '"'
                data += value
                data += '"'

            elif descriptor.type == FieldDescriptor.TYPE_BYTES:
                if descriptor.name in json_content_fields:
                    data += value.decode('utf-8')
                else:
                    data += '"'
                    data += base64.standard_b64encode(value).decode('utf-8')
                    data += '"'

            elif descriptor.type == FieldDescriptor.TYPE_MESSAGE:
                data += MessageToJsonEx(value, use_integers_for_enums, json_content_fields)

            elif descriptor.type == FieldDescriptor.TYPE_ENUM:
                data += str(value)

            elif descriptor.type == FieldDescriptor.TYPE_BOOL:
                data += 'true' if value else 'false'

        Index += 1
        if Index < ListFieldsLast:
            data += ','

    data += '}'

    return data
