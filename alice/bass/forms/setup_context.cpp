#include "setup_context.h"

#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/fetcher/serialization.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>

namespace NBASS {

// static
TResultValue TSetupContext::FromJson(const NSc::TValue& request, TInitializer initData, TPtr* context) {
    TConstructor constructor = [context](TStringBuf formName, TMaybe<TInputAction>&& inputAction, TSlotList&& slots,
                                         const NSc::TValue& meta, TInitializer initData,
                                         const NSc::TValue& sessionState, TSetupResponses&& setupResponses,
                                         TMaybe<NSc::TValue> blocks, TDataSources&& dataSources) {
        *context =
            new TSetupContext{formName,     std::move(inputAction),    std::move(slots), meta, std::move(initData),
                              sessionState, std::move(setupResponses), std::move(blocks), std::move(dataSources),
                              /* originalFormName= */ formName};
        return Nothing();
    };

    return FromJsonImpl(request, std::move(initData), constructor, true /* shouldValidate */);
}

void TSetupContext::ToJson(NSc::TValue* out) const {
    NSc::TValue reportDataJson;
    NSc::TValue& formJson = reportDataJson["form"];

    if (const auto error = SlotsToJson(&formJson, "slots")) {
        error->ToJson(*out);
        return;
    }
    formJson["name"].SetString(FormName());

    if (SourceResponses) {
        NSc::TValue& sourcesJson = reportDataJson["setup_responses"];

        for (const auto& sourceResponse : SourceResponses) {
            sourcesJson[sourceResponse.first] = NHttpFetcher::ResponseToJson(sourceResponse.second);
        }
    }

    BlocksToJson(&reportDataJson);

    (*out)["setup_meta"] = SetupMetaJson;
    (*out)["setup_meta"]["is_feasible"].SetBool(IsFeasible);
    (*out)["report_data"] = std::move(reportDataJson);
}

void TSetupContext::SetFeasible(bool isFeasible) {
    IsFeasible = isFeasible;
}

void TSetupContext::AddSourceResponse(TStringBuf name, NHttpFetcher::TResponse::TRef response) {
    bool inserted = SourceResponses.insert(std::make_pair(TString{name}, response)).second;
    if (!inserted) {
        LOG(WARNING) << "Duplicate source name " << name << ", response ignored" << Endl;
    }
}

void TSetupContext::AddSourceFactorsData(TStringBuf source, const NSc::TValue& data) {
    SetupMetaJson["factors_data"][source] = data;
}

} // namespace NBASS
