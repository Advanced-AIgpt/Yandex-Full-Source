#pragma once

#include <alice/cachalot/library/modules/activation/common.h>

#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/str_stl.h>
#include <util/system/rwlock.h>


namespace NCachalot {

struct TActivationStorageYqlCodesOptions {
public:
    TActivationOperationOptions Flags;
    TString DatabaseName;

public:
    // For THashMap.
    bool operator==(const TActivationStorageYqlCodesOptions& other) const;
};


struct TActivationStorageYqlCodes : public TThrRefBase {
    TString Annoncement;
    TString TryAcquireLeadership;
    TString GetLeader;
    TString CleanupLeaders;
};

using TActivationStorageYqlCodesPtr = TIntrusivePtr<TActivationStorageYqlCodes>;

}   // namespace NCachalot


// For THashMap.
template <>
struct THash<NCachalot::TActivationStorageYqlCodesOptions> {
    size_t operator()(const NCachalot::TActivationStorageYqlCodesOptions& object) const noexcept {
        return CombineHashes(THash<TStringBuf>()(object.DatabaseName), object.Flags.CombineFlags());
    }
};


namespace NCachalot {

class TActivationStorageYqlCodesStorage {
public:
    void Initialize(const TString& databaseName);
    TActivationStorageYqlCodesPtr GetCodes(const TActivationStorageYqlCodesOptions& options);

    static TActivationStorageYqlCodesStorage& GetInstance();

private:
    TRWMutex Lock;
    THashMap<TActivationStorageYqlCodesOptions, TActivationStorageYqlCodesPtr> Config2Codes;
};


TActivationStorageYqlCodesPtr MakeCodes(const TActivationStorageYqlCodesOptions options);

}   // namespace NCachalot
