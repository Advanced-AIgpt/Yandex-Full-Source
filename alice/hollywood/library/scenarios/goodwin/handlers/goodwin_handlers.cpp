#include "goodwin_handlers.h"

#include "actions.h"

#include <alice/hollywood/library/frame_filler/lib/frame_filler_utils.h>

#include <alice/megamind/library/search/protos/alice_meta_info.pb.h>
#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/common/location.pb.h>
#include <alice/megamind/protos/scenarios/web_search_source.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>
#include <alice/library/network/headers.h>
#include <alice/library/proto/proto.h>

#include <library/cpp/json/json_writer.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/url/url.h>

#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/mapfindptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/string/subst.h>
#include <util/system/env.h>

namespace NAlice {
namespace NFrameFiller {

using TScenarioRunResponse = NScenarios::TScenarioRunResponse;
using TScenarioCommitResponse = NScenarios::TScenarioCommitResponse;

namespace {

const TString META_KEY = "search_doc_meta";
const TString DATA_URL_PREFIX = "data_url_";
const TString MAIN_URL = "main_url";
const TString COMMIT_URL_PREFIX = "commit_url_";

const ::google::protobuf::util::JsonOptions JSON_PRETTY_PRINT = [](){
    ::google::protobuf::util::JsonOptions options;
    options.add_whitespace = true;
    return options;
}();

NJson::TJsonValue PatchState(NJson::TJsonValue doc) {
    const NJson::TJsonValue state = doc["scenario_response"]["state"];
    if (state.IsDefined()) {
        doc["scenario_response"].EraseValue("state");
        doc["scenario_response"]["state"]["value"] = state;
        doc["scenario_response"]["state"]["@type"] = "type.googleapis.com/google.protobuf.Struct";
    }
    return doc;
}

NJson::TJsonValue PatchActions(NJson::TJsonValue doc) {
    const NJson::TJsonValue actions = doc["scenario_response"]["actions"];
    if (!doc["scenario_response"]["frame_actions"].IsDefined()) {
        if (!actions.IsDefined()) {
            return doc;
        }
        NJson::TJsonValue& frameActions = doc["scenario_response"]["frame_actions"];
        for (const auto& [actionName, action] : actions.GetMap()) {
            frameActions[actionName] = JsonFromProto(ToFrameAction(
                JsonToProto<NGoodwin::TAction>(action),
                actionName
            ));
        }
    }
    if (doc["scenario_response"]["actions"].IsDefined()) {
        doc["scenario_response"].EraseValue("actions");
    }

    if (doc["on_submit"].IsDefined()) {
        NScenarios::TFrameAction onSubmitAction;
        try {
            onSubmitAction = JsonToProto<NScenarios::TFrameAction>(doc["on_submit"]);
        } catch (const yexception& e) {
            onSubmitAction = ToFrameAction(
                JsonToProto<NGoodwin::TAction>(doc["on_submit"]),
                "__on_submit"
            );
        }

        // FIXME(the0): Remove this hack when DIALOG-6253 is done.
        if (onSubmitAction.HasCallback()) {
            const NScenarios::TCallbackDirective callback = onSubmitAction.GetCallback();
            *onSubmitAction.MutableDirectives()->AddList()->MutableCallbackDirective() = callback;
        }

        doc["on_submit"] = JsonFromProto(onSubmitAction);
    }
    return doc;
}

NJson::TJsonValue PatchDoc(NJson::TJsonValue doc) {
    return PatchActions(PatchState(doc));
}

struct TRequestParams {
    NHollywood::IHttpRequester::EMethod Method = NHollywood::IHttpRequester::EMethod::Get;
    TString Url;
    TString Body;
    THashMap<TString, TString> Headers;
};

TMaybe<TRequestParams> ParseRequestParams(const ::google::protobuf::Value& proto) {
    TRequestParams params;
    if (proto.kind_case() == ::google::protobuf::Value::kStringValue && !proto.string_value().empty()) {
        params.Url = proto.string_value();
        return params;
    }

    const auto& protoMap = proto.struct_value().fields();

    const auto* urlProto = MapFindPtr(protoMap, "url");
    if (!urlProto) {
        return Nothing();
    }
    params.Url = urlProto->string_value();

    if (const auto* bodyProto = MapFindPtr(protoMap, "body")) {
        params.Body = bodyProto->string_value();
    }

    params.Method = NHollywood::IHttpRequester::EMethod::Get;
    if (const auto* methodProto = MapFindPtr(protoMap, "method")) {
        if (methodProto->string_value() == "POST") {
            params.Method = NHollywood::IHttpRequester::EMethod::Post;
        }
    } else if (!params.Body.empty()) {
        params.Method = NHollywood::IHttpRequester::EMethod::Post;
    }

    if (const auto* headersProto = MapFindPtr(protoMap, "headers")) {
        for (const auto& [key, value] : headersProto->struct_value().fields()) {
            params.Headers[key] = value.string_value();
        }
    }

    if (const auto* authProto = MapFindPtr(protoMap, "auth")) {
        const auto& authMapProto = authProto->struct_value().fields();
        if (const auto* typeProto = MapFindPtr(authMapProto, "type"); typeProto->string_value() == "OAuth") {
            if (const auto* tokenIdProto = MapFindPtr(authMapProto, "token_id")) {
                params.Headers["Authorization"] = "OAuth " + GetEnv(tokenIdProto->string_value());
            }
        }
    }

    return params;
}

TSearchDocMeta GetMeta(const NJson::TJsonValue& doc) {
    return TSearchDocMeta{
        doc[META_KEY]["type"].GetString(),
        doc[META_KEY]["subtype"].GetString()
    };
}

TString Format(const TString& urlTemplate, const TSemanticFrame& frame) {
    TString url = urlTemplate;
    for (const auto& slot : frame.GetSlots()) {
        if (slot.GetType().empty() || slot.GetValue().empty()) {
            continue;
        }
        SubstGlobal(url, "{" + slot.GetName() + "}", CGIEscapeRet(slot.GetValue()));
    }
    return url;
}

TScenarioRunResponse MakeIrrelevantResponse() {
    TScenarioRunResponse response;
    // FIXME(the0): Support internationalized response based on request.GetBaseRequest().GetClientInfo().GetLang()
    response.MutableResponseBody()->MutableLayout()->AddCards()->SetText("У меня нет хорошего ответа.");
    response.MutableFeatures()->SetIsIrrelevant(true);
    return response;
}

TString MakeLogMessage(const TRequestParams& params) {
    TStringBuilder msg;
    msg << "Headers:\n";
    for (const auto& [header, value] : params.Headers) {
        msg << "  " << header << ": " << value << "\n";
    };
    msg << "Url: " << params.Url << "\n";
    msg << "Method: " << ((params.Method == NHollywood::IHttpRequester::EMethod::Get) ? "GET" : "POST") << "\n";
    msg << "Body: " << params.Body;
    return msg;
}

TString MakeRequestId(const TString& prefix, size_t index) {
    return prefix + ToString(index + 1);
}

void RequestUrls(
    NHollywood::IHttpRequester& requester,
    const ::google::protobuf::RepeatedPtrField<::google::protobuf::Value>& urlsProto,
    const TString& prefix,
    std::function<void(const TString&)> handleResponse,
    TRTLogger& logger
) {
    for (size_t index = 0; index < static_cast<size_t>(urlsProto.size()); ++index) {
        const auto& urlDataProto = urlsProto[static_cast<int>(index)];
        THashMap<TString, TString> parsedHeaders;
        if (const auto params = ParseRequestParams(urlDataProto); params.Defined()) {
            LOG_INFO(logger) << "Will make request.\n" << MakeLogMessage(*params);
            requester.Add(MakeRequestId(prefix, index), params->Method, params->Url, params->Body, params->Headers);
        } else {
            LOG_WARNING(logger) << "Bad url data: " << SerializeProtoText(urlDataProto);
        }
    }
    requester.Start();
    for (size_t index = 0; index < static_cast<size_t>(urlsProto.size()); ++index) {
        handleResponse(requester.Fetch(MakeRequestId(prefix, index)));
    }
}

} // namespace

TFrameFillerScenarioResponse ToScenarioResponse(const TString& string) {
    return ToScenarioResponse(
        JsonToProto<TFrameFillerRequest>(
            PatchDoc(NJson::ReadJsonFastTree(string)),
            /* validateUtf8= */ true,
            /* ignoreUnknownFields= */ true
        )
    );
}

TGoodwinScenarioRunHandler::TGoodwinScenarioRunHandler(
    TSimpleSharedPtr<NHollywood::IHttpRequester> urlRequester,
    TAcceptDocPredicate acceptDoc
)
    : UrlRequester(urlRequester)
    , AcceptDoc(std::move(acceptDoc))
{
    Y_ENSURE(UrlRequester);
}

TFrameFillerScenarioResponse TGoodwinScenarioRunHandler::Do(
    const NHollywood::TScenarioRunRequestWrapper& request,
    TRTLogger& logger
) const {
    const auto response =  DoImpl(request, logger);
    LOG_INFO(logger) << "Internal scenario response: " << SerializeProtoText(response);
    return response;
}

TFrameFillerScenarioResponse TGoodwinScenarioRunHandler::DoImpl(
    const NHollywood::TScenarioRunRequestWrapper& request,
    TRTLogger& logger
) const {
    if (!request.Proto().HasInput()) {
        return ToScenarioResponse(TError<TScenarioRunResponse>{} << "No input in request.");
    }
    const NScenarios::TInput& input = request.Proto().GetInput();

    if (input.GetEventCase() == NScenarios::TInput::EventCase::kCallback) {
        LOG_INFO(logger) << "Got callback input: " << input.GetCallback();
        const auto& directive = input.GetCallback();
        if (directive.GetName() == REQUEST_URL_CALLBACK) {
            return ProcessRequestUrlDirective(
                directive,
                input,
                request.Proto().GetBaseRequest(),
                request.GetDataSource(EDataSourceType::BLACK_BOX),
                logger
            );
        }
        return ToScenarioResponse(TError<TScenarioRunResponse>{} << "Unknown directive: " << directive.Utf8DebugString());
    }

    LOG_INFO(logger) << "Got regular request.";
    return ProcessGoodwinResponse(request, logger);
}

TGoodwinScenarioCommitHandler::TGoodwinScenarioCommitHandler(TSimpleSharedPtr<NHollywood::IHttpRequester> urlRequester)
    : UrlRequester(urlRequester)
{
    Y_ENSURE(UrlRequester);
}

NScenarios::TScenarioCommitResponse TGoodwinScenarioCommitHandler::Do(
    const NHollywood::TScenarioApplyRequestWrapper& request,
    TRTLogger& logger
) const {
    const auto response = DoImpl(request, logger);
    LOG_INFO(logger) << "Internal scenario response: " << SerializeProtoText(response);
    return response;
}

NScenarios::TScenarioCommitResponse TGoodwinScenarioCommitHandler::DoImpl(
    const NHollywood::TScenarioApplyRequestWrapper& request,
    TRTLogger& logger
) const {
    TScenarioCommitResponse response;
    *response.MutableSuccess() = TScenarioCommitResponse::TSuccess{};
    if (!request.Proto().HasArguments() || !request.Proto().GetArguments().Is<::google::protobuf::ListValue>()) {
        LOG_WARNING(logger) << "Got no urls.";
        return response;
    }
    ::google::protobuf::ListValue commitUrlsProto;
    if (!request.Proto().GetArguments().UnpackTo(&commitUrlsProto)) {
        return TError<TScenarioCommitResponse>{} << "Error unpacking commit urls.";
    }

    RequestUrls(
        *UrlRequester,
        commitUrlsProto.values(),
        COMMIT_URL_PREFIX,
        [](const TString&){},
        logger
    );

    return response;
}

NScenarios::TScenarioApplyResponse TGoodwinScenarioApplyHandler::Do(
    const NHollywood::TScenarioApplyRequestWrapper& /* request */,
    TRTLogger& /* logger */
) const {
    return TError<NScenarios::TScenarioApplyResponse>{} << "Unexpected apply request.";
}

TFrameFillerScenarioResponse TGoodwinScenarioRunHandler::ProcessGoodwinUrl(
    const TString& urlTemplate,
    const ::google::protobuf::Value* callbackPayload,
    const NJson::TJsonValue& fetchedData,
    const NScenarios::TInput& input,
    const NScenarios::TScenarioBaseRequest& baseRequest,
    const NScenarios::TDataSource* blackbox
) const {
    Y_ENSURE(!urlTemplate.empty(), "Got empty url!");

    NJson::TJsonValue urlPayload;
    urlPayload["goodwin"]["payload"].SetType(NJson::JSON_MAP);
    urlPayload["goodwin"]["state"].SetType(NJson::JSON_MAP);
    urlPayload["goodwin"]["data"] = fetchedData;
    if (callbackPayload) {
        urlPayload["goodwin"]["payload"] = JsonFromProto(*callbackPayload);
    }
    ::google::protobuf::Struct state;
    if (baseRequest.GetState().UnpackTo(&state)) {
        urlPayload["goodwin"]["state"] = JsonFromProto(state);
    }
    urlPayload["goodwin"]["input_frames"].SetType(NJson::JSON_ARRAY);
    for (const auto& frame : input.GetSemanticFrames()) {
        urlPayload["goodwin"]["input_frames"].AppendValue(JsonFromProto(frame));
    }

    THashMap<TString, TString> headers;
    // const TString& clientIP = baseRequest.GetOptions().GetClientIP();
    // if (!clientIP.empty()) {
    //     headers["X-Forwarded-For"] = clientIP;
    // }
    const TMaybe<TLocation> location = baseRequest.HasLocation() ? baseRequest.GetLocation() : TMaybe<TLocation>{};
    if (location.Defined()) {
        headers["X-Alice-Goodwin-Longitude-Latitude"] = Join(",", location->GetLon(), location->GetLat());
    }
    if (blackbox && !blackbox->GetUserInfo().GetUid().empty()) {
        headers["X-Alice-Goodwin-Uid"] = blackbox->GetUserInfo().GetUid();
    }

    TAliceMetaInfo aliceMetaInfo;
    aliceMetaInfo.SetRequestType("Goodwin");
    *aliceMetaInfo.MutableClientInfo() = baseRequest.GetClientInfo();
    headers[NNetwork::HEADER_X_YANDEX_ALICE_META_INFO] = Base64Encode(aliceMetaInfo.SerializeAsString());

    TRequestParams params{
        .Method = NHollywood::IHttpRequester::EMethod::Post,
        .Url = urlTemplate,
        .Body = JsonToString(urlPayload),
        .Headers = headers
    };
    if (!input.GetSemanticFrames().empty()) {
        params.Url = Format(params.Url, input.GetSemanticFrames(0));
    }
    return ToScenarioResponse(UrlRequester->Add(MAIN_URL, params.Method, params.Url, params.Body, params.Headers).Start().Fetch(MAIN_URL));
}

TFrameFillerScenarioResponse TGoodwinScenarioRunHandler::ProcessRequestUrlDirective(
    const NScenarios::TCallbackDirective& directive,
    const NScenarios::TInput& input,
    const NScenarios::TScenarioBaseRequest& baseRequest,
    const NScenarios::TDataSource* blackbox,
    TRTLogger& logger
) const {
    if (const auto* urlTemplateProto = MapFindPtr(directive.GetPayload().fields(), "url")) {
        LOG_INFO(logger) << "Processing url: " << urlTemplateProto->string_value();

        NJson::TJsonValue fetchedData(NJson::JSON_ARRAY);
        if (const auto* dataUrlsProto = MapFindPtr(directive.GetPayload().fields(), "data_urls");
            dataUrlsProto && !dataUrlsProto->list_value().values().empty()
        ) {
            LOG_INFO(logger) << "Data urls provided, will fetch data.";
            RequestUrls(
                *UrlRequester,
                dataUrlsProto->list_value().values(),
                DATA_URL_PREFIX,
                [&fetchedData](const TString& response){ fetchedData.AppendValue(response); },
                logger
            );
        }

        TFrameFillerScenarioResponse response = ProcessGoodwinUrl(
            urlTemplateProto->string_value(),
            MapFindPtr(directive.GetPayload().fields(), "payload"),
            fetchedData,
            input,
            baseRequest,
            blackbox
        );

        if (const auto* commitUrlsProto = MapFindPtr(directive.GetPayload().fields(), "commit_urls");
            commitUrlsProto && !commitUrlsProto->list_value().values().empty()
        ) {
            LOG_INFO(logger) << "Commit urls provided, will make commit candidate.";
            NScenarios::TScenarioRunResponse::TCommitCandidate commitCandidate;
            if (response.GetFrameFillerRequest().HasScenarioResponse()) {
                *commitCandidate.MutableResponseBody() = GetResponseBody(response.GetFrameFillerRequest());
            }
            commitCandidate.MutableArguments()->PackFrom(commitUrlsProto->list_value());
            *response.MutableFrameFillerRequest()->MutableCommitCandidate() = commitCandidate;
        }

        return response;
    }
    return ToScenarioResponse(TError<TScenarioRunResponse>{} << "Bad directive: " << directive.Utf8DebugString());
}

TMaybe<NJson::TJsonValue> TGoodwinScenarioRunHandler::GetGoodwinDoc(const NJson::TJsonValue& renderrerResponse) const {
    for (const auto& docsKey : {"docs", "docs_right"}) {
        for (const NJson::TJsonValue& doc : renderrerResponse[docsKey].GetArray()) {
            if (!doc.IsDefined() || !AcceptDoc(GetMeta(doc))) {
                continue;
            }
            return doc;
        }
    }
    return Nothing();
}

NJson::TJsonValue GetRenderrerResponse(
    const NHollywood::TScenarioRunRequestWrapper& request,
    TRTLogger& logger
) {
    const NScenarios::TDataSource* renderrerDataSource = request.GetDataSource(EDataSourceType::WEB_SEARCH_RENDERRER);
    if (renderrerDataSource && !renderrerDataSource->GetWebSearchRenderrer().GetResponse().empty()) {
        LOG_INFO(logger) << "Got rendderer response from EDataSourceType::WEB_SEARCH_RENDERRER";
        return JsonFromString(renderrerDataSource->GetWebSearchRenderrer().GetResponse());
    }

    return NJson::TJsonValue::UNDEFINED;
}

TFrameFillerScenarioResponse TGoodwinScenarioRunHandler::ProcessGoodwinResponse(
    const NHollywood::TScenarioRunRequestWrapper& request,
    TRTLogger& logger
) const {
    const NJson::TJsonValue jsonRenderrerResponse = GetRenderrerResponse(request, logger);
    if (!jsonRenderrerResponse.IsDefined()) {
        LOG_INFO(logger) << "No search renderrer response.";
        return NFrameFiller::ToScenarioResponse(MakeIrrelevantResponse());
    }
    LOG_INFO(logger) << "Renderrer response: " << jsonRenderrerResponse;
    if (const auto doc = GetGoodwinDoc(jsonRenderrerResponse); doc.Defined()) {
        return ToScenarioResponse(
            JsonToProto<TFrameFillerRequest>(
                PatchDoc(*doc),
                /* validateUtf8= */ true,
                /* ignoreUnknownFields= */ true
            )
        );
    }

    LOG_INFO(logger) << "No goodwin doc found.";
    return NFrameFiller::ToScenarioResponse(MakeIrrelevantResponse());
}

} // namespace NFrameFiller
} // namespace NAlice
