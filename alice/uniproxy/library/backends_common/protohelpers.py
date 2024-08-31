from google.protobuf import json_format
import json


def proto_from_json(proto_type, obj):
    """
    tmp = {}
    for k in obj:
        if k in vars(proto_type):
            tmp[k] = obj[k]
    """
    return json_format.Parse(json.dumps(obj), proto_type(), ignore_unknown_fields=True)


def proto_to_json(proto, use_integers_for_enums=False):
    return json.loads(json_format.MessageToJson(
        proto,
        including_default_value_fields=True,
        use_integers_for_enums=use_integers_for_enums
    ))
