#pragma once

#include "movie_base.h"

#include <alice/hollywood/library/nlg/nlg_data.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/suggesters/movie_akinator/proto/movie_akinator_state.pb.h>

#include <alice/library/logger/logger.h>
#include <alice/library/util/rng.h>
#include <alice/megamind/protos/analytics/scenarios/general_conversation/general_conversation.pb.h>
#include <alice/megamind/protos/common/frame.pb.h>

#include <util/generic/array_ref.h>
#include <util/generic/hash_set.h>

namespace NAlice::NHollywood {

struct TAkinatorState {
    bool WasHelpShown = false;
    ui64 NodeId = 0;
    ui64 LeftChildId = 0;
    ui64 RightChildId = 0;
    THashSet<TString> ShownHelpActions;

    template <typename TProtoState>
    TProtoState ToProto() {
        TProtoState state;

        state.SetWasHelpShown(WasHelpShown);
        state.SetNodeId(NodeId);
        state.SetLeftChildId(LeftChildId);
        state.SetRightChildId(RightChildId);

        auto& shownHelpActions = *state.MutableShownHelpActions();
        shownHelpActions.Reserve(ShownHelpActions.size());
        for (const auto& element : ShownHelpActions) {
            *shownHelpActions.Add() = element;
        }

        return state;
    }

    template <typename TProtoState>
    static TAkinatorState FromProto(const TProtoState& state) {
        const auto& shownHelpActions = state.GetShownHelpActions();
        return {
            .WasHelpShown = state.GetWasHelpShown(),
            .NodeId = state.GetNodeId(),
            .LeftChildId = state.GetLeftChildId(),
            .RightChildId = state.GetRightChildId(),
            .ShownHelpActions = THashSet<TString>{shownHelpActions.begin(), shownHelpActions.end()}
        };
    }
};

struct TAkinatorSuggestsState {
    struct TSuggest {
        TString SuggestId;
        TString SuggestTitle;
        bool HasPredefinedEffect = false;
    };

    TVector<TSuggest> Suggests;
};

struct TAkinatorStateWrapper {
    TAkinatorState BaseState;
    TAkinatorSuggestsState* SuggestsState;
    TMaybe<TString> DiscussableEntityId;

    TAkinatorStateWrapper(TAkinatorState baseState, TAkinatorSuggestsState* suggestsState = nullptr)
        : BaseState(std::move(baseState))
        , SuggestsState(suggestsState)
    {}
};

TMaybe<TFrame> LoadAkinatorSemanticFrame(const TScenarioInputWrapper& requestInput);

void BuildIrrelevantResponse(TRunResponseBuilder& builder, TResponseBodyBuilder& bodyBuilder, TNlgData& nlgData);

class TAkinatorResponseBuilder {
public:
    TAkinatorResponseBuilder(const TClusteredMovies& clusteredMovies,
                             const NAlice::TSemanticFrame& semanticFrame,
                             const TScenarioRunRequestWrapper& request, TAkinatorStateWrapper& state,
                             TNlgData& nlgData, TNlgWrapper& nlgWrapper,
                             TRunResponseBuilder& builder, TRTLogger& logger, IRng& rng);

    void Build();

    const NScenarios::NGeneralConversation::TMovieAkinatorResponseInfo& GetResponseAnalyticsInfo() const {
        return ResponseAnalyticsInfo;
    }

private:
    const TClusteredMovies& ClusteredMovies;
    NAlice::TSemanticFrame SemanticFrame;
    TFrame Frame;
    bool UseGcMovieDiscussWeak = false;

    TAkinatorStateWrapper& State;
    TNlgData& NlgData;
    TNlgWrapper& NlgWrapper;
    TRunResponseBuilder& Builder;
    TResponseBodyBuilder& BodyBuilder;
    NScenarios::NGeneralConversation::TMovieAkinatorResponseInfo ResponseAnalyticsInfo;
    TRTLogger& Logger;
    IRng& Rng;

private:
    void BuildResponse(const TFilterCondition& filterCondition);

    void BuildShowDescriptionResponse(const TSlot& showDescriptionSlot, bool renderCards);
    void AddOpenOnKinopoiskSuggest(const TMovieInfo& movieInfo);
    const TMovieInfo* GetShowDescriptionMovieInfo(const TSlot& showDescriptionSlot) const;

    void BuildFeedbackResponse(const TSlot& feedbackSlot);

    void BuildResponseByTree(const TTree& tree, bool renderCards);

    void LoadCurrentNodes(const TTree& tree, const TNode*& node, const TNode*& leftChild, const TNode*& rightChild);

    void LoadCurrentNodesFromState(const TTree& tree, const TNode*& node,
                                   const TNode*& leftChild, const TNode*& rightChild);
    void UpdateCurrentNodesInState(const TNode* node, const TNode* leftChild, const TNode* rightChild);

    size_t LoadCurrentNodeIndex(const TTree& tree);
    void GetNextNodes(const TTree& tree, size_t nodeIndex, const TNode*& node,
                      const TNode*& leftChild, const TNode*& rightChild);
    void SampleNextNodes(const TTree& tree, const TNode*& node,
                         const TNode*& leftChild, const TNode*& rightChild);

    void BuildBinaryChoiceResponse(const TNode& leftChild, const TNode& rightChild, bool renderCards);
    void BuildBinaryChoiceResponseCards(const TNode& leftChild, const TNode& rightChild);
    void BuildBinaryChoiceActions(const TNode& leftChild, const TNode& rightChild);

    void BuildGalleryResponse(TArrayRef<const size_t> movieInfoIndices, const TNode* node, bool renderCards);
    void BuildGalleryResponseCards(const TArrayRef<const size_t>& movieInfoIndices);
    void BuildGalleryActions(const TArrayRef<const size_t>& movieInfoIndices, const TNode* node);

    void AddContentSuggest(const TFilterCondition& currentCondition);

    void AddChooseSimilarContentHandler();
    void AddFailedChooseSimilarContentAction();
    void AddChooseSimilarContentSuggest();

    void AddDiscussAction(const TArrayRef<const size_t>& movieInfoIndices);
    const TMovieInfo* SampleMovieToDiscuss(const TArrayRef<const size_t>& movieInfoIndices);

    void AddFeedbackSuggests(bool isShowDescription);
    void AddFeedbackSuggest(const TString& feedbackType, bool isShowDescription);

    void AddBegemotFilledFrameAction(const TStringBuf frameName, const TString& actionName);

    void AddContentInfoToAnalytics(const TFilterCondition& filterCondition);

    void GenerateActionHelp();
};

} // namespace NAlice::NHollywood
