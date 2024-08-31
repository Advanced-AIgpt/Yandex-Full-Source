//
// HOLLYWOOD FRAMEWORK
// Internal class : local scene graph implementation
//

#include "scene_graph.h"

#include "scenario_baseinit.h"

namespace NAlice::NHollywoodFw::NPrivate {


//
// Local structure to declare all possible graph compinators
//
struct TGraphAllVersions {
    ENodeType GraphType;
    TVector<TVector<EStageDeclaration>> NodeCallerObjects;

    bool Match(ENodeType graphType, const TScenarioGraphFlow& flow) const {
        if (graphType != GraphType || flow.size() != NodeCallerObjects.size() + 2) {
            return false;
        }
        // Check all objects in flow.GraphEntries_ and 1st objects in NodeCallerObjects (in reverse order,
        // without bordering elements)
        for (size_t i = 0; i < NodeCallerObjects.size(); i++) {
            if (flow.AtReverse(i).InitialStage != NodeCallerObjects[i][0]) {
                return false;
            }
        }
        return true;
    }

    void Build(TVector<TApphostNode>& graph, const TScenarioGraphFlow& flow) const {
        for (size_t i = 0; i < NodeCallerObjects.size(); i++) {
            const TScenarioGraphFlow::TLocalGraphNode& node = flow.AtReverse(i);
            TApphostNode appHostNode;
            appHostNode.NodeName = node.NodeName;
            appHostNode.ExpFlag = node.ExpFlag;
            appHostNode.NodeType = GraphType;
            if (i == 0) {
                // For the first node in each local graph incoming arguments are
                // - absent (if this is a 'run' node (1t call))
                // - come from MM scenario arguments
                appHostNode.ProtoSource = (GraphType == ENodeType::Run ? EHwProtoSource::None : EHwProtoSource::Arguments);
            } else {
                // All other nodes should read data from apphost context
                appHostNode.ProtoSource = EHwProtoSource::HwSceneItem;
            }
            appHostNode.LocalGraphs = NodeCallerObjects[i];
            graph.push_back(appHostNode);
        }
    }
};

static const TVector<TGraphAllVersions>& GetAllPossibleGraphs() {
    static TVector<TGraphAllVersions> allPossibleVectors = {
        // 1 nodes (run): Dispatch + Main + Render
        {ENodeType::Run,      {{EStageDeclaration::Dispatch,
                                EStageDeclaration::SceneMainRun,
                                EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        },
        // 1 nodes (continue): Continue + Render
        {ENodeType::Continue, {{EStageDeclaration::SceneMainContinue,
                                EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        },
        // 1 nodes (apply): Apply + Render
        {ENodeType::Apply,    {{EStageDeclaration::SceneMainApply,
                                EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        },
        // 1 nodes (commit): Commit
        {ENodeType::Commit,   {{EStageDeclaration::SceneMainCommit,
                                EStageDeclaration::Finalize}}
        },
        // 1 nodes (single renderer, not applicable for '/run'):
        {ENodeType::Continue, {{EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        },
        {ENodeType::Apply,  {{EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        },
        // 2 nodes (run): DispatchSetup, Dispatch + Main + Render
        {ENodeType::Run,      {{EStageDeclaration::DispatchSetup,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::Dispatch,
                                EStageDeclaration::SceneMainRun,
                                EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        },
        // 2 nodes (run): Dispatch + Main, Render
        {ENodeType::Run,      {{EStageDeclaration::Dispatch,
                                EStageDeclaration::SceneMainRun,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        },
        // 2 nodes (run): Dispatch + MainSetup, Main + Render
        {ENodeType::Run,      {{EStageDeclaration::Dispatch,
                                EStageDeclaration::SceneSetupRun,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::SceneMainRun,
                                EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        },
        // 2 nodes (continue): ContinueSetup, Continue + Render
        {ENodeType::Continue, {{EStageDeclaration::SceneSetupContinue,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::SceneMainContinue,
                                EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        },
        // 2 nodes (continue): Continue, Render
        {ENodeType::Continue, {{EStageDeclaration::SceneMainContinue,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        },
        // 2 nodes (apply): ApplySetup, Apply + Render
        {ENodeType::Apply,    {{EStageDeclaration::SceneSetupApply,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::SceneMainApply,
                                EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        },
        // 2 nodes (apply): Apply, Render
        {ENodeType::Apply,    {{EStageDeclaration::SceneMainApply,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        },
        // 2 nodes (commit): CommitSetup, Commit
        {ENodeType::Commit,   {{EStageDeclaration::SceneSetupCommit,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::SceneMainCommit,
                                EStageDeclaration::Finalize}}
        },
        // 3 nodes (run): DispatchSetup, Dispatch + MainSetup, Main + Render
        {ENodeType::Run,      {{EStageDeclaration::DispatchSetup,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::Dispatch,
                                EStageDeclaration::SceneSetupRun,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::SceneMainRun,
                                EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        },
        // 3 nodes (run): DispatchSetup, Dispatch + Main, Render
        {ENodeType::Run,      {{EStageDeclaration::DispatchSetup,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::Dispatch,
                                EStageDeclaration::SceneMainRun,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        },
        // 3 nodes (run): Dispatch + MainSetup, Main, Render
        {ENodeType::Run,      {{EStageDeclaration::Dispatch,
                                EStageDeclaration::SceneSetupRun,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::SceneMainRun,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        },
        // 3 nodes (continue): ContinueSetup, Continue, Render
        {ENodeType::Continue, {{EStageDeclaration::SceneSetupContinue,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::SceneMainContinue,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        },
        // 3 nodes (apply): ApplySetup, Apply, Render
        {ENodeType::Apply,    {{EStageDeclaration::SceneSetupApply,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::SceneMainApply,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        },
        // 4 nodes (run): DispatchSetup, Dispatch + MainSetup, Main, Render
        {ENodeType::Run,      {{EStageDeclaration::DispatchSetup,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::Dispatch,
                                EStageDeclaration::SceneSetupRun,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::SceneMainRun,
                                EStageDeclaration::Bypass},
                                {EStageDeclaration::Render,
                                EStageDeclaration::Finalize}}
        }
    };
    return allPossibleVectors;
}

/*
    Classify current stage by type:
        - Dispatcher functions (setup, dispatch)
        - Scene functuons (setu, main)
        - Renderer functions
        - Internal functions
*/
TScenarioGraphs::EStageType TScenarioGraphs::GetStageType(EStageDeclaration stage) {
    switch (stage) {
        case EStageDeclaration::DispatchSetup:
            return EStageType::ScenarioSetup;
        case EStageDeclaration::Dispatch:
            return EStageType::ScenarioDispatch;
        case EStageDeclaration::SceneSetupRun:
        case EStageDeclaration::SceneSetupContinue:
        case EStageDeclaration::SceneSetupApply:
        case EStageDeclaration::SceneSetupCommit:
            return EStageType::SceneSetup;
        case EStageDeclaration::SceneMainRun:
        case EStageDeclaration::SceneMainContinue:
        case EStageDeclaration::SceneMainApply:
        case EStageDeclaration::SceneMainCommit:
            return EStageType::SceneMain;
        case EStageDeclaration::Render:
            return EStageType::Renderer;
        case EStageDeclaration::Bypass:
        case EStageDeclaration::Finalize:
            return EStageType::Internal;
        case EStageDeclaration::Undefined:
            break;
    }
    return EStageType::Unknown;
}

/*
    Convert array of TScenarioGraphFlow into
    The vector flow.GraphEntries_ contains all registered nodes in reverse order, i.e.
       - NAME_FOR_FINAL_RESPONSE (Undefined)
       - main (SceneMainRun)
       - run (DispatchSetup)
       - NAME_FOR_INITIAL_REQUEST (Undefined)
*/
void TScenarioGraphs::AttachGraph(const TScenarioGraphFlow& flow) {
    // Min length for each graph is 3 node
    Y_ENSURE(flow.size() >= 3, "Too small graph declaration");
    const auto& firstNode = flow.First();
    const auto& lastNode = flow.Last();
    Y_ENSURE(lastNode.NodeName == NAME_FOR_FINAL_RESPONSE, "Invalid or unsupported node name for local graph");
    ENodeType graphType;
    if (firstNode.NodeName == NAME_FOR_INITIAL_REQUEST) {
        graphType = ENodeType::Run;
    } else if (firstNode.NodeName == NAME_FOR_CONTINUE) {
        graphType = ENodeType::Continue;
    } else if (firstNode.NodeName == NAME_FOR_APPLY) {
        graphType = ENodeType::Apply;
    } else if (firstNode.NodeName == NAME_FOR_COMMIT) {
        graphType = ENodeType::Commit;
    } else {
        Y_ENSURE(false, "Invalid or unsupported node name for local graph");
    }

    const auto& all = GetAllPossibleGraphs();
    for (const auto& it : all) {
        if (it.Match(graphType, flow)) {
            it.Build(ScenarioGraph_, flow);
            return;
        }
    }

    TStringBuilder sb;
    sb << "Invalid flow.";
    for (size_t i = 0; i < flow.size(); i++) {
        const auto& entry = flow.At(i);
        sb << " Node: " << entry.NodeName << "; Exp: '" << entry.ExpFlag << "'; stage: " << entry.InitialStage << ".";
    }
    Y_ENSURE(false, sb);
}

/*
    Build all collected graphs and validate errors
*/
void TScenarioGraphs::BuildAllGraphs(const TVector<EStageDeclaration>& stages, bool bDebugGraph) {
    // Check valid names
    for (const auto& itGraphs : ScenarioGraph_) {
        TString testName(itGraphs.NodeName);
        Y_ENSURE(!testName.to_lower(), "Apphost nodename must be lower_case: '" << itGraphs.NodeName << '\'');
    }

    // Check multiple / different expFlags
    for (size_t i = 0; i < ScenarioGraph_.size(); ++i) {
        for (size_t j = i + 1; j < ScenarioGraph_.size(); ++j) {
            if (ScenarioGraph_[i].NodeName == ScenarioGraph_[j].NodeName) {
                Y_ENSURE(ScenarioGraph_[i].ExpFlag != ScenarioGraph_[j].ExpFlag, "Two apphost nodes '" <<
                         ScenarioGraph_[i].NodeName << "' have the same experiments definition: '" <<
                         ScenarioGraph_[i].ExpFlag << '\'');
            }
        }
        if (ScenarioGraph_[i].ExpFlag) {
            const auto& nodeName = ScenarioGraph_[i].NodeName;
            Y_ENSURE(FindIfPtr(ScenarioGraph_, [nodeName](const auto& gph) -> bool {
                return nodeName == gph.NodeName && gph.ExpFlag.Empty();
            }), "Apphost node '" << nodeName << "' has experiment but default path doesn't exist");
        }
    }


    // Check that all scenarios and scenes objects exist in graphs
    TVector<EStageDeclaration> stagesCopy = stages;
    for (auto it = stagesCopy.begin(); it != stagesCopy.end(); ++it) {
        if (*it != EStageDeclaration::Undefined) {
            for (const auto& itGraphs : ScenarioGraph_) {
                if (itGraphs.Contains(*it)) {
                    *it = EStageDeclaration::Undefined;
                }
            }
        }
    }
    if (bDebugGraph) {
        // Output information about all found graphs and nodes in the scenario
        for (const auto& it : ScenarioGraph_) {
            it.DebugDump();
        }
    }

    for (const auto& it : stagesCopy) {
        Y_ENSURE(it == EStageDeclaration::Undefined,
                 "The current scenario graph is not compatible with declared functions. Missing function: " << it <<
                 ". Adjust local scenario graph with SetApphostGraph() call.");
    }
}

/*
    Find a local graph for incoming apphost node
*/
const TApphostNode* TScenarioGraphs::FindLocalGraph(const TString& apphostNode, const TRequest::TFlags* flags) const {
    //
    // 1st stage - find a scene graph with specified experiment flags
    //
    if (flags) {
        const TApphostNode* graph = FindIfPtr(ScenarioGraph_, [apphostNode, flags](const auto& graphNode) -> bool {
            if (graphNode.NodeName == apphostNode) {
                // Found, checking experiments
                if (graphNode.ExpFlag && flags->IsExperimentEnabled(graphNode.ExpFlag)) {
                    return true;
                }
            }
            return false;
        });
        if (graph != nullptr) {
            return graph;
        }
    }
    //
    // 2nd stage - find a scene graph without experiments
    //
    return FindIfPtr(ScenarioGraph_, [apphostNode](const auto& graphNode) -> bool {
        if (!graphNode.ExpFlag && graphNode.NodeName == apphostNode) {
             return true;
        }
        return false;
    });
}

const TApphostNode* TScenarioGraphs::FindLocalGraph(EStageDeclaration stage) const {
    for (const auto& it : ScenarioGraph_) {
        if (it.Contains(stage) && it.ExpFlag.Empty()) {
            return &it;
        }
    }
    return nullptr;
}

/*
    Dump detailed information about apphost and local graphs
*/
void TApphostNode::DebugDump() const {
    Cout << "  Apphost node: '/" << NodeName <<
        (ExpFlag ? "'; Experiment: " : "'") << ExpFlag << Endl;
    for (const auto& it : LocalGraphs) {
        Cout << "    Local graph node: " << it << Endl;
    }
}

} // namespace NAlice::NHollywoodFw::NPrivate
