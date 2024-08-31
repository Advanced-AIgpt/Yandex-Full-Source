#pragma once

#include <alice/megamind/library/models/directives/uniproxy_directive_model.h>

#include <google/protobuf/struct.pb.h>

#include <util/generic/variant.h>

namespace NAlice::NMegamind {

enum class EUpdateDatasyncMethod {
    Put /* "PUT" */,
};

class TUpdateDatasyncDirectiveModel final : public TUniproxyDirectiveModel {
public:
    constexpr static TStringBuf SpeechkitName = "update_datasync";

    using TValue = std::variant<TString, google::protobuf::Struct>;

    TUpdateDatasyncDirectiveModel(const TString& key, const TString& value,
                                  EUpdateDatasyncMethod method)
        : TUniproxyDirectiveModel(TString{SpeechkitName})
        , Key(key)
        , Value(value)
        , Method(method)
    {
    }

    TUpdateDatasyncDirectiveModel(const TString& key, const google::protobuf::Struct& value,
                                  EUpdateDatasyncMethod method)
        : TUniproxyDirectiveModel(TString{SpeechkitName})
        , Key(key)
        , Value(value)
        , Method(method)
    {
    }

    void Accept(IModelSerializer& serializer) const final {
        return serializer.Visit(*this);
    }

    [[nodiscard]] const TString& GetKey() const {
        return Key;
    }

    [[nodiscard]] const TValue& GetValue() const {
        return Value;
    }

    [[nodiscard]] EUpdateDatasyncMethod GetMethod() const {
        return Method;
    }

private:
    TString Key;
    TValue Value;
    EUpdateDatasyncMethod Method;
};

} // namespace NAlice::NMegamind
