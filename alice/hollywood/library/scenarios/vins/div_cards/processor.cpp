#include "processor.h"

namespace NAlice::NHollywoodFw::NVins {

IDivCardProcessor::IDivCardProcessor(const TString& intent, const TString& divCardName)
    : Intents_({intent})
    , DivCardName_(divCardName)
{
}

IDivCardProcessor::IDivCardProcessor(const TSet<TString>& intents, const TString& divCardName)
    : Intents_(intents)
    , DivCardName_(divCardName)
{
}

TDivCardProcessor& TDivCardProcessor::Instance() {
    static TDivCardProcessor instance;
    return instance;
}

TDivCardProcessor::EProcessorRet TDivCardProcessor::Process(
    const NProtoVins::TVinsRunResponse& response,
    TDivCardResult& result)
{
    for (const auto& it : Processors_) {
        if (it->Intents().contains(response.GetBassResponse().GetForm().GetName())) {
            result.ScenarioRenderCard = it->Process(response);
            result.DivCardName = it->GetDivCardName();
            return EProcessorRet::Success;
        }
    }
    return EProcessorRet::Unknown;
}

}  // namespace NAlice::NHollywoodFw::NVins
