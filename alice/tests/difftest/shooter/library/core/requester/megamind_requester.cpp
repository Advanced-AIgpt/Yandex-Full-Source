#include "megamind_requester.h"

#include <alice/joker/library/log/log.h>
#include <alice/tests/difftest/shooter/library/core/util.h>
#include <alice/tests/difftest/shooter/library/core/engine.h>

#include <library/cpp/json/json_writer.h>
#include <library/cpp/neh/http_common.h>
#include <library/cpp/neh/neh.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/uri/uri.h>

#include <util/generic/guid.h>
#include <util/generic/queue.h>
#include <util/string/builder.h>
#include <util/string/strip.h>

namespace NAlice::NShooter {

namespace {

class TMegamindRequest : public NNonCopyable::TNonCopyable, public TMaybe<NJson::TJsonValue> {
public:
    TMegamindRequest(const IContext& ctx, const IEngine &engine, const TString& filePath);

    TStringBuf RequestId() const;
    TMaybe<TStringBuf> Timestamp() const;
    TStringBuf Guid() const;

private:
    const TString Guid_;
};

TMaybe<NJson::TJsonValue> ConstructRequestJsonValue(const IContext& ctx, const IEngine& engine, const TString& filePath) {
    TFileInput in{filePath};
    NJson::TJsonValue value;
    try {
        NJson::ReadJsonTree(&in, &value, /* throwOnError = */ true);
    } catch (const NJson::TJsonException&) {
        LOG(WARNING) << CurrentExceptionMessage() << Endl;
        return Nothing();
    }

    const auto& reqId = value["header"]["request_id"].GetString();
    if (reqId.empty()) {
        LOG(WARNING) << "No request_id in file " << filePath.Quote() << Endl;
        return Nothing();
    }

    // insert token
    const auto& tokens = ctx.Tokens();
    if (tokens.TestUserToken) {
        value["request"]["additional_options"]["oauth_token"] = tokens.TestUserToken;
    }

    // enable/disable exps
    const auto& runSettings = engine.RunSettings();
    auto& expers = value["request"]["experiments"];
    for (const auto& exp : runSettings.DisabledExperiments) {
        expers[exp] = "0";
    }
    for (const auto& exp : runSettings.EnabledExperiments) {
        expers[exp] = "1";
    }

    return value;
}

TMegamindRequest::TMegamindRequest(const IContext& ctx, const IEngine& engine, const TString& filePath)
    : TMaybe<NJson::TJsonValue>{ConstructRequestJsonValue(ctx, engine, filePath)}
    , Guid_{CreateGuidAsString()}
{
}

TStringBuf TMegamindRequest::RequestId() const {
    Y_ASSERT(Defined());
    return GetRef()["header"]["request_id"].GetString();
}

TMaybe<TStringBuf> TMegamindRequest::Timestamp() const {
    Y_ASSERT(Defined());
    if (Get()->Has("application")) {
        const auto& app = GetRef()["application"];
        if (app.Has("timestamp")) {
            return app["timestamp"].GetString();
        }
    }
    return Nothing();
}

TStringBuf TMegamindRequest::Guid() const {
    return Guid_;
}

TString MegamindRequestToString(const TMegamindRequest& request) {
    TStringStream ss;
    NJson::TJsonWriter{&ss, /* formatOutput = */ true, /* sortKeys = */ true}.Write(request.GetRef());
    return std::move(ss.Str());
}

} // namespace

TMegamindRequester::TMegamindRequester(const IContext& ctx, const IEngine& engine)
    : Ctx_{ctx}
    , Engine_{engine}
{
}

TMaybe<TRequestResponse> TMegamindRequester::Request(const TFsPath& path) const {
    TMegamindRequest request{Ctx_, Engine_, path};
    if (!request.Defined()) {
        return Nothing();
    }

    // Prepare a request
    ui16 port = Engine_.Ports()->Get("megamind");
    NUri::TUri uri{"localhost", port, "/speechkit/app/pa"};
    TString headers = BuildHeaders(Ctx_, port, request.RequestId(), request.Guid(), request.Timestamp());
    TString body = MegamindRequestToString(request);

    // Do the request
    NNeh::TMessage msg{NNeh::TMessage::FromString(uri.PrintS())};
    NNeh::NHttp::MakeFullRequest(msg, headers, body, "application/json", NNeh::NHttp::ERequestType::Post);
    NNeh::TResponseRef r = NNeh::Request(msg)->Wait(TDuration::Seconds(5));

    if (!r) {
        LOG(WARNING) << "Request " << request.RequestId() << " timed out" << Endl;
        return Nothing();
    } else if (r->IsError()) {
        LOG(WARNING) << "No success on request " << request.RequestId() << ": error code " << r->GetErrorCode()
            << ", error text " << r->GetErrorText().Quote() << Endl;
        return Nothing();
    }

    const auto& runSettings = Engine_.RunSettings();
    TFsPath outputPath = TFsPath{runSettings.ResponsesPath} / request.RequestId() / "response.json";
    return TMaybe<TRequestResponse>({BeautifyJson(std::move(r->Data)), r->Duration, outputPath});
}

} // namespace NAlice::NShooter
