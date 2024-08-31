import pytest
from google.protobuf.descriptor_pb2 import FileDescriptorSet
from google.protobuf.text_format import Parse

from alice.wonderlogs.proto_tests.lib.proto_checker import compare_file_descriptor_sets

FILE_DESCRIPTOR_SET = '''
file {
    name: "alice/wonderlogs/test.proto"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "Message"
        field {
            name: "Field"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_STRING
            options {
                [NYT.column_name]: "field"
            }
            json_name: "field"
        }
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
    }
}
'''

CHANGED_COLUMN_NAME = '''
file {
    name: "alice/wonderlogs/test.proto"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "Message"
        field {
            name: "Field"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_STRING
            options {
                [NYT.column_name]: "field2"
            }
            json_name: "field"
        }
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
    }
}
'''

DELETED_COLUMN_NAME = '''
file {
    name: "alice/wonderlogs/test.proto"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "Message"
        field {
            name: "Field"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_STRING
            options {}
            json_name: "field"
        }
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
    }
}
'''

EMPTY = ''

NOT_SNAKE_CASE1 = '''
file {
    name: "alice/wonderlogs/test.proto"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "Message"
        field {
            name: "Field"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_STRING
            options {
                [NYT.column_name]: "Field"
            }
            json_name: "field"
        }
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
    }
}
'''

NOT_SNAKE_CASE2 = '''
file {
    name: "alice/wonderlogs/test.proto"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "Message"
        field {
            name: "Field"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_STRING
            options {
                [NYT.column_name]: "field_"
            }
            json_name: "field"
        }
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
    }
}
'''

NOT_SNAKE_CASE3 = '''
file {
    name: "alice/wonderlogs/test.proto"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "Message"
        field {
            name: "Field"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_STRING
            options {
                [NYT.column_name]: "fiEld"
            }
            json_name: "field"
        }
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
    }
}
'''

FILE_DESCRIPTOR_SET_ONEOF = '''
file {
    name: "alice/wonderlogs/test.proto"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "Message"
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
        oneof_decl {
            name: "OneofField"
            options {
                [NYT.variant_field_name]: "oneof_field"
            }
        }
    }
}
'''

CHANGED_COLUMN_NAME_ONEOF = '''
file {
    name: "alice/wonderlogs/test.proto"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "Message"
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
        oneof_decl {
            name: "OneofField"
            options {
                [NYT.variant_field_name]: "oneof_field2"
            }
        }
    }
}
'''

DELETED_COLUMN_NAME_ONEOF = '''
file {
    name: "alice/wonderlogs/test.proto"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "Message"
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
        oneof_decl {
            name: "OneofField"
            options {}
        }
    }
}
'''

NOT_SNAKE_CASE_ONEOF = '''
file {
    name: "alice/wonderlogs/test.proto"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "Message"
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
        oneof_decl {
            name: "OneofField"
            options {
                [NYT.variant_field_name]: "oneofField"
            }
        }
    }
}
'''

FILE_DESCRIPTOR_SET_COMPOSITE = '''
file {
    name: "alice/wonderlogs/test.proto"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "Message"
        field {
            name: "Int32"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_INT32
            options {}
            oneof_index: 0
            json_name: "Int32"
        }
        field {
            name: "String"
            number: 2
            label: LABEL_OPTIONAL
            type: TYPE_STRING
            options {
                [NYT.column_name]: "string"
            }
            oneof_index: 0
            json_name: "String"
        }
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
        oneof_decl {
            name: "OneofField"
            options {
                [NYT.variant_field_name]: "oneof_field"
            }
        }
    }
}
'''

