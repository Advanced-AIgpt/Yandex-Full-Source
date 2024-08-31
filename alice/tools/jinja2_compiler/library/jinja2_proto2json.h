#pragma once

#include <jinja2cpp/value.h>
#include <contrib/libs/jinja2cpp/include/jinja2cpp/template.h>
#include <contrib/libs/jinja2cpp/include/jinja2cpp/template_env.h>

#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/message.h>

#include <util/generic/vector.h>
#include <util/generic/map.h>
#include <util/generic/yexception.h>

namespace NAlice {

struct TName {
    /*
        <-------------- FullName -------------->
        <-- Prefix --><-- Parent --><-- Name -->
                           <Parent1>
        <---------- Root ---------->
    */
    TName() = default;
    TName(const TString& fullName, const TString& prefix, const TString& type)
        : FullName(fullName)
        , Prefix(prefix)
        , Type(type)
    {
        Y_ENSURE(FullName.StartsWith(Prefix), "Invalid name definition");
        const auto pos = fullName.rfind('.');
        if (pos == std::string::npos) {
            Y_ENSURE(Prefix.Empty(), "Invalid name definition");
            Name = FullName;
            return; // Parent and Root will be ""
        }
        Y_ENSURE(Prefix.size() <= pos, "Invalid name definition");
        Name = fullName.substr(pos+1);
        if (Prefix.size() == pos) {
            Parent =  ""; // Prefix.Name (no other words between)
            Parent1 = "";
        } else {
            Parent =  fullName.substr(Prefix.size() + 1, pos - Prefix.size() - 1);
            const auto ppos = Parent.rfind('.');
            if (ppos == std::string::npos) {
                Parent1 = Parent;
            } else {
                Parent1 = Parent.substr(ppos+1);
            }
        }
        Root = fullName.substr(0, pos);
    }
    TString FullName;
    TString Name;
    TString Prefix;
    TString Parent;
    TString Parent1;
    TString Root;
    TString Type;
};

class TJinja2Proto2Json {
public:
    struct TEnumDefinition {
        TName FullName;
        TMap<TString, int> Values;
    };
    struct TOneofDefinition {
        TName FullName;
        const google::protobuf::OneofDescriptor* OneofDescr;
    };
    struct TDescrDefinition {
        TName FullName;
        const google::protobuf::Descriptor* Descr;

        TMap<const google::protobuf::OneofDescriptor*, TOneofDefinition> InternalOneofs;
        TMap<TString, TEnumDefinition> NestedEnums;
        TMap<TString, TDescrDefinition> NestedMessages;
    };

    TJinja2Proto2Json() = default;

    void AddProtobufRoot(const google::protobuf::Descriptor* descr);
    void DebugDump(const TString& var, jinja2::TemplateEnv& env) const;
    void PrepareVars(jinja2::TemplateEnv& env);

private:
    TDescrDefinition& AddDescr(const google::protobuf::Descriptor* descr);

    void PrepareDescr(jinja2::ValuesMap& root, const TDescrDefinition& msgDescr, int depth) const;
    void PrepareEnum(jinja2::ValuesList& root, const TEnumDefinition& enumDescr) const;
    void PrepareField(jinja2::ValuesList& allFields, const google::protobuf::FieldDescriptor* fd, int depth) const;

    void Dump(const TString& name, const jinja2::Value* currentVar, int depth) const;

private:
    TMap<TString, TDescrDefinition> GlobalMessageMap_;
    TMap<TString, TEnumDefinition> GlobalEnumMap_;
};

} // namespace NAlice
