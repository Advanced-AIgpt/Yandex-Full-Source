#include "jinja2_proto2json.h"

#include "google/protobuf/descriptor.h"
#include "string_convertions.h"

#include <alice/protos/extensions/extensions.pb.h>

#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/message.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/any.pb.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/string/ascii.h>
#include <util/string/split.h>
#include <util/string/printf.h>
#include <util/stream/format.h>
#include <util/string/subst.h>
#include <util/string/join.h>

namespace NAlice {

namespace {

//
// Create complex type from simble (for example "TVector<TString>"" from "TString")
//
jinja2::Value MakeComplexType(const TStringBuf& prefix, const TString& simpleType) {
    TString str(prefix);
    str.append("<");
    str.append(simpleType);
    str.append(">");
    return jinja2::Value(str.c_str());
}

void AddNameCase(jinja2::ValuesMap& valueMap, const TString& name) {
    valueMap["name"] = jinja2::Value(name.c_str());
    valueMap["name_lowercase"] = jinja2::Value(ConvertString(name, EStringDetector::LOWER_CASE).c_str());
    valueMap["name_lcamelcase"] = jinja2::Value(ConvertString(name, EStringDetector::LOWER_CAMEL_CASE).c_str());
    valueMap["name_ucamelcase"] = jinja2::Value(ConvertString(name, EStringDetector::UPPER_CAMEL_CASE).c_str());
    valueMap["name_uppercase"] = jinja2::Value(ConvertString(name, EStringDetector::UPPER_CASE).c_str());
}

/*
    Add all possible names to any object (message, enu, etc)
    All names has following format:
        <object>.name - name "as is"
        <object>.name_lowercase
        <object>.name_lcamelcase
        <object>.name_ucamelcase
        <object>.name_uppercase
        <object>.proto.name                             TMyClass (the same as name)
        <object>.proto.namespace                        NNamespace1.NNamespace2
        <object>.proto.parent_name1                     TMyParentClass
        <object>.proto.parent_name                      TBaseClass1.TBaseClass2.TMyParentClass
        <object>.proto.full_name                        NNamespace1.NNamespace2.TBaseClass1.TBaseClass2.TMyParentClass.TMyClass
        <object>.proto.root_name                        NNamespace1.NNamespace2.TBaseClass1.TBaseClass2.TMyParentClass
        <object>.proto.type                             string, int32, enum, message, ...
        <object>.cpp.name                               TMyClass (the same as name)
        <object>.cpp.namespace                          NNamespace1::NNamespace2
        <object>.cpp.parent_name1                       TMyParentClass
        <object>.cpp.parent_name                        TBaseClass1::TBaseClass2::TMyParentClass
        <object>.cpp.full_name                          NNamespace1::NNamespace2 TBaseClass1::TBaseClass2::TMyParentClass::TMyClass
        <object>.cpp.type_base                          TString, i32, message/enum name
        <object>.cpp.type_full                          i32, TMaybe<i32>, TVector<i32>, ...
*/
void AddAllNames(jinja2::ValuesMap& valueMap, const TName& name) {
    AddNameCase(valueMap, name.Name);
    jinja2::ValuesMap valueMapProto;
    valueMapProto["name"] = jinja2::Value(name.Name.c_str());
    valueMapProto["namespace"] = jinja2::Value(name.Prefix.c_str());
    valueMapProto["parent_name1"] = jinja2::Value(name.Parent1.c_str());
    valueMapProto["parent_name"] = jinja2::Value(name.Parent.c_str());
    valueMapProto["root_name"] = jinja2::Value(name.Root.c_str());
    valueMapProto["full_name"] = jinja2::Value(name.FullName.c_str());
    valueMapProto["type_base"] = jinja2::Value(name.Type.c_str());
    valueMapProto["type_full"] = jinja2::Value(name.Type.c_str());
    valueMap["proto"] = valueMapProto;

    jinja2::ValuesMap valueMapCpp;
    TName nameCopy = name;
    SubstGlobal(nameCopy.Prefix, ".", "::");
    SubstGlobal(nameCopy.Parent, ".", "::");
    SubstGlobal(nameCopy.Root, ".", "::");
    TString FullNameCpp = Join("::", nameCopy.Prefix, nameCopy.Parent, nameCopy.Name);

    valueMapCpp["name"] = jinja2::Value(name.Name.c_str());
    valueMapCpp["namespace"] = jinja2::Value(nameCopy.Prefix.c_str());
    valueMapCpp["parent_name1"] = jinja2::Value(nameCopy.Parent1.c_str());
    valueMapCpp["parent_name"] = jinja2::Value(nameCopy.Parent.c_str());
    valueMapCpp["full_name"] = jinja2::Value(FullNameCpp.c_str());
    valueMapCpp["type_base"] = jinja2::Value("");
    valueMapCpp["type_full"] = jinja2::Value("");
        //<object>.cpp.type_base                          TString, i32, message/enum name
        //<object>.cpp.type_full                          i32, TMaybe<i32>, TVector<i32>, ...
    valueMap["cpp"] = valueMapCpp;
}

//
// Extract value from field using reflection
// This function used to handle all [options] for fields in proto files and set key-values data in jinja2::ValuesMap
//
template <class T>
bool ExtractValueFromField(jinja2::ValuesMap& value, const google::protobuf::FieldDescriptor* fd, const T& descrOptions) {
    const google::protobuf::Reflection* reflection = descrOptions.GetReflection();

    switch (fd->type()) {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
            value[fd->name().c_str()] = jinja2::Value(reflection->GetFloat(descrOptions, fd));
            break;
        case google::protobuf::FieldDescriptor::TYPE_INT64:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64:
            value[fd->name().c_str()] = jinja2::Value(reflection->GetInt64(descrOptions, fd));
            break;
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
            value[fd->name().c_str()] = jinja2::Value(static_cast<i64>(reflection->GetUInt64(descrOptions, fd)));
            break;
        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32:
            value[fd->name().c_str()] = jinja2::Value(reflection->GetInt32(descrOptions, fd));
            break;
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
            value[fd->name().c_str()] = jinja2::Value(reflection->GetUInt32(descrOptions, fd));
            break;
        case google::protobuf::FieldDescriptor::TYPE_BOOL:
            value[fd->name().c_str()] = jinja2::Value(reflection->GetBool(descrOptions, fd));
            break;
        case google::protobuf::FieldDescriptor::TYPE_STRING:
            value[fd->name().c_str()] = jinja2::Value(reflection->GetString(descrOptions, fd).c_str());
            break;
        case google::protobuf::FieldDescriptor::TYPE_ENUM:
            if (const google::protobuf::EnumValueDescriptor* enumDescr = reflection->GetEnum(descrOptions, fd)) {
                value[fd->name().c_str()] = jinja2::Value(enumDescr->name().c_str());
                break;
            }
            // No valid enum descriptor found
            value[fd->name().c_str()] = jinja2::Value();
            return false;
        default:
            return false;
    }
    return true;
}

template <class T>
bool ExtractRepeatedValueFromField(jinja2::ValuesMap& value, const google::protobuf::FieldDescriptor* fd, const T& descrOptions) {
    const google::protobuf::Reflection* reflection = descrOptions.GetReflection();
    int count = reflection->FieldSize(descrOptions, fd);

    jinja2::ValuesList listOfElements;
    for (int i = 0; i < count; i++) {
        switch (fd->type()) {
            case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
            case google::protobuf::FieldDescriptor::TYPE_FLOAT:
                listOfElements.push_back(jinja2::Value(reflection->GetRepeatedFloat(descrOptions, fd, i)));
                break;
            case google::protobuf::FieldDescriptor::TYPE_INT64:
            case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
            case google::protobuf::FieldDescriptor::TYPE_SINT64:
                listOfElements.push_back(jinja2::Value(reflection->GetRepeatedInt64(descrOptions, fd, i)));
                break;
            case google::protobuf::FieldDescriptor::TYPE_UINT64:
                listOfElements.push_back(jinja2::Value(static_cast<i64>(reflection->GetRepeatedUInt64(descrOptions, fd, i))));
                break;
            case google::protobuf::FieldDescriptor::TYPE_INT32:
            case google::protobuf::FieldDescriptor::TYPE_FIXED64:
            case google::protobuf::FieldDescriptor::TYPE_FIXED32:
            case google::protobuf::FieldDescriptor::TYPE_SINT32:
                listOfElements.push_back(jinja2::Value(reflection->GetRepeatedInt32(descrOptions, fd, i)));
                break;
            case google::protobuf::FieldDescriptor::TYPE_UINT32:
                listOfElements.push_back(jinja2::Value(reflection->GetRepeatedUInt32(descrOptions, fd, i)));
                break;
            case google::protobuf::FieldDescriptor::TYPE_BOOL:
                listOfElements.push_back(jinja2::Value(reflection->GetRepeatedBool(descrOptions, fd, i)));
                break;
            case google::protobuf::FieldDescriptor::TYPE_STRING:
                listOfElements.push_back(jinja2::Value(reflection->GetRepeatedString(descrOptions, fd, i).c_str()));
                break;
            case google::protobuf::FieldDescriptor::TYPE_ENUM:
                if (const google::protobuf::EnumValueDescriptor* enumDescr = reflection->GetRepeatedEnum(descrOptions, fd, i)) {
                    listOfElements.push_back(jinja2::Value(enumDescr->name().c_str()));
                    break;
                }
                // No valid enum descriptor found
                // don't break
            default:
                value[fd->name().c_str()] = listOfElements;
                return false;
        }
    }
    value[fd->name().c_str()] = listOfElements;
    return true;
}

template <class T>
void AddOptions(jinja2::ValuesMap& rootValueMap, const T& descrOptions) {
    jinja2::ValuesMap optionsMap;
    const google::protobuf::Reflection* reflection = descrOptions.GetReflection();
    std::vector<const google::protobuf::FieldDescriptor*> allOptions;
    reflection->ListFields(descrOptions, &allOptions);

    for (const auto& it : allOptions) {
        if (it->type() == google::protobuf::FieldDescriptor::TYPE_UINT64) {
            Cout << "Warning: Option type for option "<< it->name() << " has type uint64 (unsupported), convert it to int64: " << Endl;
        }
        if (it->is_repeated()) {
            if (!ExtractRepeatedValueFromField(optionsMap, it, descrOptions)) {
                Cout << "Warning: Repeated option type "<< it->name() << " it not supported: " << static_cast<int>(it->type()) << ". Ignored" << Endl;
            }
        } else {
            if (!ExtractValueFromField(optionsMap, it, descrOptions)) {
                Cout << "Warning: Option type for option "<< it->name() << " it not supported: " << static_cast<int>(it->type()) << ". Ignored" << Endl;
            }
        }
    }
    rootValueMap["options"] = optionsMap;
}


} // anonimous namespace

/*
    Scan and add all required root messages and child basic objects (enums, submessages, oneof)
*/
void TJinja2Proto2Json::AddProtobufRoot(const google::protobuf::Descriptor* descr) {
    AddDescr(descr);

    // Check all classes
    for (auto& it : GlobalMessageMap_) {
        if (!it.second.FullName.Parent.Empty()) {
            const google::protobuf::Descriptor* d = descr->file()->pool()->FindMessageTypeByName(it.second.FullName.Root);
            Y_ENSURE(d, "Class  definition without parent message. Root: " << it.second.FullName.Root);
            AddDescr(d);
        }
    }
    // Attach all nested classes
    for (auto& it : GlobalMessageMap_) {
        if (!it.second.FullName.Parent.Empty()) {
            auto itParent = GlobalMessageMap_.find(it.second.FullName.Root);
            Y_ENSURE(itParent != GlobalMessageMap_.end(), "Undefined parent class " << it.second.FullName.FullName);
            itParent->second.NestedMessages[it.second.FullName.FullName] = it.second;
        }
    }
}

TJinja2Proto2Json::TDescrDefinition& TJinja2Proto2Json::AddDescr(const google::protobuf::Descriptor* descr) {
    const TString fullName = descr->full_name().c_str();
    auto it = GlobalMessageMap_.find(fullName);
    if (it != GlobalMessageMap_.end()) {
        // This message already exists in global map
        return it->second;
    }

    TDescrDefinition& descrDefinition = GlobalMessageMap_[fullName];
    descrDefinition.FullName = TName(fullName, descr->file()->package(), "message");
    descrDefinition.Descr = descr;

    // Check that this is a standard descriptor (struct, any, etc)
    if (fullName.StartsWith("google.protobuf")) {
        // Don't need to process inside
        return descrDefinition;
    }

    // Add nested objects
    // Enum
    for (int i = 0; i < descr->field_count(); i++) {
        const google::protobuf::FieldDescriptor* field = descr->field(i);
        if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_ENUM) {
            const google::protobuf::EnumDescriptor* enumDescr = field->enum_type();
            if (!GlobalEnumMap_.contains(enumDescr->full_name())) {
                TEnumDefinition enumDef;
                enumDef.FullName = TName(enumDescr->full_name(), enumDescr->file()->package(), "enum");
                for (int j = 0; j < enumDescr->value_count(); j++) {
                    const google::protobuf::EnumValueDescriptor* evd = enumDescr->value(j);
                    enumDef.Values[evd->name().c_str()] = evd->number();
                }
                // Insert into either descriptor or global list
                if (enumDef.FullName.Parent.Empty()) {
                    GlobalEnumMap_[enumDef.FullName.FullName] = enumDef;
                } else {
                    const google::protobuf::Descriptor* d = descr->file()->pool()->FindMessageTypeByName(enumDef.FullName.Root);
                    Y_ENSURE(d, "Enum definition without parent message. Root: " << enumDef.FullName.Root);
                    auto& descr = AddDescr(d);
                    descr.NestedEnums[enumDef.FullName.FullName] = enumDef;
                }
            }
        }
    }

