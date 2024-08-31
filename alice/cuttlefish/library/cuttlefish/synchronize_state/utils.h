#pragma once
#include <alice/cuttlefish/library/cuttlefish/common/http_utils.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/protos/events.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/megamind/protos/common/experiments.pb.h>
#include <apphost/lib/proto_answers/http.pb.h>
#include <apphost/api/service/cpp/service.h>
#include <util/string/ascii.h>
#include <util/string/builder.h>

namespace NAlice::NCuttlefish::NAppHostServices {

inline void AddHttpRequestToDraft(
    NAppHost::IServiceContext& ctx, NAppHostHttp::THttpRequest&& req, const TStringBuf itemType
) {
    req.SetPath(TStringBuilder() << itemType << "@" << req.GetPath());
    ctx.AddProtobufItem(std::move(req), ITEM_TYPE_HTTP_REQUEST_DRAFT);
}

inline TStringBuf GetHttpRequestDraftType(const NAppHostHttp::THttpRequest& req) {
    TStringBuf resultItemType, resultPath;
    Y_ENSURE(TStringBuf(req.GetPath()).TrySplit('@', resultItemType, resultPath), "Not a draft");
    return resultItemType;
}

inline void AddHttpRequestFromDraft(
    NAppHost::IServiceContext& ctx, NAppHostHttp::THttpRequest&& req
) {
    const TString typeAndPath = req.GetPath();

    TStringBuf resultItemType, resultPath;
    Y_ENSURE(TStringBuf(typeAndPath).TrySplit('@', resultItemType, resultPath), "Not a draft");

    req.SetPath(TString(resultPath));
    ctx.AddProtobufItem(std::move(req), resultItemType);
}

inline bool IsExperimentFlagEnabled(const NAlice::TExperimentsProto& exps, const TString& key) {
    const auto storage = exps.GetStorage();
    const auto it = storage.find(key);
    if (it == storage.end()) {
        return false;
    }
    const auto& value = it->second;
    if (value.HasString()) {
        return !value.GetString().empty();
    }
    if (value.HasNumber()) {
        return value.GetNumber() != 0;
    }
    if (value.HasBoolean()) {
        return value.GetBoolean();
    }
    return false;
}

inline bool ConductingExperiment(const NAlice::TExperimentsProto& exps, const TString& key) {
    return IsExperimentFlagEnabled(exps, key);
}

bool AppendTestIds(
    TStringBuilder* path,
    const NAliceProtocol::TSessionContext& ctx,
    const NAliceProtocol::TAbFlagsProviderOptions& options
);

}  // namespace NAlice::NCuttlefish::NAppHostServices
