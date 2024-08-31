#include "processors.h"

#include <util/system/execpath.h>

namespace NAlice::NHollywoodFw::NSearch {

/*
    Return singletom instance of TSearchProcessorInstance
*/
TSearchProcessorInstance& TSearchProcessorInstance::Instance() {
    static TSearchProcessorInstance instance;
    return instance;
}

NScenarios::TAnalyticsInfo::TObject TSearchProcessorInstance::CreateAiLog(const TSearchResultParser& searchInfo) {
    NScenarios::TAnalyticsInfo::TObject aiObject;

    // Prepare analytics info for future use
    const auto descrDoc = searchInfo.CollectSnippets({TSearchResultParser::EUseDatasource::Docs,
                                                      TSearchResultParser::EUseDatasource::DocsRight});
    const auto descrWiz = searchInfo.CollectSnippets({TSearchResultParser::EUseDatasource::Wizplaces});
    auto* searchGenericInfo = aiObject.MutableSearchGenericInfo();
    searchGenericInfo->SetWinner("");
    for (const auto& it: descrDoc) {
        NAlice::NSearch::TSearchGenericInfo::TDataSourceType dsType;
        dsType.SetParent(it.Parent);
        dsType.SetType(it.Type);
        dsType.SetSubtype(it.Subtype);
        dsType.SetTemplate(it.Template);
        *searchGenericInfo->AddDataSourceDocs() = std::move(dsType);
    }
    for (const auto& it: descrWiz) {
        NAlice::NSearch::TSearchGenericInfo::TDataSourceType dsType;
        dsType.SetParent(it.Parent);
        dsType.SetType(it.Type);
        dsType.SetSubtype(it.Subtype);
        dsType.SetTemplate(it.Template);
        *searchGenericInfo->AddDataSourceWizplaces() = std::move(dsType);
    }

    aiObject.SetId("hwf_processor");
    aiObject.SetName("datasource_dump");
    aiObject.SetHumanReadable("Сводная информация по источникам данных для фреймворка");
    return aiObject;
}

/*
    Process handling search request
    This function enumerates all exisiting search processors and find who will be able to fill rich card info
*/
ISearchProcessor::EProcessorRet TSearchProcessorInstance::ProcessSearchObject(const TRunRequest& runRequest,
    const TSearchResultParser& searchInfo,
    TSearchResult& results)
{
    const auto aiObject = CreateAiLog(searchInfo);
    for (const auto& it : Processors_) {
        TSearchResult resTmp = {
            /* ScenarioData  */ NData::TScenarioData{},
            /* RenderArgs    */ NHollywood::TSearchRenderProto{},
            /* SearchFeatures*/ NScenarios::TSearchFeatures{},
            /* AiObject      */ aiObject};

        LOG_DEBUG(runRequest.Debug().Logger()) << "Try to use processor: " << it->GetName();
        const auto rc = it->ProcessSearchObject(runRequest, searchInfo, resTmp);
        switch (rc) {
            case ISearchProcessor::EProcessorRet::Unknown:
                // Not recognized, continue enumeration (choose next processor in for() loop)
                break;
            case ISearchProcessor::EProcessorRet::Success:
                resTmp.AiObject.MutableSearchGenericInfo()->SetWinner(it->GetName());
                LOG_DEBUG(runRequest.Debug().Logger()) << "Selected processor: " << it->GetName();
                resTmp.RenderArgs.SetProcWinner(it->GetName());
                results = std::move(resTmp);
                runRequest.AI().AddObject(std::move(results.AiObject));
                return rc;
            case ISearchProcessor::EProcessorRet::OldFlow:
            case ISearchProcessor::EProcessorRet::Irrelevant:
                runRequest.AI().AddObject(std::move(results.AiObject));
                return rc;
        }
    }
    runRequest.AI().AddObject(std::move(results.AiObject));
    return ISearchProcessor::EProcessorRet::Unknown;
}

/*
    Ctor for ISearchProcessor
*/
ISearchProcessor::ISearchProcessor(const TStringBuf& name)
    : Name_ (name)
{
}


} // namespace NAlice::NHollywoodFw::NSearch
