#include "common.h"
#include <alice/cuttlefish/library/experiments/flags_json.h>
#include <alice/cuttlefish/library/experiments/session_context_proxy.h>
#include <alice/cuttlefish/library/experiments/utils.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/stream/file.h>
#include <util/generic/guid.h>
#include <util/string/cast.h>


namespace NVoice::NExperiments {

NJson::TJsonValue CreateJson(TStringBuf jsonString)
{
    NJson::TJsonValue json;
    try {
        NJson::ReadJsonTree(jsonString, &json, /* throwOnError = */ true);
    } catch (const yexception& exc)  {
        Cerr << "Invalid test JSON: " << exc << "\nSource:\n" << jsonString << Endl;
        throw;
    }
    return json;
}

TExpPatch CreateExperimentPatch(TStringBuf patchJson, const TExpContext& ctx)
{
    return TExpPatch(ctx, CreateJson(patchJson).GetArraySafe());
}


std::pair<bool, NJson::TJsonValue> CreateAndPatch(
    TStringBuf patchJson,
    TStringBuf eventJson,
    const TExpContext& experimentContext,
    const NAliceProtocol::TSessionContext& sessionContext
) {
    NJson::TJsonValue event = CreateJson(eventJson);
    const TExpPatch patch = CreateExperimentPatch(patchJson, experimentContext);
    bool result = patch.Apply(event, sessionContext);
    return {result, event};
}

void CheckPatch(
    TStringBuf patchJson,
    TStringBuf originalEventJson,
    TStringBuf patchedEventJson,
    const TExpContext& experimentContext,
    const NAliceProtocol::TSessionContext& sessionContext
) {
    const auto res = CreateAndPatch(patchJson, originalEventJson, experimentContext, sessionContext);
    UNIT_ASSERT(res.first);
    UNIT_ASSERT_VALUES_EQUAL(res.second, CreateJson(patchedEventJson));
}

TString MakeFile(TStringBuf data)
{
    const TString fileName = CreateGuidAsString();
    TFileOutput out(fileName);
    out.Write(data);
    return fileName;
}


NAliceProtocol::TSessionContext CreateSessionContext(
    const NJson::TJsonValue& event,
    TString staffLogin
) {
    NAliceProtocol::TSessionContext ctx;

    if (const TString* val = GetValueByPath<TString>(event, {"payload", "lang"}))
        SetLang(ctx, *val);
    if (const TString* val = GetValueByPath<TString>(event, {"payload", "vinsUrl"}))
        SetVinsUrl(ctx, *val);
    if (const TString* val = GetValueByPath<TString>(event, {"payload", "vins", "application", "app_id"}))
        SetAppId(ctx, *val);
    if (const TString* val = GetValueByPath<TString>(event, {"payload", "uuid"}))
        SetUuid(ctx, *val);
    if (const TString* val = GetValueByPath<TString>(event, {"payload", "auth_token"}))
        SetAppToken(ctx, *val);

    if (staffLogin) {
        ctx.MutableUserInfo()->SetStaffLogin(std::move(staffLogin));
    }

    return ctx;
}

} // namespace NVoice::NExperiments
