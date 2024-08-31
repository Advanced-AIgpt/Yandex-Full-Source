#include "proto_struct.h"

#include <util/string/cast.h>

namespace NAlice {

namespace {

const google::protobuf::Struct EmptyStruct;
const google::protobuf::ListValue EmptyList;

} // anonymous namespace

/*
    Internal function
    Get a value for the current struct ({})
    This function work with single key value only!
*/
const google::protobuf::Value* TProtoStructParser::GetChildValue(const google::protobuf::Struct* proto, TString singleKey) const {
    if (proto == nullptr) {
        Trace(false, TStringBuilder{} << "Can't have a child value because proto is null ptr");
        return nullptr;
    }
    const auto& fields = proto->fields();
    const auto& it = fields.find(singleKey);
    if (it == fields.end()) {
        if (IsTraceNeed()) {
            // Prepare data for trace reporting
            TStringBuilder tsb;
            int count = 0;
            tsb << "Child value '" << singleKey << "' not found. Nearest values: ";
            for (const auto& f : fields) {
                tsb << f.first << " ";
                if (count++ > 10) {
                    break;
                }
            }
            Trace(false, tsb);
        }
        return nullptr;
     }
     return &it->second;
}

/*
    Internal function
    Get a value for the current list ([])
    This function work with single key value only!
*/
const google::protobuf::Value* TProtoStructParser::GetChildValue(const google::protobuf::ListValue* list, TString singleKey) const {
    int index;
    if (list == nullptr || !TryFromString(singleKey, index)) {
        Trace(false, TStringBuilder{} << "Can't have a child value because either list is null ptr or index is not a number: " << singleKey);
        return nullptr;
    }
    if (index < 0 || index >= list->values().size()) {
        return nullptr;
    }
    return &(list->values()[index]);
}

/*
    Extract final value from full path
    For example: "Key1.Key2.1.Key3.Value"
    First part (Key1.Key2.1.Key3) will be extracted from Struct/ListValue object
    Last part (Key3) will be extracted as a Value object
*/
const google::protobuf::Value* TProtoStructParser::GetFinalValue(const google::protobuf::Struct& proto, TString path) const {
    TVector<TString> paths;
    TString lastKey = SplitPath(path, paths);
    if (lastKey.Empty()) {
        return nullptr;
    }
    const auto* subkey = GetSubKey(&proto, paths);
    if (subkey == nullptr) {
        return nullptr;
    }
    return GetChildValue(subkey, lastKey);
}

/*
    Get final value as a string
*/
TMaybe<TString> TProtoStructParser::GetValueString(const google::protobuf::Struct& proto, TString path) const {
    const auto* value = GetFinalValue(proto, path);
    if (value && value->has_string_value()) {
        return value->string_value();
    }
    return Nothing();
}

/*
    Get final value as an integer
    Note google::protobuf::Struct has only 'number' type (the same for both double and int)
*/
TMaybe<int> TProtoStructParser::GetValueInt(const google::protobuf::Struct& proto, TString path) const {
    TMaybe<double> value = GetValueDouble(proto, path);
    if (!value.Defined()) {
        return Nothing();
    }
    return static_cast<int>(*value);
}

/*
    Get final value as a double
*/
TMaybe<double> TProtoStructParser::GetValueDouble(const google::protobuf::Struct& proto, TString path) const {
    const auto* value = GetFinalValue(proto, path);
    if (value && value->has_number_value()) {
        return value->number_value();
    }
    return Nothing();
}

/*
    Get final value as a boolean
*/
TMaybe<bool> TProtoStructParser::GetValueBool(const google::protobuf::Struct& proto, TString path) const {
    const auto* value = GetFinalValue(proto, path);
    if (value && value->has_bool_value()) {
        return value->bool_value();
    }
    return Nothing();
}

/*
    Test selected key and return one of possible result
    See TProtoStructParser::EResult enum declaration
*/
TProtoStructParser::EResult TProtoStructParser::TestKey(const google::protobuf::Struct& proto, TString path) const {
    TVector<TString> paths;
    TString lastKey = SplitPath(path, paths);
    if (lastKey.Empty()) {
        return EResult::Absent;
    }
    const auto* subkey = GetSubKey(&proto, paths);
    if (subkey == nullptr) {
        return EResult::Absent;
    }
    const auto* value = GetChildValue(subkey, lastKey);
    if (value == nullptr) {
        return EResult::Absent;
    }
    if (value->has_struct_value()) {
        return EResult::Map;
    }
    if (value->has_list_value()) {
        return EResult::List;
    }
    if (value->has_string_value()) {
        return EResult::String;
    }
    if (value->has_number_value()) {
        return EResult::Numeric;
    }
    if (value->has_bool_value()) {
        return EResult::Boolean;
    }
    if (value->has_null_value()) {
        return EResult::Null;
    }
    return EResult::Absent;
}

/*
    Get a subkey of given path (result must be a map)
*/
const google::protobuf::Struct& TProtoStructParser::GetKey(const google::protobuf::Struct& proto, TString path) const {
    TVector<TString> paths;
    Split(path, Delimiter_, paths);
    if (paths.empty()) {
        return proto;
    }
    const auto* res = GetSubKey(&proto, paths);
    return res ? *res : EmptyStruct;
}

/*
    Get a subkey of given path (result must be a list)
*/
const google::protobuf::ListValue& TProtoStructParser::GetArray(const google::protobuf::Struct& proto, TString path) const {
    TVector<TString> paths;
    TString lastKey = SplitPath(path, paths);
    if (lastKey.Empty()) {
        return EmptyList;
    }
    const auto* subkey = GetSubKey(&proto, paths);
    if (subkey == nullptr) {
        return EmptyList;
    }
    const auto* value = GetChildValue(subkey, lastKey);
    if (value && value->has_list_value()) {
        return value->list_value();
    }
    return EmptyList;
}

/*
    Helpers for trace and error messages
*/
void TProtoStructParser::Trace(bool errorFlag, const TString& message) const {
    if (Logger_ != nullptr) {
        LOG(*Logger_, errorFlag ? LevelForError_ : LevelForTrace_) << message;
    }
    if (errorFlag && ThrowOnError_) {
        Y_ENSURE(false, message);
    }
}

/*
    Split mulpitle path into array of strings and extract last key
    checkNotList - true, if source path must NOT contain array sequence ([])
*/
TString TProtoStructParser::SplitPath(TString path, TVector<TString>& paths) const {
    Split(path, Delimiter_, paths);
    if (paths.empty()) {
        return {};
    }
    // Ensure that the source path doesn't contain array sequence
    for (const auto& it: paths) {
        Y_ENSURE(it != ArrayFlag_, "The path can not contain array sequence ([])");
    }
    TString lastKey = paths.back();
    paths.pop_back();
    return lastKey;
}

/*
    Get recurse subkey
*/
const google::protobuf::Struct* TProtoStructParser::GetSubKey(const google::protobuf::Struct* proto, const TVector<TString> paths) const {
    if (proto == nullptr) {
        return nullptr;
    }
    // Either proto or list must be active
    const google::protobuf::ListValue* list = nullptr;
    for (const auto& p : paths) {
        const auto* value = proto ? GetChildValue(proto, p) : GetChildValue(list, p);
        if (value == nullptr) {
            Trace(false, TStringBuilder{} << "Can't find child key " << p);
            return nullptr;
        }
        if (value->has_struct_value()) {
            proto = &value->struct_value();
            list = nullptr;
        } else if (value->has_list_value()) {
            proto = nullptr;
            list = &value->list_value();
        }
    }
    return proto;
}

} // namespace NAlice
