#include "constructor.h"

#include <alice/library/json/json.h>
#include <alice/library/proto/protobuf.h>

#include <util/charset/utf8.h>

#include <utility>

namespace NAlice::NMegamindApi {

TResponseConstructor::TResponseConstructor(NJson::TJsonValue&& response)
    : Response(std::move(response))
{
}

void TResponseConstructor::PushSpeechKitProto(const TSpeechKitResponseProto& proto) {
    Y_ASSERT(!DoesProtoHave64BitNumber<TSpeechKitResponseProto_TResponse>());
    Response = JsonFromProto(proto);
}

void TResponseConstructor::NormalizeResponse() {
    if (!Response["response"]["experiments"].IsDefined()) {
        Response["response"]["experiments"] = NJson::TJsonValue{NJson::JSON_MAP};
    }

    if (!Response["response"].Has("directives")) {
        Response["response"]["directives"] = NJson::TJsonValue{NJson::JSON_ARRAY};
    }

    // We should replace empty string with null in DialogId
    // See https://wiki.yandex-team.ru/Alice/VINS/speechkitapi/#protocol for more info.
    if (!Response.GetValueByPath("header.dialog_id") || Response["header"]["dialog_id"] == "") {
        Response["header"]["dialog_id"] = NJson::TJsonValue(NJson::JSON_NULL);
    }

    if (Response["response"].Has("cards")) {
        // https://st.yandex-team.ru/ALICE-4537
        if (Response["response"]["cards"].IsArray() && !Response["response"]["cards"].GetArray().empty()) {
            Response["response"]["card"] = Response["response"]["cards"][0];
        }
    } else {
        // See: https://st.yandex-team.ru/ALICE-5949
        Response["response"]["cards"] = NJson::TJsonValue{NJson::JSON_ARRAY};
        Response["response"]["card"] = NJson::TJsonValue{NJson::JSON_NULL};
    }
}

NJson::TJsonValue TResponseConstructor::MakeResponse() && {
    NormalizeResponse();
    return std::move(Response);
}

} // namespace NAlice::NMegamindApi
