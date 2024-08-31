#pragma once

#include "uniproxy_directive_model.h"

#include <utility>

#include <util/generic/string.h>

#include <google/protobuf/struct.pb.h>

namespace NAlice::NMegamind {

class TUniversalUniproxyDirectiveModel final : public virtual TUniproxyDirectiveModel {
public:
    TUniversalUniproxyDirectiveModel(const TString& name, google::protobuf::Struct payload)
        : TUniproxyDirectiveModel(name)
        , Payload(std::move(payload)) {
    }

    void Accept(IModelSerializer& serializer) const final {
        serializer.Visit(*this);
    }

    [[nodiscard]] const google::protobuf::Struct& GetPayload() const {
        return Payload;
    }

private:
    google::protobuf::Struct Payload;
};

} // namespace NAlice::NMegamind
