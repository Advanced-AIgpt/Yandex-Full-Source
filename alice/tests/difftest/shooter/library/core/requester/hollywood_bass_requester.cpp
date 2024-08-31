#include "hollywood_bass_requester.h"

#include <alice/joker/library/log/log.h>
#include <alice/tests/difftest/shooter/library/core/util.h>
#include <alice/tests/difftest/shooter/library/core/engine.h>

#include <library/cpp/json/json_writer.h>
#include <library/cpp/neh/http_common.h>
#include <library/cpp/neh/neh.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/uri/uri.h>

#include <util/generic/queue.h>
#include <util/string/builder.h>
#include <util/string/strip.h>

namespace NAlice::NShooter {

THollywoodBassRequester::THollywoodBassRequester(const IContext& ctx, const IEngine& engine)
    : Ctx_{ctx}
    , Engine_{engine}
{
}

TMaybe<TRequestResponse> THollywoodBassRequester::Request(const TFsPath& path) const {
    TFileInput in{path};
    NJson::TJsonValue request = NJson::ReadJsonTree(&in);

    // Prepare a request
    NUri::TUri uri{"localhost", Engine_.Ports()->Get("bass"), request["path"].GetString()};

    // TODO(sparkle): add Joker service header
    TStringBuilder headersStr;
    for (const auto& header : request["headers"].GetArray()) {
        headersStr << header[0].GetString() << ": " << header[1].GetString() << "\r\n";
    }
    for (const auto& [key, value]: MakeProxyHeaders(Ctx_, path.GetName())) {
        headersStr << key << ": " << value << "\r\n";
    }

    TString body = Base64Decode(request["content"].GetString());

    // Do the request
    auto requestType = NNeh::NHttp::ERequestType::Get;
    if (request["method"].GetInteger() == 1) {
        requestType = NNeh::NHttp::ERequestType::Post;
    }

    NNeh::TMessage msg{NNeh::TMessage::FromString(uri.PrintS())};
    NNeh::NHttp::MakeFullRequest(msg, headersStr, body, /*contentType= */ "", requestType);
    NNeh::TResponseRef r = NNeh::Request(msg)->Wait(TDuration::Seconds(5));

    if (!r) {
        LOG(WARNING) << "Request " << path.GetName() << " timed out" << Endl;
        return Nothing();
    } else if (r->IsError()) {
        LOG(WARNING) << "No success on request " << path.GetName() << ": error code " << r->GetErrorCode()
            << ", error text " << r->GetErrorText().Quote() << Endl;
        return Nothing();
    }

    const auto& runSettings = Engine_.RunSettings();
    TFsPath outputPath = TFsPath{runSettings.ResponsesPath} / path.GetName() / "response.json";
    return TMaybe<TRequestResponse>({BeautifyJson(std::move(r->Data)), r->Duration, outputPath});
}

} // namespace NAlice::NShooter