    // Oneof
    for (int i = 0; i < descr->field_count(); i++) {
        const google::protobuf::FieldDescriptor* field = descr->field(i);
        const google::protobuf::OneofDescriptor* oneofDescr = field->containing_oneof();
        if (oneofDescr && !oneofDescr->is_synthetic()) {
            // synthetic oneofs (optional keyword) will be mapped into TMaybe
            if (!descrDefinition.InternalOneofs.contains(oneofDescr)) {
                TOneofDefinition oneof;
                oneof.FullName = TName(oneofDescr->full_name(), oneofDescr->file()->package(), "oneof");
                oneof.OneofDescr = oneofDescr;
                descrDefinition.InternalOneofs[oneofDescr] = oneof;
            }
        }
    }

    // Child messages (only for user-defined classes)
    for (int i = 0; i < descr->field_count(); i++) {
        const google::protobuf::FieldDescriptor* field = descr->field(i);
        if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
            AddDescr(field->message_type());
        }
    }

    // Other local declarated classes
    for (int i = 0; i < descr->nested_type_count(); i++) {
        AddDescr(descr->nested_type(i));
    }
    return descrDefinition;
}

/*
    Dump variable and its content
    Source variabe must have format Var1.Subvar2. ...
    Array objects can be addresses with Var1.* or Var.0, etc
*/
void TJinja2Proto2Json::DebugDump(const TString& var, jinja2::TemplateEnv& env) const {
    TVector<TString> splittedVars;
    Split(var.c_str(), ".", splittedVars);
    const jinja2::Value* currentVar = nullptr;
    bool lookupInGlobals = true;

    // vars array has at least one value, so we can start from index #1
    for (const auto& it: splittedVars) {
        if (lookupInGlobals) {
            // Step 1. Looking for a global variable
            auto fn = [&](jinja2::ValuesMap& globalVar) mutable {
                for (auto& it : globalVar) {
                    if (splittedVars[0] == it.first) {
                        currentVar = &it.second;
                        break;
                    }
                }
            };
            env.ApplyGlobals(fn);
            lookupInGlobals = false;
            if (currentVar == nullptr) {
                Cout << "Can not dump: can not find a variable '" << it << "'" << Endl;
                return;
            }
            continue;
        }
        if (currentVar == nullptr) {
            // Nothing to dump
            return;
        }
        // Find a required subelement
        if (it == "" || it == "*") {
            // dump all subvalues of currentVar
            break;
        }
        if (IsAsciiDigit(it[0])) {
            // currentVar should be a list and we want to dump required element
            if (!currentVar->isList()) {
                Cout << "Can not dump: current variable doesn't contains list info" << Endl;
                return;
            }
            size_t order = FromString(it);
            jinja2::ValuesList& list = currentVar->asList();
            if (order >= list.size()) {
                Cout << "Can not dump: index is out of range" << Endl;
                return;
            }
            currentVar = &list[order];
            continue;
        }
        // All other cases: currentVar should be a map
        if (!currentVar->isMap()) {
            Cout << "Can not dump: current variable is not a map" << Endl;
            return;
        }
        const auto& nextVar = currentVar->asMap().find(it);
        if (nextVar == currentVar->asMap().end()) {
            Cout << "Can not dump: can not find " << it << Endl;
            return;
        }
        currentVar = &nextVar->second;
    }
    // Dump currentVar and all childs
    Dump(var, currentVar, 0);
}