FILE_DESCRIPTOR_SET_COMPOSITE_CORRUPTED = '''
file {
    name: "alice/wonderlogs/test.proto"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "Message"
        field {
            name: "Int32"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_INT32
            options {
                [NYT.column_name]: "int32"
            }
            oneof_index: 0
            json_name: "Int32"
        }
        field {
            name: "String"
            number: 2
            label: LABEL_OPTIONAL
            type: TYPE_STRING
            options {}
            oneof_index: 0
            json_name: "String"
        }
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
        oneof_decl {
            name: "OneofField"
            options {
                [NYT.variant_field_name]: "oneof_field2"
            }
        }
    }
}
'''

FILE_DESCRIPTOR_SET_WITH_CORRECT_MAP = '''
file {
    name: "alice/wonderlogs/protos/tmp.proto"
    package: "NAlice.NWonderlogs"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "TMessage"
        field {
            name: "Map"
            number: 1
            label: LABEL_REPEATED
            type: TYPE_MESSAGE
            type_name: ".NAlice.NWonderlogs.TMessage.MapEntry"
            options {
                [NYT.flags]: MAP_AS_DICT
                [NYT.column_name]: "map"
            }
            json_name: "map"
        }
        nested_type {
            name: "MapEntry"
            field {
                name: "key"
                number: 1
                label: LABEL_OPTIONAL
                type: TYPE_STRING
                json_name: "key"
            }
            field {
                name: "value"
                number: 2
                label: LABEL_OPTIONAL
                type: TYPE_STRING
                json_name: "value"
            }
            options {
                map_entry: true
            }
        }
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
    }
}
'''

FILE_DESCRIPTOR_SET_WITH_INCORRECT_MAP = '''
file {
    name: "alice/wonderlogs/protos/tmp.proto"
    package: "NAlice.NWonderlogs"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "TMessage"
        field {
            name: "Map"
            number: 1
            label: LABEL_REPEATED
            type: TYPE_MESSAGE
            type_name: ".NAlice.NWonderlogs.TMessage.MapEntry"
            options {
                [NYT.column_name]: "map"
            }
            json_name: "map"
        }
        nested_type {
            name: "MapEntry"
            field {
                name: "key"
                number: 1
                label: LABEL_OPTIONAL
                type: TYPE_STRING
                json_name: "key"
            }
            field {
                name: "value"
                number: 2
                label: LABEL_OPTIONAL
                type: TYPE_STRING
                json_name: "value"
            }
            options {
                map_entry: true
            }
        }
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
    }
}
'''

FILE_DESCRIPTOR_SET_WITH_NESTED_TYPE = '''
file {
    name: "alice/wonderlogs/test.proto"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "Message"
        field {
            name: "Field"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_STRING
            options {
                [NYT.column_name]: "field"
            }
            json_name: "field"
        }
        nested_type {
            name: "TNested"
            field {
                name: "RequestId"
                number: 1
                label: LABEL_OPTIONAL
                type: TYPE_STRING
                options {
                    [NYT.column_name]: "request_id"
                }
                json_name: "request_id"
            }
        }
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
    }
}
'''

FILE_DESCRIPTOR_SET_WITH_NESTED_TYPE_DIFFERENT_NAME = '''
file {
    name: "alice/wonderlogs/test.proto"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "Message"
        field {
            name: "Field"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_STRING
            options {
                [NYT.column_name]: "field"
            }
            json_name: "field"
        }
        nested_type {
            name: "TNested"
            field {
                name: "RequestId"
                number: 1
                label: LABEL_OPTIONAL
                type: TYPE_STRING
                options {
                    [NYT.column_name]: "qweqweqwe"
                }
                json_name: "request_id"
            }
        }
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
    }
}
'''

FILE_DESCRIPTOR_SET_WITHOUT_FIELD = '''
file {
    name: "alice/wonderlogs/test.proto"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "TSpeechKitRequestProto"
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
    }
}
'''

