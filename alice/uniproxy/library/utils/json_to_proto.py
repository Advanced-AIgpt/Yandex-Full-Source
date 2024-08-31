from google.protobuf.pyext._message import FieldDescriptor
import json
import base64


PROTOBUF_INTEGER_TYPES = (
    FieldDescriptor.TYPE_SINT32,
    FieldDescriptor.TYPE_SINT64,
    FieldDescriptor.TYPE_INT32,
    FieldDescriptor.TYPE_INT64,
    FieldDescriptor.TYPE_UINT32,
    FieldDescriptor.TYPE_UINT64,
)

PROTOBUF_FLOAT_TYPES = (
    FieldDescriptor.TYPE_FLOAT,
    FieldDescriptor.TYPE_DOUBLE
)


# --------------------------------------------------------------------------------------------------------------------
def JsonToMessage(proto, data, json_content_fields=[]):
    return DictToMessage(proto, json.loads(data), json_content_fields)


# --------------------------------------------------------------------------------------------------------------------
def DictToMessage(proto, data, json_content_fields=[], message=None):
    if hasattr(proto, 'DESCRIPTOR'):
        message = proto() if message is None else message
    else:
        proto = proto._concrete_class
        message = proto() if message is None else message

    for field in proto.DESCRIPTOR.fields:
        field_name = field.json_name
        field_value = data.get(field_name)

        if field_value is None:
            continue

        if field.label == FieldDescriptor.LABEL_REPEATED:
            for value in field_value:
                attr = getattr(message, field_name)

                if field.type in PROTOBUF_INTEGER_TYPES:
                    attr.append(value)

                elif field.type in PROTOBUF_FLOAT_TYPES:
                    attr.append(value)

                elif field.type == FieldDescriptor.TYPE_BOOL:
                    attr.append(value)

                elif field.type == FieldDescriptor.TYPE_ENUM:
                    attr.append(value)

                elif field.type == FieldDescriptor.TYPE_STRING:
                    if field_name in json_content_fields:
                        attr.append(json.dumps(value))
                    else:
                        attr.append(value)

                elif field.type == FieldDescriptor.TYPE_BYTES:
                    if field_name in json_content_fields:
                        attr.append(json.dumps(value).encode('utf-8'))
                    else:
                        attr.append(value)

                elif field.type == FieldDescriptor.TYPE_MESSAGE:
                    m = attr.add()
                    DictToMessage(type(m), value, json_content_fields=json_content_fields, message=m)

        else:
            if field.type in PROTOBUF_INTEGER_TYPES:
                setattr(message, field_name, field_value)

            elif field.type in PROTOBUF_FLOAT_TYPES:
                setattr(message, field_name, field_value)

            elif field.type == FieldDescriptor.TYPE_BOOL:
                setattr(message, field_name, field_value)

            elif field.type == FieldDescriptor.TYPE_ENUM:
                setattr(message, field_name, field_value)

            elif field.type == FieldDescriptor.TYPE_STRING:
                if field_name in json_content_fields:
                    setattr(message, field_name, json.dumps(field_value))
                else:
                    setattr(message, field_name, field_value)

            elif field.type == FieldDescriptor.TYPE_BYTES:
                if field_name in json_content_fields:
                    setattr(message, field_name, json.dumps(field_value).encode('utf-8'))
                else:
                    if isinstance(field_value, str):
                        setattr(message, field_name, field_value.encode('utf-8'))
                    elif isinstance(field_value, bytes):
                        setattr(message, field_name, field_value)
                    else:
                        setattr(message, field_name, str(field_value).encode('utf-8'))

            elif field.type == FieldDescriptor.TYPE_MESSAGE:
                inner = getattr(message, field_name)
                if field_value:
                    inner = DictToMessage(
                        inner.DESCRIPTOR,
                        field_value,
                        json_content_fields=json_content_fields,
                        message=inner
                    )
                else:
                    inner.SetInParent()

    return message


def DictToMessage2(proto, data, json_fields=[], message=None):
    if hasattr(proto, 'DESCRIPTOR'):
        message = proto() if message is None else message
    else:
        proto = proto._concrete_class
        message = proto() if message is None else message

    JsonFields = json_fields.get(proto.DESCRIPTOR.name, [])

    for field in proto.DESCRIPTOR.fields:
        field_name = field.json_name
        field_value = data.get(field_name)

        if field_value is None:
            continue

        if field.label == FieldDescriptor.LABEL_REPEATED:
            attr = getattr(message, field_name)
            for value in field_value:
                if field.type == FieldDescriptor.TYPE_BYTES:
                    if JsonFields and field_name in JsonFields:
                        attr.append(json.dumps(value).encode('utf-8'))
                    else:
                        attr.append(value.encode('utf-8'))

                    if isinstance(value, str):
                        field_data = base64.b64decode(field_value)
                        attr.append(field_data)
                    elif isinstance(field_value, bytes):
                        attr.append(field_value)
                    else:
                        attr.append(str(field_value).encode('utf-8'))

                elif field.type == FieldDescriptor.TYPE_MESSAGE:
                    m = attr.add()
                    DictToMessage2(type(m), value, json_fields=json_fields, message=m)

                else:
                    attr.append(value)

        else:
            if field.type in PROTOBUF_INTEGER_TYPES:
                setattr(message, field_name, field_value)

            elif field.type in PROTOBUF_FLOAT_TYPES:
                setattr(message, field_name, field_value)

            elif field.type == FieldDescriptor.TYPE_BOOL:
                setattr(message, field_name, field_value)

            elif field.type == FieldDescriptor.TYPE_ENUM:
                setattr(message, field_name, field_value)

            elif field.type == FieldDescriptor.TYPE_STRING:
                setattr(message, field_name, field_value)

            elif field.type == FieldDescriptor.TYPE_BYTES:
                if JsonFields and field_name in JsonFields:
                    setattr(message, field_name, json.dumps(field_value).encode('utf-8'))
                else:
                    if isinstance(field_value, str):
                        field_data = base64.b64decode(field_value)
                        setattr(message, field_name, field_data)
                    elif isinstance(field_value, bytes):
                        setattr(message, field_name, field_value)
                    else:
                        setattr(message, field_name, str(field_value).encode('utf-8'))

            elif field.type == FieldDescriptor.TYPE_MESSAGE:
                inner = getattr(message, field_name)
                inner.SetInParent()
                if field_value:
                    inner = DictToMessage2(
                        inner.DESCRIPTOR,
                        field_value,
                        json_fields=json_fields,
                        message=inner
                    )

    return message
