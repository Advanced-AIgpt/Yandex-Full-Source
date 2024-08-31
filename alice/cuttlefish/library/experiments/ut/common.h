#pragma once

#include <alice/cuttlefish/library/experiments/experiment_patch.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/json/json_reader.h>
#include <util/generic/array_ref.h>
#include <util/folder/path.h>


namespace NVoice::NExperiments {

NJson::TJsonValue CreateJson(TStringBuf json);

inline NJson::TJsonValue::TArray CreateJsonArray(TStringBuf json) {
    return CreateJson(json).GetArraySafe();
}

TExpPatch CreateExperimentPatch(TStringBuf patchJson, const TExpContext& ctx = {});

std::pair<bool, NJson::TJsonValue> CreateAndPatch(
    TStringBuf patchJson,
    TStringBuf eventJson,
    const TExpContext& experimentContext = {},
    const NAliceProtocol::TSessionContext& sessionContext = {}
);

void CheckPatch(
    TStringBuf patchJson,
    TStringBuf originalEventJson,
    TStringBuf patchedEventJson,
    const TExpContext& experimentContext = {},
    const NAliceProtocol::TSessionContext& sessionContext = {}
);

TString MakeFile(TStringBuf data);

NAliceProtocol::TSessionContext CreateSessionContext(
    const NJson::TJsonValue& synchronizeStateEvent,
    TString staffLogin = {}
);

} // namespace NVoice::NExperiments