FILE_DESCRIPTOR_SET_WITH_FIELD = '''
file {
    name: "alice/wonderlogs/test.proto"
    dependency: "mapreduce/yt/interface/protos/extension.proto"
    message_type {
        name: "TSpeechKitRequestProto"
        field {
            name: "GuestData"
            number: 9
            label: LABEL_OPTIONAL
            type: TYPE_MESSAGE
            type_name: ".NAlice.TSpeechKitRequestProto.TGuestData"
            options {
                [NYT.column_name]: "guest_data"
            }
            json_name: "guest_data"
        }
        nested_type {
            name: "TGuestData"
            field {
                name: "UserInfo"
                number: 1
                label: LABEL_OPTIONAL
                type: TYPE_MESSAGE
                type_name: ".NAlice.TBlackBoxUserInfo"
                json_name: "user_info"
            }
            field {
                name: "RawPersonalData"
                number: 2
                label: LABEL_OPTIONAL
                type: TYPE_STRING
                json_name: "raw_personal_data"
            }
            options {
                [NYT.default_field_flags]: SERIALIZATION_YT
            }
        }
        options {
            [NYT.default_field_flags]: SERIALIZATION_YT
        }
    }
}
'''


@pytest.mark.parametrize('old,new,passed,message', [
    (FILE_DESCRIPTOR_SET, CHANGED_COLUMN_NAME, False,
     'alice/wonderlogs/test.proto:Field "1" on message "Message" option'
     ' "column_name" the old value "field" is not equal to the new value "field2"'),

    (FILE_DESCRIPTOR_SET, DELETED_COLUMN_NAME, False,
     'alice/wonderlogs/test.proto:Field "1" on message "Message" option "column_name" has been deleted'),

    (FILE_DESCRIPTOR_SET, FILE_DESCRIPTOR_SET, True, ''),

    (EMPTY, FILE_DESCRIPTOR_SET, True, ''),

    (EMPTY, DELETED_COLUMN_NAME, False,
     'alice/wonderlogs/test.proto:Field "1" on message "Message" option "column_name" must be set'),

    (DELETED_COLUMN_NAME, FILE_DESCRIPTOR_SET, False,
     'alice/wonderlogs/test.proto:Field "1" on message "Message" option "column_name" didn\'t exist'),

    (EMPTY, NOT_SNAKE_CASE1, False,
     'alice/wonderlogs/test.proto:Field "1" on message "Message" option'
     ' "column_name" value "Field" must be in snake case'),

    (EMPTY, NOT_SNAKE_CASE2, False,
     'alice/wonderlogs/test.proto:Field "1" on message "Message" option'
     ' "column_name" value "field_" must be in snake case'),

    (EMPTY, NOT_SNAKE_CASE3, False,
     'alice/wonderlogs/test.proto:Field "1" on message "Message" option'
     ' "column_name" value "fiEld" must be in snake case'),

    (FILE_DESCRIPTOR_SET_ONEOF, CHANGED_COLUMN_NAME_ONEOF, False,
     'alice/wonderlogs/test.proto:Oneof "OneofField" on message "Message" option'
     ' "variant_field_name" the old value "oneof_field" is not equal to the new value "oneof_field2"'),

    (FILE_DESCRIPTOR_SET_ONEOF, DELETED_COLUMN_NAME_ONEOF, False,
     'alice/wonderlogs/test.proto:Oneof "OneofField" on message "Message" option'
     ' "variant_field_name" has been deleted'),

    (FILE_DESCRIPTOR_SET_ONEOF, FILE_DESCRIPTOR_SET_ONEOF, True, ''),

    (EMPTY, FILE_DESCRIPTOR_SET_ONEOF, True, ''),

    (EMPTY, DELETED_COLUMN_NAME_ONEOF, False,
     'alice/wonderlogs/test.proto:Oneof "OneofField" on message "Message" option'
     ' "variant_field_name" must be set'),

    (DELETED_COLUMN_NAME_ONEOF, FILE_DESCRIPTOR_SET_ONEOF, False,
     'alice/wonderlogs/test.proto:Oneof "OneofField" on message "Message" option'
     ' "variant_field_name" didn\'t exist'),

    (EMPTY, NOT_SNAKE_CASE_ONEOF, False,
     'alice/wonderlogs/test.proto:Oneof "OneofField" on message "Message" option'
     ' "variant_field_name" value "oneofField" must be in snake case'),

    (FILE_DESCRIPTOR_SET_COMPOSITE, FILE_DESCRIPTOR_SET_COMPOSITE_CORRUPTED, False,
     'alice/wonderlogs/test.proto:Field "1" on message "Message" option'
     ' "column_name" didn\'t exist\n'
     'alice/wonderlogs/test.proto:Field "2" on message "Message" option'
     ' "column_name" has been deleted\n'
     'alice/wonderlogs/test.proto:Oneof "OneofField" on message "Message"'
     ' option "variant_field_name" the old value "oneof_field" is not equal to the'
     ' new value "oneof_field2"'),

    (EMPTY, FILE_DESCRIPTOR_SET_WITH_CORRECT_MAP, True, ''),

    (EMPTY, FILE_DESCRIPTOR_SET_WITH_INCORRECT_MAP, False,
     'alice/wonderlogs/protos/tmp.proto:Map field "1" on message "TMessage" option'
     ' "flags" must be set MAP_AS_DICT'),

    (FILE_DESCRIPTOR_SET_WITH_INCORRECT_MAP, FILE_DESCRIPTOR_SET_WITH_CORRECT_MAP, False,
     'alice/wonderlogs/protos/tmp.proto:Field "1" on message "TMessage" option "flags"'
     ' the old value "[]" is not equal to the new value "[10]"'),

    (FILE_DESCRIPTOR_SET_WITH_CORRECT_MAP, FILE_DESCRIPTOR_SET_WITH_INCORRECT_MAP, False,
     'alice/wonderlogs/protos/tmp.proto:Field "1" on message "TMessage" option "flags"'
     ' the old value "[10]" is not equal to the new value "[]"'),

    (FILE_DESCRIPTOR_SET_WITH_CORRECT_MAP, FILE_DESCRIPTOR_SET_WITH_CORRECT_MAP, True, ''),

    (FILE_DESCRIPTOR_SET_WITH_INCORRECT_MAP, FILE_DESCRIPTOR_SET_WITH_INCORRECT_MAP, True, ''),

    (FILE_DESCRIPTOR_SET_WITH_NESTED_TYPE, FILE_DESCRIPTOR_SET_WITH_NESTED_TYPE_DIFFERENT_NAME, False,
     'alice/wonderlogs/test.proto:Field "1" on message "TNested" option "column_name"'
     ' the old value "request_id" is not equal to the new value "qweqweqwe"'),

    (FILE_DESCRIPTOR_SET_WITH_NESTED_TYPE, FILE_DESCRIPTOR_SET_WITH_NESTED_TYPE, True, ''),

    (FILE_DESCRIPTOR_SET_WITH_NESTED_TYPE_DIFFERENT_NAME, FILE_DESCRIPTOR_SET_WITH_NESTED_TYPE_DIFFERENT_NAME,
     True, ''),

    (FILE_DESCRIPTOR_SET_WITHOUT_FIELD, FILE_DESCRIPTOR_SET_WITH_FIELD, False,
     'alice/wonderlogs/test.proto:Field "1" on message "TGuestData" option "column_name" must be set\n'
     'alice/wonderlogs/test.proto:Field "2" on message "TGuestData" option "column_name" must be set'),
])
def test_checker(old, new, passed, message):
    old_file_descriptor_set = FileDescriptorSet()
    Parse(old, old_file_descriptor_set)
    new_file_descriptor_set = FileDescriptorSet()
    Parse(new, new_file_descriptor_set)

    assert (passed, message) == compare_file_descriptor_sets(
        old_file_descriptor_set, new_file_descriptor_set)
