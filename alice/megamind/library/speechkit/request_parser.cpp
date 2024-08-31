#include "request_parser.h"

#include <alice/megamind/api/request/constructor.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/scenarios/defs/names.h>

#include <alice/megamind/protos/common/experiments.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>

#include <library/cpp/json/common/defs.h>
#include <library/cpp/protobuf/json/json2proto.h>

#include <util/generic/scope.h>


namespace NAlice::NMegamind {

namespace {

void AppendExperiments(const TExperimentsProto& auxExperiments, TSpeechKitRequestProto& skrProto) {
    auto& storage = *skrProto.MutableRequest()->MutableExperiments()->MutableStorage();
    for (const auto& [key, value] : auxExperiments.GetStorage()) {
        storage[key] = value;
    }
}

void UpdateCgi(const TString& cgi, TSpeechKitRequestProto& skrProto) {
    if (!cgi.Empty()) {
        skrProto.MutableRequest()->MutableAdditionalOptions()->MutableBassOptions()->SetMegamindCgi(cgi);
    }
}

TStatus ParseJsonRequest(TRequestCtx& requestCtx, TSpeechKitRequestProto& skrProto) {
    static constexpr bool throwOnError = true;

    NJson::TJsonValue skrJson;
    try {
        NJson::ReadJsonFastTree(requestCtx.Body(), &skrJson, throwOnError);
    } catch (const NJson::TJsonException& e) {
        return TError{TError::EType::Critical} << "ParseRequest: parse json exception: " << e.what();
    }

    auto requestConstructor = NMegamindApi::TRequestConstructor{requestCtx.RTLogger()};
    if (const auto status = requestConstructor.PushSpeechKitJson(skrJson); !status.Ok()) {
        return TError{TError::EType::Critical} << "ParseRequest: TRequestConstructor error: " << status.GetMessage();
    }

    std::move(requestConstructor).MakeRequest().Swap(&skrProto);

    return Success();
}

TStatus ParseProtobufRequest(TRequestCtx& requestCtx, TSpeechKitRequestProto& skrProto) {
    if (skrProto.ParseFromString(requestCtx.Body())) {
        NMegamindApi::TRequestConstructor::PatchContacts(skrProto, requestCtx.RTLogger());
        return Success();
    }

    return TError{TError::EType::Critical} << "ParseRequest: Unable to parse SKR protobuf (ParseFromString), reqsize: "
                                           << requestCtx.Body().size();
}

TStatus RawParseRequest(TRequestCtx& requestCtx, TSpeechKitInitContext& initCtx) {
    TSpeechKitRequestProto skrProto;
    TStatus parseStatus;
    const TRequestCtx::EContentType type = requestCtx.ContentType();
    switch (type) {
        case TRequestCtx::TRequestMeta::Json:
            parseStatus = ParseJsonRequest(requestCtx, skrProto);
            break;

        case TRequestCtx::TRequestMeta::Protobuf:
            parseStatus = ParseProtobufRequest(requestCtx, skrProto);
            break;

        default:
            parseStatus = TError{TError::EType::Critical}
                << "ParseRequest: Unknown content type: '"
                << NMegamindAppHost::TRequestMeta_EContentType_Name(type)
                << "', int value: " << static_cast<int>(type);
            break;
    }

    if (parseStatus) {
        return std::move(*parseStatus);
    }

    initCtx.Proto = MakeSimpleShared<TSpeechKitRequestProto>();
    initCtx.Proto->Swap(&skrProto);
    return Success();
}

} // namespace anonymous

TStatus ParseSkRequest(TRequestCtx& requestCtx, TSpeechKitInitContext& initCtx) {
    try {
        if (auto err = RawParseRequest(requestCtx, initCtx)) {
            return std::move(*err);
        }

        auto& skrProto = *initCtx.Proto;

        // All requests amendmens are here!
        UpdateCgi(requestCtx.Cgi().Print(), skrProto);
        AppendExperiments(requestCtx.Config().GetExperiments(), skrProto);
        initCtx.EventProtoPtr->Swap(initCtx.Proto->MutableRequest()->MutableEvent());
    } catch (...) {
        return TError{TError::EType::Critical} << "ParseRequest: Exception during parse request: " << CurrentExceptionMessage();
    }

    return Success();
}

} // namespace NAlice::NSpeechKit
