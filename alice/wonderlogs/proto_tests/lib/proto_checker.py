import re

from enum import Enum, auto
from google.protobuf import descriptor_pb2
from mapreduce.yt.interface.protos import extension_pb2

SNAKE_CASE = re.compile('^[a-z]+([a-z0-9])*(_[a-z0-9]+)*$')

# MAP_AS_DICT, MAP_AS_OPTIONAL_DICT
MAP_FLAGS = {10, 11}


class CheckStatus(Enum):
    OK = auto()
    APPEARED = auto()
    NOT_EQUAL = auto()
    DELETED = auto()
    NOT_SNAKE_CASE = auto()
    NOT_SET = auto()


def check_fields(old, new, option):
    new_has = new.options.HasExtension(option)
    if old:
        old_has = old.options.HasExtension(option)
        if new_has:
            if not old_has:
                return CheckStatus.APPEARED
            else:
                old_option = old.options.Extensions[option]
                new_option = new.options.Extensions[option]
                if new_option != old_option:
                    return CheckStatus.NOT_EQUAL
        else:
            if old_has:
                return CheckStatus.DELETED
    elif new_has:
        new_option = new.options.Extensions[option]
        if not SNAKE_CASE.match(new_option):
            return CheckStatus.NOT_SNAKE_CASE
    elif not new_has:
        return CheckStatus.NOT_SET
    return CheckStatus.OK


