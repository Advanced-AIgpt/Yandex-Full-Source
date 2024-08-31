#pragma once

#include "utils.h"

#include <alice/bass/libs/ydb_helpers/path.h>
#include <alice/bass/libs/ydb_helpers/settings.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>

#include <utility>

namespace NYdb {
namespace NScheme {
class TSchemeClient;
}
}

namespace NVideoCommon {
inline constexpr TStringBuf KINOPOISK_SVOD_V1_TABLE_PREFIX = "kinopoisk_svod_v1_";
inline constexpr TStringBuf KINOPOISK_SVOD_V2_TABLE_PREFIX = "kinopoisk_svod_v2_";

class TIsSVODStatus : public NYdb::TStatus {
public:
    template <typename TStatus>
    TIsSVODStatus(TStatus&& status, bool IsSVOD)
        : NYdb::TStatus(std::forward<TStatus>(status))
        , SVOD(IsSVOD) {
    }

    bool IsSVOD() const {
        return SVOD;
    }

private:
    bool SVOD;
};

struct TSVODError {
    explicit TSVODError(TStringBuf msg)
        : Msg(msg) {
    }

    explicit TSVODError(const NYdb::TStatus& status)
        : Msg(TStringBuilder() << status.GetStatus() << ": " << status.GetIssues().ToString()) {
    }

    TString Msg;
};

TMaybe<TSVODError> RemoveIfNotKinopoiskSVOD(NYdb::NTable::TTableClient& client, const NYdbHelpers::TTablePath& path,
                                            const NYdb::NTable::TRetryOperationSettings& settings,
                                            const TVector<TString>& in, TVector<TString>& out);
TMaybe<TSVODError> RemoveIfNotKinopoiskSVOD(NYdb::NTable::TTableClient& client, const NYdbHelpers::TTablePath& path,
                                            const TVector<TString>& in, TVector<TString>& out);

TIsSVODStatus
IsKinopoiskSVOD(NYdb::NTable::TTableClient& client, const NYdbHelpers::TTablePath& path, const TString& kinopoiskId,
                const NYdb::NTable::TRetryOperationSettings& settings = NYdbHelpers::DefaultYdbRetrySettings());

// Kinopoisk SVOD tables should be named with timestamp string. This
// method drops all but two last tables and the latest table.
bool DropOldKinopoiskSVODTables(NYdb::NScheme::TSchemeClient& schemeClient, NYdb::NTable::TTableClient& tableClient,
                                const NYdbHelpers::TPath& database, TStringBuf prefix, const TString& latest);

class TKinopoiskSourceRequestFactoryWrapper : public ISourceRequestFactory {
public:
    TKinopoiskSourceRequestFactoryWrapper(std::unique_ptr<ISourceRequestFactory> delegate,
                                          const TString& kinopoiskToken, const TString& serviceId,
                                          const TMaybe<TString>& authorization = Nothing(),
                                          const TMaybe<TString>& userAgent = Nothing());

    // ISourceRequestFactory overrides:
    NHttpFetcher::TRequestPtr Request(TStringBuf path) override;
    NHttpFetcher::TRequestPtr AttachRequest(TStringBuf path, NHttpFetcher::IMultiRequest::TRef multiRequest) override;

private:
    void FillRequest(NHttpFetcher::TRequest& request);

private:
    std::unique_ptr<ISourceRequestFactory> Delegate;

    TString KinopoiskToken;
    TString ServiceId;

    TMaybe<TString> Authorization;
    TMaybe<TString> UserAgent;
};

class TKinopoiskContentInfoProvider : public IContentInfoProvider {
public:
    TKinopoiskContentInfoProvider(std::unique_ptr<TKinopoiskSourceRequestFactoryWrapper> source,
                                  bool enableShowingItemsComingSoon);

    // IContentInfoProvider overrides:
    std::unique_ptr<ISerialDescriptorHandle>
    MakeSerialDescriptorRequest(TVideoItemConstScheme tvShowItem,
                                NHttpFetcher::IMultiRequest::TRef multiRequest) override;

    EPreferredSeasonDownloadMode GetPreferredSeasonDownloadMode() const override {
        return EPreferredSeasonDownloadMode::Individual;
    }

    std::unique_ptr<ISeasonDescriptorHandle>
    MakeSeasonDescriptorRequest(const TSerialDescriptor& serialDescr, const TSeasonDescriptor& seasonDescr,
                                NHttpFetcher::IMultiRequest::TRef multiRequest) override;

protected:
    // IContentInfoProvider overrides:
    std::unique_ptr<IVideoItemHandle> MakeContentInfoRequestImpl(TLightVideoItemConstScheme item,
                                                         NHttpFetcher::IMultiRequest::TRef multiRequest) override;

private:
    std::unique_ptr<TKinopoiskSourceRequestFactoryWrapper> Source;
    bool EnableShowingItemsComingSoon;
};

NYdb::TValue ToKinopoiskIdStruct(const TString& kinopoiskId);
NYdb::TValue ToIdsList(const TVector<ui64>& ids);
NYdb::TValue ToIdStruct(const ui64 Id);

template <typename TIt>
NYdb::TValue ToKinopoiskIdsList(TIt begin, TIt end) {
    Y_ASSERT(begin != end);

    NYdb::TValueBuilder builder;

    builder.BeginList();
    for (; begin != end; ++begin)
        builder.AddListItem(ToKinopoiskIdStruct(*begin));
    builder.EndList();

    return builder.Build();
}

} // namespace NVideoCommon