void TJinja2Proto2Json::PrepareVars(jinja2::TemplateEnv& env) {
    {
        // Add all descriptions
        jinja2::ValuesMap valueMap;
        for (const auto& itMsg : GlobalMessageMap_) {
            if (itMsg.second.FullName.Parent.Empty()) {
                jinja2::ValuesMap rootDescr;
                PrepareDescr(rootDescr, itMsg.second, 0);
                valueMap[itMsg.second.FullName.Name] = rootDescr;
            }
        }
        env.AddGlobal("messages", valueMap);
    }
    {
        // Add all global enums
        jinja2::ValuesList valueMap;
        for (const auto& itEnum : GlobalEnumMap_) {
            PrepareEnum(valueMap, itEnum.second);
        }
        env.AddGlobal("enums", valueMap);
    }
}

/*
*/
void TJinja2Proto2Json::PrepareDescr(jinja2::ValuesMap& root, const TDescrDefinition& msgDescr, int depth) const {
    if (depth > 3) {
        return;
    }
    AddAllNames(root, msgDescr.FullName);

    if (msgDescr.FullName.Prefix == "google.protobuf") {
        return;
    }

    // Add all child messages
    jinja2::ValuesList childMessages;
    for (const auto& it : msgDescr.NestedMessages) {
        jinja2::ValuesMap childMsgRoot;
        PrepareDescr(childMsgRoot, it.second, depth+1);
        childMessages.push_back(childMsgRoot);
    }
    root["messages"] = childMessages;

    // Add all child enums
    jinja2::ValuesList childEnums;
    for (const auto& it : msgDescr.NestedEnums) {
        PrepareEnum(childEnums, it.second);
    }
    root["enums"] = childEnums;
    AddOptions(root, msgDescr.Descr->options());

    // Add all fields (non included into oneof)
    jinja2::ValuesList allFields;
    for (auto i = 0; i<msgDescr.Descr->field_count(); i++) {
        const auto* fd = msgDescr.Descr->field(i);
        const google::protobuf::OneofDescriptor* oneofDescr = fd->containing_oneof();
        if (oneofDescr && !oneofDescr->is_synthetic()) {
            // Will be handled in another for loop
            continue;
        }
        PrepareField(allFields, fd, depth);
    }
    root["fields"] = allFields;

    // Add all fileds (from oneofs)
    jinja2::ValuesList oneofRoot;
    for (const auto& it: msgDescr.InternalOneofs) {
        jinja2::ValuesMap childOneof;
        childOneof["name"] = it.second.FullName.Name.c_str();
        TString type = it.second.FullName.Name;
        type.append("Oneof");
        childOneof["type"] = type.c_str();

        jinja2::ValuesList allFields;
        for (auto i = 0; i<msgDescr.Descr->field_count(); i++) {
            const auto* fd = msgDescr.Descr->field(i);
            const google::protobuf::OneofDescriptor* oneofDescr = fd->containing_oneof();
            if (oneofDescr == it.first) {
                // Add these fields into oneof declaration
                PrepareField(allFields, fd, depth);
            }
        }
        childOneof["fields"] = allFields;
        oneofRoot.push_back(childOneof);
    }
    root["field_count"] = msgDescr.Descr->field_count();
    root["oneofs"] = oneofRoot;
}

