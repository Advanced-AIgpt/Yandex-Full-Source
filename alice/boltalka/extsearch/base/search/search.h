#pragma once

#include "async_logger.h"
#include "index_data.h"
#include "relevance.h"
#include "unistat_registry.h"

#include <kernel/externalrelev/relev.h>
#include <search/reqparam/reqparam.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/threading/local_executor/local_executor.h>


namespace NNlg {

class TNlgSearch : public TThrRefBase, public IExternalSearch {
public:
    IRelevance* CreateRelevance(const TBaseIndexData& baseIndexData, TIndexAccessors& /*accessors*/, const TRequestParams* rp) const override;
    IRelevance* CreateRelevance(const TArchiveManager& archiveManager, const TRequestParams* rp) const;
    TNlgRelevance* CreateRelevanceDeprecated(const TArchiveManager& archiveManager, const TRequestParams* rp) const;
    IRelevance* CreateRelevanceProtocol(const TArchiveManager& archiveManager, const TRequestParams* rp) const;

    const IFactorsInfo* GetFactorsInfo() const override;
    IIndexData* GetIndexData() override;
    void Init(const TSearchConfig* config) override;
    void PrepareRequestParams(TRequestParams* rp) const override;
    void TuneRequestParams(TRequestParams* rp) const override;

private:
    TIndexData IndexData;
    THolder<TAsyncLogger> Logger;
    THolder<NPar::TLocalExecutor> Seq2SeqExecutor;
    THolder<TUnistatRegistry> UnistatRegistry;
};


TNlgSearch* CreateNlgSearch();
TNlgSearch* CreateNlgSearch(const TString& indexDir, const TSearchConfig& config);

using TNlgSearchPtr = TIntrusivePtr<TNlgSearch>;

}
