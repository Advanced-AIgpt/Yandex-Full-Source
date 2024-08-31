#pragma once

#include <alice/megamind/protos/analytics/megamind_analytics_info.pb.h>
#include <alice/megamind/protos/div/div2_cards.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/iterator_range.h>

#include <google/protobuf/repeated_field.h>

namespace NAlice::NMegamindApi {

class TResponseConstructor final {
public:
    template <typename T>
    using TConstRepeatedFieldRange = TIteratorRange<typename google::protobuf::RepeatedPtrField<T>::const_iterator>;

public:
    TResponseConstructor() = default;
    explicit TResponseConstructor(NJson::TJsonValue&& response);

    void PushSpeechKitProto(const TSpeechKitResponseProto& proto);

    NJson::TJsonValue MakeResponse() &&;

private:
    void NormalizeResponse();

private:
    NJson::TJsonValue Response;
};

} // namespace NAlice::NMegamindApi