def compare_message(old_message, new_message, proto_name):
    message = {'fields': {}, 'oneofs': {}, 'nested_types': {}}

    if old_message:
        for field in old_message.field:
            message['fields'][field.number] = field

        for oneof in old_message.oneof_decl:
            message['oneofs'][oneof.name] = oneof

        for nested_type in old_message.nested_type:
            message['nested_types'][nested_type.name] = nested_type

    option_column_name = extension_pb2.column_name
    option_variant_field_name = extension_pb2.variant_field_name
    option_flags = extension_pb2.flags

    names = {
        'proto_name': proto_name,
    }

    passed = True
    test_message = ''

    names['option_name'] = option_column_name.name
    for new_field in new_message.field:
        if new_message.options.map_entry:
            continue

        old_field = message.get('fields', {}).get(new_field.number)

        check_status = check_fields(old_field, new_field, option_column_name)

        if check_status == CheckStatus.OK:
            continue

        passed = False

        old_option = old_field.options.Extensions[option_column_name] if old_field else None
        new_option = new_field.options.Extensions[option_column_name]

        names['field_number'] = new_field.number
        names['message_name'] = new_message.name
        names['old_value'] = old_option
        names['new_value'] = new_option

        status_messages = {
            CheckStatus.APPEARED: ('{proto_name}:Field "{field_number}" on message "{message_name}" option'
                                   ' "{option_name}" didn\'t exist\n'),
            CheckStatus.NOT_EQUAL: ('{proto_name}:Field "{field_number}" on message "{message_name}" option'
                                    ' "{option_name}" the old value "{old_value}" is not equal to the'
                                    ' new value "{new_value}"\n'),
            CheckStatus.DELETED: ('{proto_name}:Field "{field_number}" on message "{message_name}" option'
                                  ' "{option_name}" has been deleted\n'),
            CheckStatus.NOT_SNAKE_CASE: ('{proto_name}:Field "{field_number}" on message "{message_name}" option'
                                         ' "{option_name}" value "{new_value}" must be in snake case\n'),
            CheckStatus.NOT_SET: ('{proto_name}:Field "{field_number}" on message "{message_name}" option'
                                  ' "{option_name}" must be set\n')
        }

        test_message += status_messages[check_status].format(**names)

    names['option_name'] = option_variant_field_name.name
    for new_oneof in new_message.oneof_decl:
        old_oneof = message.get('oneofs', {}).get(new_oneof.name)

        check_status = check_fields(old_oneof, new_oneof, option_variant_field_name)

        if check_status == CheckStatus.OK:
            continue

        passed = False

        old_option = old_oneof.options.Extensions[option_variant_field_name] if old_oneof else None
        new_option = new_oneof.options.Extensions[option_variant_field_name]

        names['oneof_name'] = new_oneof.name
        names['message_name'] = new_message.name
        names['old_value'] = old_option
        names['new_value'] = new_option

        status_messages = {
            CheckStatus.APPEARED: ('{proto_name}:Oneof "{oneof_name}" on message "{message_name}" option'
                                   ' "{option_name}" didn\'t exist\n'),
            CheckStatus.NOT_EQUAL: ('{proto_name}:Oneof "{oneof_name}" on message "{message_name}" option'
                                    ' "{option_name}" the old value "{old_value}" is not equal to'
                                    ' the new value "{new_value}"\n'),
            CheckStatus.DELETED: ('{proto_name}:Oneof "{oneof_name}" on message "{message_name}" option'
                                  ' "{option_name}" has been deleted\n'),
            CheckStatus.NOT_SNAKE_CASE: ('{proto_name}:Oneof "{oneof_name}" on message "{message_name}" option'
                                         ' "{option_name}" value "{new_value}" must be in snake case\n'),
            CheckStatus.NOT_SET: ('{proto_name}:Oneof "{oneof_name}" on message "{message_name}" option'
                                  ' "{option_name}" must be set\n')
        }
        test_message += status_messages[check_status].format(**names)

    names['option_name'] = option_flags.name
    for new_field in new_message.field:
        if not new_field.type_name.endswith('.MapEntry'):
            continue

        old_field = message.get('fields', {}).get(new_field.number)

        check_status = CheckStatus.OK
        if not old_field:
            new_options = new_field.options.Extensions[option_flags]
            if not (set(new_options) & MAP_FLAGS):
                check_status = CheckStatus.NOT_SET
            else:
                check_status = CheckStatus.OK

        if check_status == CheckStatus.OK:
            continue

        passed = False

        names['field_number'] = new_field.number
        names['message_name'] = new_message.name

        status_messages = {
            CheckStatus.NOT_SET: ('{proto_name}:Map field "{field_number}" on message "{message_name}" option'
                                  ' "{option_name}" must be set MAP_AS_DICT\n'),
        }

        test_message += status_messages[check_status].format(**names)

    names['option_name'] = option_flags.name
    for new_field in new_message.field:
        old_field = message.get('fields', {}).get(new_field.number)

        check_status = CheckStatus.OK

        if old_field:
            old_option = old_field.options.Extensions[option_flags]
            new_option = new_field.options.Extensions[option_flags]
            if new_option != old_option:
                check_status = CheckStatus.NOT_EQUAL

        if check_status == CheckStatus.OK:
            continue

        passed = False

        old_option = old_field.options.Extensions[option_flags] if old_field else None
        new_option = new_field.options.Extensions[option_flags]

        names['field_number'] = new_field.number
        names['message_name'] = new_message.name
        names['old_value'] = old_option
        names['new_value'] = new_option

        status_messages = {
            CheckStatus.NOT_EQUAL: ('{proto_name}:Field "{field_number}" on message "{message_name}" option'
                                    ' "{option_name}" the old value "{old_value}" is not equal to the'
                                    ' new value "{new_value}"\n'),
        }

        test_message += status_messages[check_status].format(**names)

    for new_nested_type in new_message.nested_type:
        old_nested_type = message.get('nested_types', {}).get(new_nested_type.name)
        nested_passed, nested_test_message = compare_message(old_nested_type, new_nested_type, proto_name)
        passed &= nested_passed
        test_message += nested_test_message

    return passed, test_message


def compare_file_descriptor_proto(old, new):
    messages = {}
    for message in old.message_type:
        messages[message.name] = message

    passed = True
    test_message = ''

    for new_message in new.message_type:
        nested_passed, nested_test_message = compare_message(messages.get(new_message.name), new_message, new.name)
        passed &= nested_passed
        test_message += nested_test_message

    return passed, test_message


def compare_file_descriptor_sets(old_file_descriptor_set, new_file_descriptor_set):
    d = {}
    for proto in old_file_descriptor_set.file:
        d[proto.name] = proto

    passed = True
    test_message = ''
    for proto in new_file_descriptor_set.file:
        cur_passed, cur_test_message = compare_file_descriptor_proto(
            d.get(proto.name) or descriptor_pb2.FileDescriptorProto(), proto)
        passed &= cur_passed
        test_message += cur_test_message

    return passed, test_message.rstrip()
