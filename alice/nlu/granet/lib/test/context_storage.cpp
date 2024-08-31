#include "context_storage.h"
#include <alice/nlu/granet/lib/utils/json_utils.h>
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <alice/nlu/granet/lib/user_entity/collect_from_context.h>
#include <library/cpp/json/json_prettifier.h>
#include <library/cpp/json/json_writer.h>
#include <util/stream/file.h>
#include <util/string/printf.h>

namespace NGranet {

using namespace NJson;

namespace {

    struct TPatchMask {
        TString Name;
        int CounterDigitCount = 0;
        TVector<TString> Paths;
    };

    const TPatchMask MASKS[] = {
        {"TV", 1, {"request.device_state.is_tv_plugged_in"}},
        {"VS", 1, {"request.device_state.video.current_screen"}},
        {"VI", 4, {"request.device_state.video.screen_state.items[*].name"}},
        {"VV", 2, {"request.device_state.video.screen_state.visible_items"}},
        {"NF", 4, {"request.device_state.navigator.user_favorites[*].name"}},
    };

}

// ~~~~ TContextPatchStorage ~~~~

bool TContextPatchStorage::IsDefined() const {
    return Storage.IsDefined();
}

const TJsonValue& TContextPatchStorage::GetContextPatch(TStringBuf patchId) const {
    return Storage["patches"][patchId.Before('_')][patchId];
}

const TJsonValue& TContextPatchStorage::GetContextPatchSafe(TStringBuf patchId) const {
    const TJsonValue& patch = GetContextPatch(patchId);
    Y_ENSURE(patch.IsDefined(), "Context patch " + Cite(patchId) + " not found");
    return patch;
}

NJson::TJsonValue TContextPatchStorage::BuildContext(const TVector<TString>& patchIds) const {
    Y_ENSURE(IsDefined(), "Context storage not defined");
    NJson::TJsonValue context;
    for (const TStringBuf patchId : patchIds) {
        NJsonUtils::ApplyPatch(GetContextPatchSafe(patchId), &context, NJsonUtils::APP_MERGE);
    }
    return context;
}

void TContextPatchStorage::SaveToPath(const TFsPath& path) const {
    TString str = WriteJson(Storage, true, true);
    str = PrettifyJson(str, false, 2);
    TFileOutput(path).Write(str);
}

void TContextPatchStorage::LoadFromPath(const TFsPath& path) {
    Storage = NJsonUtils::ReadJsonFileVerbose(path);
}

const TJsonValue& TContextPatchStorage::GetJsonValue() const {
    return Storage;
}

// static
TContextPatchStorage TContextPatchStorage::FromJsonValue(TJsonValue value) {
    TContextPatchStorage result;
    result.Storage = std::move(value);
    return result;
}

// ~~~~ TContextPatchCollector ~~~~

TContextPatchCollector::TContextPatchCollector()
    : Storage(TJsonValue(JSON_MAP))
{
    Storage["_version"] = "1";
}

TVector<TString> TContextPatchCollector::CollectFromSample(const TJsonValue& sample) {
    TVector<TString> patchIds;
    CollectFromComponent(sample, "request.device_state", &patchIds);
    return patchIds;
}

void TContextPatchCollector::CollectFromComponent(
    const TJsonValue& sample, TStringBuf componentPath, TVector<TString>* patchIds)
{
    Y_ENSURE(patchIds);
    const TJsonValue& component = GetSampleComponent(sample, componentPath);
    for (const TPatchMask& mask : MASKS) {
        TJsonValue patch = CreatePatch(component, componentPath, mask.Paths);
        if (!patch.IsDefined()) {
            continue;
        }
        patchIds->push_back(EmplacePatch(mask.Name, mask.CounterDigitCount, std::move(patch)));
    }
}

// static
const TJsonValue& TContextPatchCollector::GetSampleComponent(const TJsonValue& sample, TStringBuf componentPath) {
    const TJsonValue* component = sample.GetValueByPath(componentPath);
    if (!component) {
        component = sample.GetValueByPath(componentPath.After('.'));
    }
    Y_ENSURE(component, "Can't find " + Cite(componentPath));
    return *component;
}

// static
TJsonValue TContextPatchCollector::CreatePatch(
    const TJsonValue& component, TStringBuf componentPath, const TVector<TString>& maskPaths)
{
    // Create patch
    TJsonValue patch;
    for (TStringBuf path : maskPaths) {
        if (!path.SkipPrefix(componentPath)) {
            continue;
        }
        NJsonUtils::ApplyPatch(NJsonUtils::FilterByMaskedPath(component, path), &patch, NJsonUtils::APP_MERGE);
    }
    if (!patch.IsDefined()) {
        return patch;
    }

    // Move full tree of patch to componentPath (inplace).
    TStringBuf pathRest = componentPath;
    while (!pathRest.empty()) {
        TJsonValue tmp;
        std::swap(patch, tmp);
        patch[pathRest.RNextTok('.')] = std::move(tmp);
    }
    return patch;
}

TString TContextPatchCollector::EmplacePatch(TStringBuf groupName, int digitCount, TJsonValue&& patch) {
    const TString patchString = WriteJson(patch, false, true);
    const auto [it, isNew] = PatchStringToId.try_emplace(patchString);
    if (isNew) {
        it->second = ReallyAddPatch(groupName, digitCount, std::move(patch));
    }
    IncrementPatchSampleCounter(it->second);
    return it->second;
}

TString TContextPatchCollector::ReallyAddPatch(TStringBuf groupName, int digitCount, TJsonValue&& patch) {
    TJsonValue& patchGroup = Storage["patches"][groupName].SetType(JSON_MAP);
    const unsigned int groupSize = patchGroup.GetMapSafe().size();
    const TString id = groupName + Sprintf("_%0*X", digitCount, groupSize);
    Y_ENSURE(!patchGroup.Has(id));
    patchGroup[id] = std::move(patch);
    return id;
}

void TContextPatchCollector::IncrementPatchSampleCounter(TStringBuf patchId) {
    TJsonValue& sampleCounter = Storage["debug"]["sample_count"][patchId.Before('_')][patchId];
    sampleCounter = sampleCounter.GetDoubleSafe(0) + 1;
}

TContextPatchStorage TContextPatchCollector::CreateStorage() const {
    return TContextPatchStorage::FromJsonValue(Storage);
}

} // namespace NGranet
