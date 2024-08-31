#pragma once

#include <alice/vins/api/vins_api/speechkit/protos/vins_response.pb.h>

#include <alice/protos/data/scenario/data.pb.h>

#include <util/generic/set.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywoodFw::NVins {

struct TDivCardResult {
    TString DivCardName;
    NData::TScenarioData ScenarioRenderCard;
};

class IDivCardProcessor {
public:
    IDivCardProcessor(const TString& intent, const TString& divCardName);
    IDivCardProcessor(const TSet<TString>& intent, const TString& divCardName);
    virtual ~IDivCardProcessor() = default;

    virtual NData::TScenarioData Process(const NProtoVins::TVinsRunResponse& response) const = 0;

    const TSet<TString>& Intents() const {
        return Intents_;
    }
    const TString& GetDivCardName() const {
        return DivCardName_;
    }

private:
    TSet<TString> Intents_;
    TString DivCardName_;
};

class TDivCardProcessor {
public:
    enum class EProcessorRet {
        Unknown,
        Success,
    };

    static TDivCardProcessor& Instance();

    template <class TObject>
    void Register() {
        Processors_.emplace_back(std::make_unique<TObject>());
    }

    EProcessorRet Process(const NProtoVins::TVinsRunResponse& response, TDivCardResult& result);

private:
    TVector<std::unique_ptr<IDivCardProcessor>> Processors_;
};

}  // namespace NAlice::NHollywoodFw::NVins
