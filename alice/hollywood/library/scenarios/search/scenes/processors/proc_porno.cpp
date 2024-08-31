/*
    LOCATION/POI SEARCH PROCESSOR
*/

#include "proc_porno.h"

#include <alice/hollywood/library/framework/helpers/nlu_features/nlu_features.h>

namespace NAlice::NHollywoodFw::NSearch {

ISearchProcessor::EProcessorRet TProcessorSearchPorno::ProcessSearchObject(const TRunRequest& runRequest,
    const TSearchResultParser& searchInfo,
    TSearchResult& results) const
{
    Y_UNUSED(searchInfo);
    Y_UNUSED(results);
    auto& logger = runRequest.Debug().Logger();

    // TODO: This code chould be checked and validated with https://st.yandex-team.ru/HOLLYWOOD-926
    const TMaybe<float> pornQuery = GetNluFeatureValue(runRequest, NNluFeatures::ENluFeature::IsPornQuery);
    if (pornQuery && *pornQuery > 0 && runRequest.User().GetContentSettings() == EContentSettings::children) {
        LOG_DEBUG(logger) << "pornQuery detected (" << *pornQuery << "), result irrelevant";
        return ISearchProcessor::EProcessorRet::Irrelevant;
    }
    if (pornQuery) {
        LOG_DEBUG(logger) << "pornQuery is not detected (" << *pornQuery << ")";
    } else {
        LOG_DEBUG(logger) << "pornQuery property is not set";
    }
    return ISearchProcessor::EProcessorRet::Unknown;
}

} // namespace NAlice::NHollywoodFw::NSearch
