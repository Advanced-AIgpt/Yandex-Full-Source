#pragma once

//
// HOLLYWOOD FRAMEWORK
// Internal class : local scene graph implementation
//
#include "request.h"
#include "scenario.h"

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywoodFw::NPrivate {

constexpr TStringBuf NAME_FOR_INITIAL_REQUEST = "*mm_request";
constexpr TStringBuf NAME_FOR_FINAL_RESPONSE = "*mm_response";
constexpr TStringBuf NAME_FOR_CONTINUE = "*mm_continue";
constexpr TStringBuf NAME_FOR_APPLY  = "*mm_apply";
constexpr TStringBuf NAME_FOR_COMMIT = "*mm_commit";

//
// Define where we can find TProtoHwScene
//
enum class EHwProtoSource {
    None,           // This is a first call from MM, TProtoHwScene is absent
    HwSceneItem,    // This is a next call in local scenario graph, TProtoHwScene can be found in apphost context
    Arguments       // This is a next call from MM, TProtoHwScene should be extrated from MM arguments
};

//
// Definition for single Apphost node
//
struct TApphostNode {
    // apphost node name (i.e. "run", "main", etc)
    TString NodeName;
    // optional scenario flag for this node
    TString ExpFlag;
    // Type of node
    ENodeType NodeType;
    // Type how to extract HW protobuf
    EHwProtoSource ProtoSource;
    // list of all local nodes inside the current node
    TVector<EStageDeclaration> LocalGraphs;

    EStageDeclaration At(size_t index) const {
        if (index >= LocalGraphs.size()) {
            return EStageDeclaration::Undefined;
        }
        return LocalGraphs[index];
    }
    bool Contains(EStageDeclaration stage) const {
        for (const auto it : LocalGraphs) {
            if (it == stage) {
                return true;
            }
        }
        return false;
    }
    void DebugDump() const;
};

//
// Manage all graphs in the current scenario
//
class TScenarioGraphs {
public:
    enum class EStageType {
        Unknown,
        ScenarioSetup,
        ScenarioDispatch,
        SceneSetup,
        SceneMain,
        Renderer,
        Internal
    };
    static EStageType GetStageType(EStageDeclaration stage);

    void AttachGraph(const TScenarioGraphFlow& flow);
    void BuildAllGraphs(const TVector<EStageDeclaration>& stages, bool bDebugGraph);

    // Find a descriptor for apphost node with optional flags
    const TApphostNode* FindLocalGraph(const TString& apphostNode, const TRequest::TFlags* flags) const;
    // Find a descriptor for apphost node by EStageDeclaration (tests only)
    const TApphostNode* FindLocalGraph(EStageDeclaration stage) const;
    // Get a direct access to scenario graph (used for testing purposes and to attach all handlers to grpc)
    const TVector<TApphostNode>& GetAllGraphs() const {
        return ScenarioGraph_;
    }

private:
    TVector<TApphostNode> ScenarioGraph_;
};

} // namespace NAlice::NHollywoodFw::NPrivate
