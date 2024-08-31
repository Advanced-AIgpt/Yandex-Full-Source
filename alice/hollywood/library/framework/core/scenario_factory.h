#pragma once

//
// HOLLYWOOD FRAMEWORK
// Internal class : scenario factory
//

#include "error.h"
#include "node_caller.h"
#include "request.h"
#include "scenario.h"
#include "storage.h"

#include <alice/hollywood/library/base_scenario/scenario.h> // For FindScenarioHandle()/LinkOldScenario()support

#include <alice/library/metrics/sensors.h>

#include <util/folder/path.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywoodFw {
    class TProtoHwFramework;
    class TTestEnvironment;
} // namespace NAlice::NHollywoodFw

namespace NAlice::NHollywoodFw::NPrivate {

// Name of AppHost context item pass throw nodes
constexpr TStringBuf HW_SCENE_ITEM = "hw_selected_scene";

struct THandlerInfo {
    TString HandlerName;       // Internal handler name
    const TScenario* Scenario; // Pointer to scenario handler
};

class TScenarioFactory {
public:
    // Instance of TScenarioFactory
    static TScenarioFactory& Instance() {
        static TScenarioFactory factory;
        return factory;
    }

    // For internal usage only
    // Register Scenario initialization function
    void AttachRegitrator(IScenarioRegistratorBase* obj) {
        ScenarioRegistrators_.push_back(obj);
    }

    // For internal usage only
    // Called automatically from TScenario::TScenario()
    void RegisterScenario(TScenario* sc);

    // For internal usage only
    // Called automatically from TScenario::~TScenario()
    void UnRegisterScenario(TScenario* sc);

    // For internal usage only
    // Create all scenarios from ScenarioRegistrators_
    void CreateAllScenarios();
    void DestroyAllScenarios();

    // For internal usage only
    // Finalize initialization stage and clear 'Scenario inside ctor' flag
    void FinishInitialization(const NNlg::ITranslationsContainerPtr& nlgTranslations, const TFsPath& resourcesBasePath) {
        for (auto& it : AllScenarios_) {
            it.second->FinishInitialization(nlgTranslations, resourcesBasePath);
        }
    }
    // For internal usage only inside unit tests
    // Initialize scenario, created as local variable
    void FinishInitialization(TScenario& localScenarioVar) {
        localScenarioVar.FinishInitialization(nullptr, "");
    }

    // For internal usage only
    void RegisterAllFastData(NHollywood::TFastData& fastData) const;
    void CollectAllScenariosHandlers(TVector<THandlerInfo>& allHandlers);
    const TScenario* FindScenarioName(const TString& name) const;
    bool LinkOldScenario(const NHollywood::TScenario& oldScenario, const TString& handleName);
    void DispatchScenarioHandle(const TScenario& sc, const TString& nodeName, NHollywood::TGlobalContext& globalContext, NAppHost::IServiceContext& ctx);
    TString DumpScenarios(const TSet<TString>& oldScenarios, const TSet<TString>& missedScenarios) const;

    //
    // For unit testing only
    //
    void DispatchScenarioHandleUt(const TTestEnvironment& testData, TNodeCaller& nodeCaller, NHollywood::IGlobalContext& globalContext);
    void EnsureScenarioInitialization(const TString& scenarioName);
    const TRunRequest& CreateRunRequest(const TTestEnvironment& testData);
    bool IsHollywoodApp() const {
        return IsHollywoodApp_;
    }

private:
    // List of all functions to reguster scenarios
    TVector<IScenarioRegistratorBase*> ScenarioRegistrators_;
    // List of all registered scenarios
    THashMap<TString, TScenario*> AllScenarios_;
    // Hollywood app / Unit test mode flag
    bool IsHollywoodApp_ = false;

private:
    // Call a current node
    // Internal function, called from DispatchScenarioHandle()/DispatchScenarioHandleUt()
    bool DispatchScenarioHandleRun(const TScenario& sc,
                                   const TString& nodeName,
                                   NHollywood::TGlobalContext& globalContext,
                                   NAppHost::IServiceContext& ctx,
                                   const NScenarios::TRequestMeta& meta,
                                   TRTLogger& logger);
    bool DispatchScenarioHandleCca(const TScenario& sc,
                                   const TString& nodeName,
                                   NHollywood::TGlobalContext& globalContext,
                                   NAppHost::IServiceContext& ctx,
                                   const NScenarios::TRequestMeta& meta,
                                   TRTLogger& logger,
                                   ENodeType nodeType);
    void CallLocalGraph(TNodeCaller& callerData, ui64 seed, NMetrics::ISensors& s);
    void CheckSwitchOldFlow(TNodeCaller& callerData, NHollywood::TGlobalContext& globalContext);
};

} // namespace NAlice::NHollywoodFw::NPrivate