/*
*/
void TJinja2Proto2Json::PrepareEnum(jinja2::ValuesList& root, const TEnumDefinition& enumDescr) const {
    jinja2::ValuesMap valueEnum;
    AddAllNames(valueEnum, enumDescr.FullName);
    jinja2::ValuesList valuesInEnum;
    for (const auto& it : enumDescr.Values) {
        jinja2::ValuesMap singleEnum;
        AddNameCase(singleEnum, it.first.c_str());
        singleEnum["value"] = it.second;
        valuesInEnum.push_back(singleEnum);
    }
    valueEnum["values"] = valuesInEnum;
    root.push_back(valueEnum);
}

void TJinja2Proto2Json::PrepareField(jinja2::ValuesList& allFields, const google::protobuf::FieldDescriptor* fd, int depth) const {
    jinja2::ValuesMap fieldMap;
    AddNameCase(fieldMap, fd->name());

    // Add options
    fieldMap["number"] = fd->number();
    fieldMap["is_optional"] = fd->has_optional_keyword();
    fieldMap["is_repeated"] = fd->is_repeated();

    // Add a type
    // Type of field can be represented as
    // field.type_proto - field type "as is"  (int32, string, message)
    // field.type_cpp.base - the same field type accepted for C++ (i32, TString, TClassName)
    //                       note `TClassName` will also contains namespaces and parent classes
    // field.type_cpp.full - the same but also contains additional wrappers (TVector<i32>, TMaybe<TString>, etc)
    fieldMap["type_proto"] = fd->type_name();
    jinja2::ValuesMap typeCppMap;
    TString simpeType;
    bool standardGoogleType = false;
    switch (fd->cpp_type()) {
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
            simpeType = "i32";
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
            simpeType = "i64";
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
            simpeType = "ui32";
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
            simpeType = "ui64";
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
            simpeType = fd->cpp_type_name();
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
            simpeType = fd->enum_type()->name().c_str();
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
            simpeType = "TString";
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
            simpeType = fd->message_type()->name().c_str();
            if (simpeType == "Struct") {
                simpeType = "google::protobuf::Struct";
                standardGoogleType = true;
            } else if (simpeType == "Any") {
                simpeType = "google::protobuf::Any";
                standardGoogleType = true;
            }
            break;
    }
    typeCppMap["base"] = jinja2::Value(simpeType.c_str());
    if (fd->has_optional_keyword()) {
        typeCppMap["full"] = MakeComplexType("TMaybe", simpeType);
    } else if (fd->is_repeated()) {
        typeCppMap["full"] = MakeComplexType("TVector", simpeType);
    } else {
        typeCppMap["full"] = jinja2::Value(simpeType.c_str());
    }
    fieldMap["type_cpp"] = typeCppMap;

    // Add custom data
    AddOptions(fieldMap, fd->options());

    // Add comments from source string
    google::protobuf::DebugStringOptions dso;
    dso.include_comments = true;
    dso.elide_group_body = true;
    dso.elide_oneof_body = true;
    const TString fullStrings = fd->DebugStringWithOptions(dso);
    TVector<TString> fullStringsLines;
    Split(fullStrings, "\n", fullStringsLines);

    jinja2::ValuesList valueListComments;
    for (const auto& it : fullStringsLines) {
        const auto pos = it.find("// ");
        if (pos != TString::npos) {
            valueListComments.push_back(jinja2::Value{it.c_str() + pos});
        }
    }
    fieldMap["comments"] = valueListComments;

    // For nested messages: add info from child descriptor
    if (fd->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE && !standardGoogleType) {
        TString fullRefName = fd->message_type()->full_name();
        jinja2::ValuesMap otherMsg;

        auto it = GlobalMessageMap_.find(fullRefName);
        if (it == GlobalMessageMap_.end()) {
            Y_ENSURE(false, "External reference to " << fullRefName << " not found. Ref " << fd->name());
        }
        PrepareDescr(otherMsg, it->second, depth+1);
        fieldMap["message"] = otherMsg;
    }
    allFields.push_back(fieldMap);
}

/*
*/
void TJinja2Proto2Json::Dump(const TString& name, const jinja2::Value* currentVar, int depth) const {
    for (int i = 0; i < depth; i++) {
        Cout << " ";
    }
    Cout << name;
    if (currentVar->isList()) {
        Cout << ": list" << Endl;
        int counter = 0;
        for (const auto& it : currentVar->asList()) {
            TString substring = Sprintf("[%i]", counter++);
            Dump(substring, &it, depth+1);
        }
        return;
    }
    if (currentVar->isMap()) {
        Cout << ": map" << Endl;
        for (const auto& it : currentVar->asMap()) {
            Dump(it.first.c_str(), &it.second, depth+1);
        }
        return;
    }
    if (currentVar->isString()) {
        Cout << ": " << currentVar->asString().c_str() << Endl;
        return;
    }
    const auto var = currentVar->data();
    if (std::get_if<int64_t>(&var) != nullptr) {
        Cout << ": int64: " << std::get<int64_t>(var) << Endl;
        return;
    }
    Cout << ": unknown" << Endl;
}

} // namespace NAlice
