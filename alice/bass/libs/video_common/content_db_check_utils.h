#pragma once

#include <alice/bass/libs/video_common/defs.h>
#include <alice/bass/libs/video_common/utils.h>
#include <alice/bass/libs/ydb_helpers/path.h>

#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <util/generic/string.h>
#include <util/system/types.h>

namespace NVideoCommon {

using TDbCheckListScheme = NBassApi::TContentDbCheckList<TSchemeTraits>;
using TDbCheckListConstScheme = TDbCheckListScheme::TConst;
using TDbCheckList = TSchemeHolder<TDbCheckListScheme>;

using TDbAmountCheckScheme = NBassApi::TDbAmountCheck<TSchemeTraits>;
using TDbAmountCheckConstScheme = TDbAmountCheckScheme::TConst;
using TDbAmountCheck = TSchemeHolder<TDbAmountCheckScheme>;

using TVideoItemDbAmountCheckScheme = NBassApi::TVideoItemDbAmountCheck<TSchemeTraits>;
using TVideoItemDbAmountCheckConstScheme = TVideoItemDbAmountCheckScheme::TConst;
using TVideoItemDbAmountCheck = TSchemeHolder<TVideoItemDbAmountCheckScheme>;

using TSingleItemCheckScheme = NBassApi::TSingleItemCheck<TSchemeTraits>;
using TSingleItemCheckConstScheme = TSingleItemCheckScheme::TConst;
using TSingleItemCheck = TSchemeHolder<TSingleItemCheckScheme>;

using TSerialDescriptorCheckScheme = NBassApi::TSerialDescriptorCheck<TSchemeTraits>;
using TSerialDescriptorCheckConstScheme = TSerialDescriptorCheckScheme::TConst;
using TSerialDescriptorCheck = TSchemeHolder<TSerialDescriptorCheckScheme>;

using TSeasonDescriptorCheckScheme = NBassApi::TSeasonDescriptorCheck<TSchemeTraits>;
using TSeasonDescriptorCheckConstScheme = TSeasonDescriptorCheckScheme::TConst;
using TSeasonDescriptorCheck = TSchemeHolder<TSeasonDescriptorCheckScheme>;

TDbCheckList ParseCheckListFromJsonOrThrow(TStringBuf json);
TDbCheckList LoadCheckListFromFileOrThrow(const TString& jsonPath);

class TDbChecker {
public:
    TDbChecker(NYdb::NTable::TTableClient& tableClient, TString database, TString candidateDir, TString referenceDir)
        : TableClient(tableClient)
        , Database(std::move(database))
        , CandidateDir(std::move(candidateDir))
        , ReferenceDir(std::move(referenceDir)) {
    }

    NVideoCommon::TResult CheckContentDb(const TDbCheckList& checks);

    NYdb::NTable::TTableClient& GetTableClient() {
        return TableClient;
    }

    const TString& GetCandidateDir() const {
        return CandidateDir;
    }

    const TString& GetReferenceDir() const {
        return ReferenceDir;
    }

private:
    NVideoCommon::TResult RunCheckForTotalCountDiff(TDbAmountCheckConstScheme check, TStringBuf tableName,
                                                    bool isForItems);
    NVideoCommon::TResult RunCheckForProviderCountDiff(TDbAmountCheckConstScheme check, TStringBuf tableName,
                                                       TStringBuf providerName, bool isForItems);

    NVideoCommon::TResult RunChecksForTotalCountDiff(TVideoItemDbAmountCheckConstScheme check);
    NVideoCommon::TResult RunChecksForProviderCountDiff(TVideoItemDbAmountCheckConstScheme check,
                                                        TStringBuf providerName);
    NVideoCommon::TResult RunChecksForIndividualItem(TSingleItemCheckConstScheme check);

private:
    NYdb::NTable::TTableClient& TableClient;
    const TString Database;
    const TString CandidateDir;
    const TString ReferenceDir;
};

} // namespace NVideoCommon
