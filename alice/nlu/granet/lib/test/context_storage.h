#pragma once

#include <alice/nlu/granet/lib/user_entity/dictionary.h>
#include <library/cpp/json/writer/json_value.h>
#include <util/folder/path.h>
#include <util/generic/noncopyable.h>

namespace NGranet {

// ~~~~ TContextPatchStorage ~~~~

class TContextPatchStorage : public TMoveOnly {
public:
    bool IsDefined() const;

    const NJson::TJsonValue& GetContextPatch(TStringBuf patchId) const;
    const NJson::TJsonValue& GetContextPatchSafe(TStringBuf patchId) const;

    NJson::TJsonValue BuildContext(const TVector<TString>& patchIds) const;

    void SaveToPath(const TFsPath& path) const;
    void LoadFromPath(const TFsPath& path);

    const NJson::TJsonValue& GetJsonValue() const;
    static TContextPatchStorage FromJsonValue(NJson::TJsonValue value);

private:
    NJson::TJsonValue Storage;
};

// ~~~~ TContextPatchCollector ~~~~

class TContextPatchCollector : public TMoveOnly {
public:
    TContextPatchCollector();

    TVector<TString> CollectFromSample(const NJson::TJsonValue& sample);

    TContextPatchStorage CreateStorage() const;

private:
    void CollectFromComponent(const NJson::TJsonValue& sample, TStringBuf componentPath, TVector<TString>* patchIds);
    static const NJson::TJsonValue& GetSampleComponent(const NJson::TJsonValue& sample, TStringBuf componentPath);
    static NJson::TJsonValue CreatePatch(const NJson::TJsonValue& component, TStringBuf componentPath,
        const TVector<TString>& maskPaths);
    TString EmplacePatch(TStringBuf groupName, int digitCount, NJson::TJsonValue&& patch);
    TString ReallyAddPatch(TStringBuf groupName, int digitCount, NJson::TJsonValue&& patch);
    void IncrementPatchSampleCounter(TStringBuf patchName);

private:
    NJson::TJsonValue Storage;
    THashMap<TString, TString> PatchStringToId;
};

} // namespace NGranet
