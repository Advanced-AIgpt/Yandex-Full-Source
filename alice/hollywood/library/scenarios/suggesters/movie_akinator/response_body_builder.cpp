#include "response_body_builder.h"

#include "util/string/cast.h"

#include <alice/hollywood/library/scenarios/suggesters/common/utils.h>

#include <alice/hollywood/library/frame/callback.h>

#include <alice/library/json/json.h>
#include <alice/library/proto/proto.h>
#include <alice/library/proto/protobuf.h>
#include <alice/library/video_common/defs.h>

#include <util/generic/hash_set.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywood {

namespace {

constexpr size_t MAX_GALLERY_SIZE = 15;

constexpr TStringBuf CHOOSE_CONTENT_FRAME = "alice.movie_akinator.choose_content";
constexpr TStringBuf CHOOSE_SIMILAR_CONTENT_FRAME = "alice.movie_akinator.choose_similar_content";
constexpr TStringBuf CHOOSE_SIMILAR_CONTENT_BY_SUGGEST_FRAME = "alice.movie_akinator.choose_similar_content_by_suggest";
constexpr TStringBuf CHOOSE_UNRECOGNIZED_CONTENT_FRAME = "alice.movie_akinator.choose_unrecognized_content";
constexpr TStringBuf RECOMMEND_FRAME = "alice.movie_akinator.recommend";
constexpr TStringBuf RESET_FRAME = "alice.movie_akinator.do_reset";
constexpr TStringBuf SHOW_DESCRIPTION_FRAME = "alice.movie_akinator.show_description";
constexpr TStringBuf DISCUSS_FRAME = "alice.general_conversation.akinator_movie_discuss";
// TODO(dan-anastasev) Left for backward compatibility, remove me
constexpr TStringBuf RECOMMEND_OLD_FRAME = "alice.movie_akinator";

const TVector<TStringBuf> ACCEPTED_FRAMES = {
    CHOOSE_CONTENT_FRAME,
    CHOOSE_SIMILAR_CONTENT_FRAME,
    CHOOSE_SIMILAR_CONTENT_BY_SUGGEST_FRAME,
    RECOMMEND_FRAME,
    SHOW_DESCRIPTION_FRAME,
    RESET_FRAME,
    RECOMMEND_OLD_FRAME,
    CHOOSE_UNRECOGNIZED_CONTENT_FRAME
};

constexpr TStringBuf NLG_TEMPLATE = "movie_akinator";

constexpr TStringBuf RENDER_DESCRIPTION = "render_description";
constexpr TStringBuf RENDER_INVALID_MOVIE_ID = "render_invalid_movie_id";
constexpr TStringBuf RENDER_IRRELEVANT = "render_irrelevant";
constexpr TStringBuf RENDER_POSTER_CLOUD = "render_poster_cloud";
constexpr TStringBuf RENDER_POSTER_GALLERY = "render_poster_gallery";
constexpr TStringBuf RENDER_UNRECOGNIZED_MOVIE = "render_unrecognized";
constexpr TStringBuf RENDER_FEEDBACK = "render_feedback";

constexpr TStringBuf DIV_CARDS_TEMPLATE = "movie_akinator_div_cards";
constexpr TStringBuf POSTER_CLOUD_CARD = "poster_cloud";
constexpr TStringBuf POSTER_GALLERY_CARD = "poster_gallery";

constexpr TStringBuf SLOT_FILM_ID = "film_id";
constexpr TStringBuf SLOT_NODE_HASH = "node_hash";
constexpr TStringBuf SLOT_NODE_ID = "node_id";
constexpr TStringBuf SLOT_SHOW_DESCRIPTION = "show_description";
constexpr TStringBuf SLOT_SHOW_GALLERY = "show_gallery";
constexpr TStringBuf SLOT_FEEDBACK = "feedback";

const TVector<TString> SHOW_DESCRIPTION_HINT_TEMPLATES = {"О чем ", "Покажи описание ", "Про что ", "Описание "};

struct TNluHint {
    TMaybe<TString> HintText;
    TMaybe<TString> FrameName;
};

const TString CHOOSE_LEFT_ACTION = "choose_left";
const TNluHint CHOOSE_LEFT_HINT = {.HintText = "Левые"};

const TString CHOOSE_RIGHT_ACTION = "choose_right";
const TNluHint CHOOSE_RIGHT_HINT = {.HintText = "Правые"};

const TString RESET_ACTION = "reset";
const TString RESET_GRANET_NAME = "alice.movie_akinator.reset";
const TNluHint SHOW_OTHER_HINT = {.HintText = "Покажи другие", .FrameName = RESET_GRANET_NAME};
const TNluHint RESET_HINT = {.HintText = "Начать сначала", .FrameName = RESET_GRANET_NAME};

const TString SHOW_DESCRIPTION_ACTION = "show_description";

const TString OPEN_ACTION = "open";
const TString OPEN_HINT = "Открой на кинопоиске";

const TString SUGGEST_CONTENT_ACTION = "suggest_content";
const TString CHOOSE_CONTENT_ACTION = "choose_content";

const TString SHOW_GALLERY_FOR_LEFT_ACTION = "show_gallery_for_left";
const TNluHint SHOW_GALLERY_FOR_LEFT_HINT = {.HintText = "Покажи галерею из левых"};

const TString SHOW_GALLERY_FOR_RIGHT_ACTION = "show_gallery_for_right";
const TNluHint SHOW_GALLERY_FOR_RIGHT_HINT = {.HintText = "Покажи галерею из правых"};

const TString CHOOSE_SIMILAR_CONTENT_ACTION = "choose_similar_content";
const TString CHOOSE_SIMILAR_CONTENT_BY_SUGGEST_ACTION = "choose_similar_content_by_suggest";
const TString FAILED_CHOOSE_SIMILAR_CONTENT_ACTION = "failed_choose_similar_content";

const TString DISCUSS_ACTION = "discuss_action";

const THashSet<TString> COPY_SLOTS = {
    TString{NVideoCommon::SLOT_CONTENT_TYPE},
    TString{NVideoCommon::SLOT_GENRE},
};

const THashSet<TString> COPY_SLOTS_FOR_SHOW_DESCRIPTION = {
    TString{NVideoCommon::SLOT_CONTENT_TYPE},
    TString{NVideoCommon::SLOT_GENRE},
    TString{SLOT_SHOW_GALLERY},
};

const THashSet<TString> COPY_SLOTS_SHOW_DESCRIPTION_RESET_ON_NEGATIVE_FEEDBACK = {
    TString{NVideoCommon::SLOT_CONTENT_TYPE},
    TString{NVideoCommon::SLOT_GENRE},
    TString{SLOT_SHOW_GALLERY},
    TString{SLOT_NODE_ID},
    TString{SLOT_NODE_HASH},
};

const TVector<TString> CONTENT_SUGGEST_PREFIXES = {
    "Посоветуй ", "Порекомендуй ", "Покажи ", "Найди ", "Давай ", "Хочу "
};

const TVector<TString> CHOOSE_SIMILAR_SUGGEST_PREFIXES = {
    "Посоветуй фильмы похожие на «", "Порекомендуй фильмы в стиле «", "Хочу кино похожее на «"
};

constexpr double SHOW_CHOOSE_SIMILAR_CONTENT_SUGGEST_PROBABILITY = 0.2;
constexpr double SHOW_CONTENT_SUGGEST_PROBABILITY = 0.2;
constexpr double SHOW_DIRECTION_SUGGEST_PROBABILITY = 0.1;
constexpr double SHOW_GALLERY_SUGGEST_PROBABILITY = 0.15;
constexpr double SHOW_DISCUSS_SUGGEST_PROBABILITY = 0.75;
constexpr double SHOW_FEEDBACK_VOICE_PROBABILITY = 0.3;

constexpr size_t DISCUSSABLE_MOVIE_TOP = 5;
constexpr size_t CHOOSE_SIMILAR_CONTENT_SUGGEST_MOVIE_TOP = 250;

constexpr TStringBuf EXP_GC_MOVIE_DISCUSS_WEAK_ACTION = "hw_akinator_gc_movie_discuss_weak";

const TString DIRECTIONS_HELP_ACTION = "directions_action_help";

const TVector<TString> HELP_ACTIONS = {
    DIRECTIONS_HELP_ACTION, // Has the highest priority, should be the first
    "choose_content_action_help",
    "choose_similar_content_action_help",
    "reset_action_help",
    "show_description_action_help",
    "show_discuss_action_help",
    "show_gallery_action_help",
};

constexpr double REPEAT_HELP_ACTION_PROBABILITY = 0.05;

constexpr TStringBuf POSITIVE_FEEDBACK = "positive";
constexpr TStringBuf NEGATIVE_FEEDBACK = "negative";

TFilterCondition GetFilterCondition(const TFrame& frame) {
    TFilterCondition condition;

    if (const auto slot = frame.FindSlot(NVideoCommon::SLOT_CONTENT_TYPE)) {
        condition.ExpectedContentType = slot->Value.AsString();
    }
    if (const auto slot = frame.FindSlot(NVideoCommon::SLOT_GENRE)) {
        condition.ExpectedGenre = slot->Value.AsString();

        if (!condition.ExpectedContentType) {
            condition.ExpectedContentType = GetContentTypeByGenre(condition.ExpectedGenre);
        }
    }

    return condition;
}

void SampleIndicesPair(size_t limit, IRng& rng, size_t& first, size_t& second) {
    first = rng.RandomInteger(limit);

    second = rng.RandomInteger(limit - 1);
    if (first == second) {
        second = limit - 1;
    }
}

void AddNodeSlots(const TNode* node, NAlice::TSemanticFrame& frame) {
    if (!node) {
        return;
    }

    auto& nodeIdSlot = *frame.AddSlots();
    nodeIdSlot.SetName(TString{SLOT_NODE_ID});
    nodeIdSlot.SetType("string");
    nodeIdSlot.SetValue(ToString(node->Index));

    auto& nodeHashSlot = *frame.AddSlots();
    nodeHashSlot.SetName(TString{SLOT_NODE_HASH});
    nodeHashSlot.SetType("string");
    nodeHashSlot.SetValue(ToString(node->Hash));
}

NScenarios::TFrameAction BuildTypeTextChooseAction(const TString& title, const TString& actionId,
                                                   const NAlice::NScenarios::TCallbackDirective& callback)
{
    NScenarios::TFrameAction action;
    action.MutableNluHint()->SetFrameName(actionId);
    action.MutableDirectives()->AddList()->MutableTypeTextSilentDirective()->SetText(title);

    *action.MutableDirectives()->AddList()->MutableCallbackDirective() = callback;

    return action;
}

NScenarios::TFrameAction BuildChooseAction(const NAlice::TSemanticFrame& requestFrame, const TMaybe<TString>& hintText,
                                           const TString& frameName, const TNode* node,
                                           const NAlice::TSemanticFrame::TSlot* addSlot,
                                           const TMaybe<TStringBuf>& frameEffectName)
{
    NScenarios::TFrameAction action;

    auto& nluHint = *action.MutableNluHint();

    if (hintText) {
        TNluPhrase& nluPhrase = *nluHint.AddInstances();
        nluPhrase.SetLanguage(ELang::L_RUS);
        nluPhrase.SetPhrase(*hintText);
    }

    nluHint.SetFrameName(frameName);

    const TStringBuf resultFrameEffectName = frameEffectName ? *frameEffectName : RECOMMEND_FRAME;
    auto frame = InitCallbackFrameEffect(requestFrame, TString{resultFrameEffectName}, COPY_SLOTS);
    AddNodeSlots(node, frame);

    if (addSlot) {
        *frame.AddSlots() = *addSlot;
    }

    *action.MutableCallback() = ToCallback(frame);

    return action;
}

void AddChooseAction(const NAlice::TSemanticFrame& requestFrame,
                     const TString& actionId, const TNluHint& hint,
                     const TNode* node, TResponseBodyBuilder& bodyBuilder,
                     TAkinatorSuggestsState* suggestsState, bool showSuggest = true,
                     const NAlice::TSemanticFrame::TSlot* addSlot = nullptr,
                     const TMaybe<TStringBuf>& frameEffectName = Nothing())
{
    Y_ENSURE(hint.HintText.Defined() || hint.FrameName.Defined());

    if (hint.HintText) {
        const auto typeTextActionId = actionId + "_by_button";

        auto action = BuildChooseAction(requestFrame, hint.HintText, actionId,
                                        node, addSlot, frameEffectName);
        auto typeTextAction = BuildTypeTextChooseAction(*hint.HintText, typeTextActionId, action.GetCallback());

        bodyBuilder.AddAction(actionId, std::move(action));
        bodyBuilder.AddAction(typeTextActionId, std::move(typeTextAction));

        if (showSuggest) {
            if (suggestsState) {
                suggestsState->Suggests.push_back({
                    .SuggestId = typeTextActionId,
                    .SuggestTitle = *hint.HintText,
                    .HasPredefinedEffect = true,
                });
            } else {
                bodyBuilder.AddActionSuggest(typeTextActionId).Title(*hint.HintText);
            }
        }
    }

    if (hint.FrameName) {
        auto action = BuildChooseAction(requestFrame, /* hintText= */ Nothing(), *hint.FrameName,
                                        node, addSlot, frameEffectName);
        bodyBuilder.AddAction(actionId + "_by_frame", std::move(action));
    }
}

void AddShowGalleryAction(const NAlice::TSemanticFrame& requestFrame,
                          const TString& actionId, const TNluHint& hint,
                          const TNode* node, TResponseBodyBuilder& bodyBuilder,
                          TAkinatorSuggestsState* suggestsState, IRng& rng)
{
    NAlice::TSemanticFrame::TSlot showGallerySlot;
    showGallerySlot.SetName(TString{SLOT_SHOW_GALLERY});
    showGallerySlot.SetType("string");
    showGallerySlot.SetValue("true");

    const bool showSuggest = rng.RandomDouble() < SHOW_GALLERY_SUGGEST_PROBABILITY;
    AddChooseAction(requestFrame, actionId, hint, node, bodyBuilder, suggestsState, showSuggest, &showGallerySlot);
}

NScenarios::TFrameAction BuildShowDescriptionAction(const TMovieInfo& movieInfo,
                                                    const TNode* node,
                                                    const NAlice::TSemanticFrame& requestFrame,
                                                    IRng& rng)
{
    NScenarios::TFrameAction action;

    auto& nluHint = *action.MutableNluHint();
    *nluHint.MutableFrameName() = SHOW_DESCRIPTION_ACTION + "_" + ToString(movieInfo.KinopoiskId);

    auto showDescriptionFrame = InitCallbackFrameEffect(requestFrame, TString{SHOW_DESCRIPTION_FRAME},
                                                        COPY_SLOTS_FOR_SHOW_DESCRIPTION);

    auto& slot = *showDescriptionFrame.AddSlots();
    slot.SetName(TString{SLOT_SHOW_DESCRIPTION});
    slot.SetType("string");
    slot.SetValue(movieInfo.OntoId);

    AddNodeSlots(node, showDescriptionFrame);

    const auto& prefix = SHOW_DESCRIPTION_HINT_TEMPLATES[rng.RandomInteger(SHOW_DESCRIPTION_HINT_TEMPLATES.size())];
    const auto suggestText = prefix + "«" + movieInfo.Name + "»?";
    action.MutableDirectives()->AddList()->MutableTypeTextSilentDirective()->SetText(suggestText);
    *action.MutableDirectives()->AddList()->MutableCallbackDirective() = ToCallback(showDescriptionFrame);

    return action;
}

void SerializeStateMovieIds(const TArrayRef<const size_t>& movieInfoIndices, const TVector<TMovieInfo>& movieInfos,
                            google::protobuf::RepeatedField<google::protobuf::uint64>& movieIds)
{
    for (size_t i = 0; i < Min(movieInfoIndices.size(), MAX_GALLERY_SIZE); ++i) {
        movieIds.Add(movieInfos[movieInfoIndices[i]].KinopoiskId);
    }
}

} // namespace

TMaybe<TFrame> LoadAkinatorSemanticFrame(const TScenarioInputWrapper& requestInput) {
    const TMaybe<TFrame> callbackFrame = GetCallbackFrame(requestInput.GetCallback());

    for (const auto& frameName : ACCEPTED_FRAMES) {
        if (callbackFrame && callbackFrame->Name() == frameName) {
            return callbackFrame;
        }
        if (const auto semanticFramePtr = requestInput.FindSemanticFrame(frameName)) {
            return TFrame::FromProto(*semanticFramePtr);
        }
    }
    return Nothing();
}

void BuildIrrelevantResponse(TRunResponseBuilder& builder, TResponseBodyBuilder& bodyBuilder, TNlgData& nlgData) {
    builder.SetIrrelevant();
    bodyBuilder.AddRenderedText(NLG_TEMPLATE, RENDER_IRRELEVANT, nlgData);
}

TAkinatorResponseBuilder::TAkinatorResponseBuilder(const TClusteredMovies& clusteredMovies,
                                                   const NAlice::TSemanticFrame& semanticFrame,
                                                   const TScenarioRunRequestWrapper& request, TAkinatorStateWrapper& state,
                                                   TNlgData& nlgData, TNlgWrapper& nlgWrapper,
                                                   TRunResponseBuilder& builder, TRTLogger& logger, IRng& rng)
    : ClusteredMovies(clusteredMovies)
    , SemanticFrame(semanticFrame)
    , Frame(TFrame::FromProto(SemanticFrame))
    , UseGcMovieDiscussWeak(request.ExpFlag(EXP_GC_MOVIE_DISCUSS_WEAK_ACTION))
    , State(state)
    , NlgData(nlgData)
    , NlgWrapper(nlgWrapper)
    , Builder(builder)
    , BodyBuilder(*builder.GetResponseBodyBuilder())
    , Logger(logger)
    , Rng(rng)
{
    NlgData.Context["is_ios"] = request.ClientInfo().IsIOS();;
}

void TAkinatorResponseBuilder::Build() {
    LOG_INFO(Logger) << "Semantic frame: " << SerializeProtoText(SemanticFrame);

    ResponseAnalyticsInfo.SetNodeId(NPOS);

    const auto filterCondition = GetFilterCondition(Frame);
    LOG_INFO(Logger) << "Filter condition: " << filterCondition;

    GenerateActionHelp();
    BuildResponse(filterCondition);

    AddContentSuggest(filterCondition);
    AddBegemotFilledFrameAction(/* frameName= */ CHOOSE_CONTENT_FRAME, /* actionName= */ CHOOSE_CONTENT_ACTION);
    AddChooseSimilarContentHandler();

    AddContentInfoToAnalytics(filterCondition);
}

void TAkinatorResponseBuilder::BuildResponse(const TFilterCondition& filterCondition) {
    bool renderCards = true;

    if (const auto feedbackSlot = Frame.FindSlot(SLOT_FEEDBACK)) {
        BuildFeedbackResponse(*feedbackSlot);
        renderCards = (feedbackSlot->Value.AsString() == NEGATIVE_FEEDBACK);
    }

    if (const auto showDescriptionSlot = Frame.FindSlot(SLOT_SHOW_DESCRIPTION)) {
        BuildShowDescriptionResponse(*showDescriptionSlot, renderCards);
        renderCards = false;
    } else if (Frame.Name() == CHOOSE_UNRECOGNIZED_CONTENT_FRAME) {
        BodyBuilder.AddRenderedTextWithButtonsAndVoice(/* nlgTemplateName = */ NLG_TEMPLATE,
                                                       /* phraseName = */ RENDER_UNRECOGNIZED_MOVIE,
                                                       /* buttons = */ {},
                                                       NlgData);
        renderCards = false;
    }

    if (const auto* treePtr = ClusteredMovies.Trees.FindPtr(filterCondition)) {
        LOG_INFO(Logger) << "Prerendered tree was found";

        BuildResponseByTree(*treePtr, renderCards);
    } else {
        LOG_INFO(Logger) << "Prerendered tree was not found, collecting a plain gallery";

        const auto movieInfoIndices = ClusteredMovies.GetFilteredMovieInfoIndices(filterCondition, MAX_GALLERY_SIZE);
        if (movieInfoIndices.empty()) {
            BuildIrrelevantResponse(Builder, BodyBuilder, NlgData);
            return;
        }

        BuildGalleryResponse(movieInfoIndices, /* node= */ nullptr, renderCards);
    }
}

void TAkinatorResponseBuilder::BuildShowDescriptionResponse(const TSlot& showDescriptionSlot, bool renderCards) {
    LOG_INFO(Logger) << "Building show description response";

    const auto* movieInfo = GetShowDescriptionMovieInfo(showDescriptionSlot);

    if (renderCards) {
        if (!movieInfo) {
            BodyBuilder.AddRenderedTextWithButtonsAndVoice(/* nlgTemplateName = */ NLG_TEMPLATE,
                                                           /* phraseName = */ RENDER_INVALID_MOVIE_ID,
                                                           /* buttons = */ {},
                                                           NlgData);
            ResponseAnalyticsInfo.MutableShowDescriptionState();
            return;
        }

        NlgData.Context["description"] = movieInfo->Description;
        BodyBuilder.AddRenderedTextWithButtonsAndVoice(/* nlgTemplateName = */ NLG_TEMPLATE,
                                                       /* phraseName = */ RENDER_DESCRIPTION,
                                                       /* buttons = */ {},
                                                       NlgData);
    }

    AddFeedbackSuggests(/* isShowDescription= */ true);
    AddOpenOnKinopoiskSuggest(*movieInfo);

    State.DiscussableEntityId = TString{"movie:"} + ToString(movieInfo->KinopoiskId);

    auto& analyticsInfo = *ResponseAnalyticsInfo.MutableShowDescriptionState();
    analyticsInfo.SetMovieId(movieInfo->KinopoiskId);
}

void TAkinatorResponseBuilder::AddOpenOnKinopoiskSuggest(const TMovieInfo& movieInfo) {
    NScenarios::TFrameAction action;

    auto& nluHint = *action.MutableNluHint();
    TNluPhrase& nluPhrase = *nluHint.AddInstances();
    nluPhrase.SetLanguage(ELang::L_RUS);
    nluPhrase.SetPhrase(OPEN_HINT);
    *nluHint.MutableFrameName() = OPEN_ACTION;

    auto& openUriDirective = *action.MutableDirectives()->AddList()->MutableOpenUriDirective();
    openUriDirective.SetName("movie_akinator_open_uri");
    openUriDirective.SetUri(movieInfo.OpenUrl);

    BodyBuilder.AddAction(OPEN_ACTION, std::move(action));
    if (State.SuggestsState) {
        State.SuggestsState->Suggests.push_back({
            .SuggestId = OPEN_ACTION,
            .SuggestTitle = OPEN_HINT,
            .HasPredefinedEffect = true,
        });
    } else {
        BodyBuilder.AddActionSuggest(OPEN_ACTION).Title(OPEN_HINT);
    }
}

const TMovieInfo* TAkinatorResponseBuilder::GetShowDescriptionMovieInfo(const TSlot& showDescriptionSlot) const {
    const auto movieOntoId = showDescriptionSlot.Value.AsString();
    if (const auto* movieIndex = ClusteredMovies.OntoIdToMovieInfoIndex.FindPtr(movieOntoId)) {
        return &ClusteredMovies.MovieInfos.at(*movieIndex);;
    }

    return nullptr;
}

void TAkinatorResponseBuilder::BuildFeedbackResponse(const TSlot& feedbackSlot) {
    NlgData.Context["attentions"]["feeback"] = feedbackSlot.Value.AsString();
    BodyBuilder.AddRenderedTextWithButtonsAndVoice(/* nlgTemplateName = */ NLG_TEMPLATE,
                                                   /* phraseName = */ RENDER_FEEDBACK,
                                                   /* buttons = */ {},
                                                   NlgData);
}

void TAkinatorResponseBuilder::BuildResponseByTree(const TTree& tree, bool renderCards) {
    const TNode* node = nullptr;
    const TNode* leftChild = nullptr;
    const TNode* rightChild = nullptr;

    LoadCurrentNodes(tree, node, leftChild, rightChild);

    const bool showGallery = Frame.FindSlot(SLOT_SHOW_GALLERY) != nullptr;
    const bool canShowBinaryChoice = leftChild && rightChild && leftChild->HasPosterCloud && rightChild->HasPosterCloud;

    if (!showGallery && canShowBinaryChoice) {
        BuildBinaryChoiceResponse(*leftChild, *rightChild, renderCards);
    } else {
        BuildGalleryResponse(node->MovieInfoIndices, node, renderCards);
    }

    UpdateCurrentNodesInState(node, leftChild, rightChild);

    if (node) {
        ResponseAnalyticsInfo.SetNodeId(node->Index);
    }
}

void TAkinatorResponseBuilder::LoadCurrentNodes(const TTree& tree, const TNode*& node,
                                                const TNode*& leftChild, const TNode*& rightChild)
{
    if (Frame.Name() == CHOOSE_UNRECOGNIZED_CONTENT_FRAME) {
        LoadCurrentNodesFromState(tree, node, leftChild, rightChild);
        return;
    }

    const size_t nodeIndex = LoadCurrentNodeIndex(tree);

    if (nodeIndex == NPOS) {
        SampleNextNodes(tree, node, leftChild, rightChild);
    } else {
        GetNextNodes(tree, nodeIndex, node, leftChild, rightChild);
    }
}

void TAkinatorResponseBuilder::LoadCurrentNodesFromState(const TTree& tree, const TNode*& node,
                                                         const TNode*& leftChild, const TNode*& rightChild)
{
    LOG_INFO(Logger) << "Loading current nodes from the state";

    if (State.BaseState.NodeId != NPOS) {
        node = &tree.Nodes[State.BaseState.NodeId];
    }
    if (State.BaseState.LeftChildId != NPOS) {
        leftChild = &tree.Nodes[State.BaseState.LeftChildId];
    }
    if (State.BaseState.RightChildId != NPOS) {
        rightChild = &tree.Nodes[State.BaseState.RightChildId];
    }
}

void TAkinatorResponseBuilder::UpdateCurrentNodesInState(const TNode* node, const TNode* leftChild,
                                                         const TNode* rightChild)
{
    State.BaseState.NodeId = node ? node->Index : NPOS;
    State.BaseState.LeftChildId = leftChild ? leftChild->Index : NPOS;
    State.BaseState.RightChildId = rightChild ? rightChild->Index : NPOS;
}

size_t TAkinatorResponseBuilder::LoadCurrentNodeIndex(const TTree& tree) {
    if (const auto filmIdSlot = Frame.FindSlot(SLOT_FILM_ID)) {
        const auto filmId = filmIdSlot->Value.AsString();

        if (const auto* nodeIndexPtr = tree.OntoIdToCluster.FindPtr(filmId)) {
            const auto movieInfoIndex = ClusteredMovies.OntoIdToMovieInfoIndex.at(filmId);
            NlgData.Context["attentions"]["choose_similar_content"] = ClusteredMovies.MovieInfos[movieInfoIndex].Name;
            ResponseAnalyticsInfo.SetShowSimilarMovieId(ClusteredMovies.MovieInfos[movieInfoIndex].KinopoiskId);
            return *nodeIndexPtr;
        }
    }

    const auto nodeIdSlot = Frame.FindSlot(SLOT_NODE_ID);
    const auto nodeHashSlot = Frame.FindSlot(SLOT_NODE_HASH);

    if (!nodeIdSlot || !nodeHashSlot) {
        Y_ENSURE(!nodeIdSlot && !nodeHashSlot);
        return NPOS;
    }

    const auto nodeId = nodeIdSlot->Value.As<size_t>();
    const auto nodeHash = nodeHashSlot->Value.As<ui64>();

    if (!nodeId || nodeId >= tree.Nodes.size() || tree.Nodes[*nodeId].Hash != nodeHash) {
        if (!nodeId) {
            LOG_INFO(Logger) << "Cannot parse nodeId";
        } else if (nodeId >= tree.Nodes.size()) {
            LOG_INFO(Logger) << "NodeId " << nodeId << " is out of range " << tree.Nodes.size();
        } else {
            LOG_INFO(Logger) << "Node " << nodeId << " has invalid hash: expected "
                << tree.Nodes[*nodeId].Hash << ", found " << nodeHash;
        }

        NlgData.Context["attentions"]["something_went_wrong"] = true;
        return NPOS;
    }

    return *nodeId;
}

void TAkinatorResponseBuilder::GetNextNodes(const TTree& tree, size_t nodeIndex, const TNode*& node,
                                            const TNode*& leftChild, const TNode*& rightChild)
{
    LOG_INFO(Logger) << "Current node from the request: " << nodeIndex;

    node = &tree.Nodes.at(nodeIndex);

    if (node->LeftChildIndex != NPOS) {
        leftChild = &tree.Nodes.at(node->LeftChildIndex);
    }
    if (node->RightChildIndex != NPOS) {
        rightChild = &tree.Nodes.at(node->RightChildIndex);
    }
}

void TAkinatorResponseBuilder::SampleNextNodes(const TTree& tree, const TNode*& node,
                                               const TNode*& leftChild, const TNode*& rightChild)
{
    if (tree.InitialSuggestNodeIndices.size() <= 1) {
        Y_ENSURE(!tree.Nodes.empty());
        node = &tree.Nodes.back();

        return;
    }

    size_t firstNodeIndex = NPOS;
    size_t secondNodeIndex = NPOS;
    SampleIndicesPair(tree.InitialSuggestNodeIndices.size(), Rng, firstNodeIndex, secondNodeIndex);

    const size_t leftChildIndex = tree.InitialSuggestNodeIndices.at(firstNodeIndex);
    const size_t rightChildIndex = tree.InitialSuggestNodeIndices.at(secondNodeIndex);

    LOG_INFO(Logger) << "Current node was not defined, sampled "
        << leftChildIndex << " and " << rightChildIndex << " nodes";

    leftChild = &tree.Nodes.at(leftChildIndex);
    rightChild = &tree.Nodes.at(rightChildIndex);

    Y_ENSURE(leftChild->HasPosterCloud);
    Y_ENSURE(rightChild->HasPosterCloud);

    node = leftChild->ClusterSize > rightChild->ClusterSize ? leftChild : rightChild;
}

void TAkinatorResponseBuilder::BuildBinaryChoiceResponse(const TNode& leftChild, const TNode& rightChild,
                                                         bool renderCards)
{
    LOG_INFO(Logger) << "Building binary choice response";

    if (renderCards) {
        BuildBinaryChoiceResponseCards(leftChild, rightChild);
    }

    if (Rng.RandomDouble() < 0.5) {
        AddDiscussAction(leftChild.MovieInfoIndices);
    } else {
        AddDiscussAction(rightChild.MovieInfoIndices);
    }

    BuildBinaryChoiceActions(leftChild, rightChild);
}

void TAkinatorResponseBuilder::BuildBinaryChoiceResponseCards(const TNode& leftChild, const TNode& rightChild) {
    Y_ENSURE(leftChild.LeftPosterCloudUrl);
    Y_ENSURE(rightChild.RightPosterCloudUrl);

    auto& clouds = NlgData.Context["clouds"];
    clouds["left_image_url"] = *leftChild.LeftPosterCloudUrl;
    clouds["right_image_url"] = *rightChild.RightPosterCloudUrl;

    if (!State.BaseState.WasHelpShown) {
        NlgData.Context["attentions"]["show_help"] = true;
        State.BaseState.WasHelpShown = true;
    }

    BodyBuilder.AddRenderedTextWithButtonsAndVoice(/* nlgTemplateName = */ NLG_TEMPLATE,
                                                   /* phraseName = */ RENDER_POSTER_CLOUD,
                                                   /* buttons = */ {},
                                                   NlgData);

    BodyBuilder.AddRenderedDivCard(DIV_CARDS_TEMPLATE, POSTER_CLOUD_CARD, NlgData);

    auto& analyticsInfo = *ResponseAnalyticsInfo.MutablePosterCloudState();
    analyticsInfo.SetLeftNodeId(leftChild.Index);
    analyticsInfo.SetRightNodeId(rightChild.Index);

    SerializeStateMovieIds(leftChild.MovieInfoIndices, ClusteredMovies.MovieInfos,
                           *analyticsInfo.MutableShownLeftMovieIds());
    SerializeStateMovieIds(rightChild.MovieInfoIndices, ClusteredMovies.MovieInfos,
                           *analyticsInfo.MutableShownRightMovieIds());
}

void TAkinatorResponseBuilder::BuildBinaryChoiceActions(const TNode& leftChild, const TNode& rightChild) {
    const bool isSessionStart = Frame.FindSlot(SLOT_NODE_ID) == nullptr;

    const bool showDirectionSuggest = isSessionStart || Rng.RandomDouble() < SHOW_DIRECTION_SUGGEST_PROBABILITY;

    AddChooseAction(SemanticFrame, CHOOSE_LEFT_ACTION, CHOOSE_LEFT_HINT, /* node= */ &leftChild,
                    BodyBuilder, State.SuggestsState, /* showSuggest= */ showDirectionSuggest);

    AddChooseAction(SemanticFrame, CHOOSE_RIGHT_ACTION, CHOOSE_RIGHT_HINT, /* node= */ &rightChild,
                    BodyBuilder, State.SuggestsState, /* showSuggest= */ showDirectionSuggest);

    const auto& resetHint = isSessionStart ? SHOW_OTHER_HINT : RESET_HINT;
    AddChooseAction(SemanticFrame, RESET_ACTION, resetHint, /* node= */ nullptr, BodyBuilder, State.SuggestsState,
                    /* showSuggest= */ true, /* addSlot= */ nullptr, /* frameEffectName= */ RESET_FRAME);

    AddShowGalleryAction(SemanticFrame, SHOW_GALLERY_FOR_LEFT_ACTION, SHOW_GALLERY_FOR_LEFT_HINT,
                         /* node= */ &leftChild, BodyBuilder, State.SuggestsState, Rng);
    AddShowGalleryAction(SemanticFrame, SHOW_GALLERY_FOR_RIGHT_ACTION, SHOW_GALLERY_FOR_RIGHT_HINT,
                         /* node= */ &rightChild, BodyBuilder, State.SuggestsState, Rng);
}

void TAkinatorResponseBuilder::BuildGalleryResponse(TArrayRef<const size_t> movieInfoIndices,
                                                    const TNode* node, bool renderCards)
{
    LOG_INFO(Logger) << "Building gallery response";

    if (movieInfoIndices.size() > MAX_GALLERY_SIZE) {
        movieInfoIndices = movieInfoIndices.Slice(/* offset= */ 0, /* size= */ MAX_GALLERY_SIZE);
    }

    if (renderCards) {
        BuildGalleryResponseCards(movieInfoIndices);
        AddFeedbackSuggests(/* isShowDescription= */ false);
    }

    AddDiscussAction(movieInfoIndices);

    BuildGalleryActions(movieInfoIndices, node);
}

void TAkinatorResponseBuilder::BuildGalleryResponseCards(const TArrayRef<const size_t>& movieInfoIndices) {
    auto& galleryItems = NlgData.Context["items"];

    TVector<TString> possibleSuggestTexts;
    for (const auto movieInfoIndex : movieInfoIndices) {
        const auto& movieInfo = ClusteredMovies.MovieInfos.at(movieInfoIndex);

        NJson::TJsonValue galleryItem;
        galleryItem["cover_url"] = movieInfo.CoverUrl;
        galleryItem["kinopoisk_id"] = movieInfo.KinopoiskId;

        galleryItems.AppendValue(galleryItem);
    }

    BodyBuilder.AddRenderedTextWithButtonsAndVoice(/* nlgTemplateName = */ NLG_TEMPLATE,
                                                   /* phraseName = */ RENDER_POSTER_GALLERY,
                                                   /* buttons = */ {},
                                                   NlgData);

    BodyBuilder.AddRenderedDivCard(DIV_CARDS_TEMPLATE, POSTER_GALLERY_CARD, NlgData);

    auto& analyticsInfo = *ResponseAnalyticsInfo.MutablePosterGalleryState();
    SerializeStateMovieIds(movieInfoIndices, ClusteredMovies.MovieInfos,
                           *analyticsInfo.MutableShownMovieIds());
}

void TAkinatorResponseBuilder::BuildGalleryActions(const TArrayRef<const size_t>& movieInfoIndices, const TNode* node) {
    AddChooseAction(SemanticFrame, RESET_ACTION, RESET_HINT, /* node= */ nullptr, BodyBuilder, State.SuggestsState,
                    /* showSuggest= */ true, /* addSlot= */ nullptr, /* frameEffectName= */ RESET_FRAME);

    if (Frame.FindSlot(SLOT_SHOW_DESCRIPTION)) {
        return;
    }

    for (const auto movieInfoIndex : movieInfoIndices) {
        const auto& movieInfo = ClusteredMovies.MovieInfos.at(movieInfoIndex);

        auto action = BuildShowDescriptionAction(movieInfo, node, SemanticFrame, Rng);
        BodyBuilder.AddAction(SHOW_DESCRIPTION_ACTION + "_" + ToString(movieInfo.KinopoiskId), std::move(action));
    }
}

void TAkinatorResponseBuilder::AddContentSuggest(const TFilterCondition& currentCondition) {
    if (Rng.RandomDouble() > SHOW_CONTENT_SUGGEST_PROBABILITY) {
        return;
    }

    const auto suggestIndex = Rng.RandomInteger(ClusteredMovies.ContentFilterSuggests.size());
    const auto& contentSuggest = ClusteredMovies.ContentFilterSuggests[suggestIndex];

    if (contentSuggest.Condition == currentCondition) {
        return;
    }

    const auto suggestPrefixIndex = Rng.RandomInteger(CONTENT_SUGGEST_PREFIXES.size());
    const auto suggestText = CONTENT_SUGGEST_PREFIXES[suggestPrefixIndex] + contentSuggest.ContentName;

    NAlice::TSemanticFrame semanticFrame;
    semanticFrame.SetName(TString{RECOMMEND_FRAME});

    if (contentSuggest.Condition.ExpectedContentType) {
        auto& slot = *semanticFrame.AddSlots();
        slot.SetName(TString{NVideoCommon::SLOT_CONTENT_TYPE});
        slot.SetType(TString{NVideoCommon::SLOT_CONTENT_TYPE_TYPE});
        slot.SetValue(*contentSuggest.Condition.ExpectedContentType);
    }

    if (contentSuggest.Condition.ExpectedGenre) {
        auto& slot = *semanticFrame.AddSlots();
        slot.SetName(TString{NVideoCommon::SLOT_GENRE});
        slot.SetType(TString{NVideoCommon::SLOT_GENRE_TYPE});
        slot.SetValue(*contentSuggest.Condition.ExpectedGenre);
    }

    const TNluHint nluHint{.HintText = suggestText};
    AddChooseAction(semanticFrame, SUGGEST_CONTENT_ACTION, nluHint, /* node= */ nullptr, BodyBuilder, State.SuggestsState);
}

void TAkinatorResponseBuilder::AddChooseSimilarContentHandler() {
    AddBegemotFilledFrameAction(/* frameName= */ CHOOSE_SIMILAR_CONTENT_FRAME,
                                /* actionName= */ CHOOSE_SIMILAR_CONTENT_ACTION);
    AddFailedChooseSimilarContentAction();
    AddChooseSimilarContentSuggest();
}

void TAkinatorResponseBuilder::AddFailedChooseSimilarContentAction() {
    NScenarios::TFrameAction failedChooseContentAction;

    auto& nluHint = *failedChooseContentAction.MutableNluHint();
    *nluHint.MutableFrameName() = CHOOSE_UNRECOGNIZED_CONTENT_FRAME;

    auto frame = InitCallbackFrameEffect(SemanticFrame, TString{CHOOSE_UNRECOGNIZED_CONTENT_FRAME},
                                         COPY_SLOTS_FOR_SHOW_DESCRIPTION);
    *failedChooseContentAction.MutableCallback() = ToCallback(frame);

    BodyBuilder.AddAction(FAILED_CHOOSE_SIMILAR_CONTENT_ACTION, std::move(failedChooseContentAction));
}

void TAkinatorResponseBuilder::AddChooseSimilarContentSuggest() {
    if (Rng.RandomDouble() > SHOW_CHOOSE_SIMILAR_CONTENT_SUGGEST_PROBABILITY) {
        return;
    }

    const auto topSize = Min(CHOOSE_SIMILAR_CONTENT_SUGGEST_MOVIE_TOP, ClusteredMovies.MovieInfos.size());
    if (topSize == 0) {
        return;
    }

    const auto suggestSimilarToMovieIndex = Rng.RandomInteger(topSize);
    const auto& movieInfo = ClusteredMovies.MovieInfos[suggestSimilarToMovieIndex];

    const auto suggestPrefixIndex = Rng.RandomInteger(CHOOSE_SIMILAR_SUGGEST_PREFIXES.size());
    const auto suggestText = CHOOSE_SIMILAR_SUGGEST_PREFIXES[suggestPrefixIndex] + movieInfo.Name + "»";

    NScenarios::TFrameAction action;

    auto& nluHint = *action.MutableNluHint();
    *nluHint.MutableFrameName() = CHOOSE_SIMILAR_CONTENT_BY_SUGGEST_ACTION;

    TNluPhrase& nluPhrase = *nluHint.AddInstances();
    nluPhrase.SetLanguage(ELang::L_RUS);
    nluPhrase.SetPhrase(suggestText);

    TSemanticFrame semanticFrame;
    semanticFrame.SetName(TString{CHOOSE_SIMILAR_CONTENT_BY_SUGGEST_FRAME});

    auto& filmIdSlot = *semanticFrame.AddSlots();
    filmIdSlot.SetName(TString{SLOT_FILM_ID});
    filmIdSlot.SetType("string");
    filmIdSlot.SetValue(movieInfo.OntoId);
    *action.MutableCallback() = ToCallback(semanticFrame);

    BodyBuilder.AddAction(CHOOSE_SIMILAR_CONTENT_BY_SUGGEST_ACTION, std::move(action));
    if (State.SuggestsState) {
        State.SuggestsState->Suggests.push_back({
            .SuggestId = CHOOSE_SIMILAR_CONTENT_BY_SUGGEST_ACTION,
            .SuggestTitle = suggestText,
            .HasPredefinedEffect = true
        });
    } else {
        BodyBuilder.AddActionSuggest(CHOOSE_SIMILAR_CONTENT_BY_SUGGEST_ACTION).Title(suggestText);
    }
}

void TAkinatorResponseBuilder::AddDiscussAction(const TArrayRef<const size_t>& movieInfoIndices) {
    AddBegemotFilledFrameAction(/* frameName= */ DISCUSS_FRAME, /* actionName= */ DISCUSS_ACTION);
    if (UseGcMovieDiscussWeak) {
        AddBegemotFilledFrameAction(/* frameName= */ TString{DISCUSS_FRAME} + "_weak",
                                    /* actionName= */ DISCUSS_ACTION + "_weak");
    }

    NlgData.Context["discuss_suggest_can_show_description"] = true;

    const TMovieInfo* movieInfo = nullptr;
    if (const auto showDescriptionSlot = Frame.FindSlot(SLOT_SHOW_DESCRIPTION)) {
        movieInfo = GetShowDescriptionMovieInfo(*showDescriptionSlot);
        if (!movieInfo || !ClusteredMovies.DiscussableMovieIndices.contains(movieInfo->KinopoiskId)) {
            return;
        }
    }

    if (!movieInfo) {
        NlgData.Context["discuss_suggest_can_show_description"] = false;
        AddBegemotFilledFrameAction(/* frameName= */ SHOW_DESCRIPTION_FRAME, /* actionName= */ SHOW_DESCRIPTION_ACTION);

        if (movieInfo = SampleMovieToDiscuss(movieInfoIndices); !movieInfo) {
            return;
        }
    }

    NlgData.Context["discuss_suggest_movie_name"] = movieInfo->Name;
    NlgData.Context["discuss_suggest_movie_type"] = movieInfo->ContentType;
    const auto suggestText = NlgWrapper.RenderPhrase(NLG_TEMPLATE, "render_discuss_suggest", NlgData).Text;

    if (State.SuggestsState) {
        State.SuggestsState->Suggests.push_back({
            .SuggestId = "lets_discuss_specific_movie",
            .SuggestTitle = suggestText,
            .HasPredefinedEffect = false
        });
    } else {
        AddTypeTextSuggest(suggestText, "lets_discuss_specific_movie", BodyBuilder);
    }
}

const TMovieInfo* TAkinatorResponseBuilder::SampleMovieToDiscuss(const TArrayRef<const size_t>& movieInfoIndices) {
    if (Rng.RandomDouble() > SHOW_DISCUSS_SUGGEST_PROBABILITY) {
        return nullptr;
    }

    TVector<size_t> discussableMovieInfoIndices;
    for (size_t movieInfoIndex : movieInfoIndices) {
        if (ClusteredMovies.DiscussableMovieIndices.contains(movieInfoIndex)) {
            discussableMovieInfoIndices.push_back(movieInfoIndex);
        }
    }

    if (discussableMovieInfoIndices.empty()) {
        return nullptr;
    }

    auto getPopularity = [&](size_t movieInfoIndex) {
        return ClusteredMovies.MovieInfos[movieInfoIndex].Popularity;
    };
    Sort(discussableMovieInfoIndices, [&getPopularity](size_t firstMovieInfoIndex, size_t secondMovieInfoIndex) {
        return getPopularity(firstMovieInfoIndex) > getPopularity(secondMovieInfoIndex);
    });

    const size_t itemIndex = Rng.RandomInteger(Min(DISCUSSABLE_MOVIE_TOP, discussableMovieInfoIndices.size()));
    const size_t movieInfoIndex = movieInfoIndices[itemIndex];

    return &ClusteredMovies.MovieInfos[movieInfoIndex];
}

void TAkinatorResponseBuilder::AddFeedbackSuggests(bool isShowDescription) {
    if (Frame.FindSlot(SLOT_FEEDBACK)) {
        return;
    }

    NlgData.Context["attentions"]["feedback_question_voice"] = (Rng.RandomDouble() < SHOW_FEEDBACK_VOICE_PROBABILITY);

    if (isShowDescription) {
        NlgData.Context["attentions"]["show_description_feedback_question"] = true;
    } else {
        NlgData.Context["attentions"]["show_gallery_feedback_question"] = true;
    }

    BodyBuilder.AddRenderedTextWithButtonsAndVoice(/* nlgTemplateName = */ NLG_TEMPLATE,
                                                   /* phraseName = */ "render_feedback_question",
                                                   /* buttons = */ {},
                                                   NlgData);

    for (const auto& feedbackType : {POSITIVE_FEEDBACK, NEGATIVE_FEEDBACK}) {
        AddFeedbackSuggest(TString{feedbackType}, isShowDescription);
    }
}

void TAkinatorResponseBuilder::AddFeedbackSuggest(const TString& feedbackType, bool isShowDescription) {
    const TString feedbackSuggestName = feedbackType + "_feedback_suggest";
    const TString suggestText = NlgWrapper.RenderPhrase(NLG_TEMPLATE, "render_" + feedbackSuggestName, NlgData).Text;

    NScenarios::TFrameAction action;

    action.MutableNluHint()->SetFrameName(feedbackSuggestName);

    TSemanticFrame frame;
    if (feedbackType == POSITIVE_FEEDBACK) {
        frame = SemanticFrame;
    } else if (isShowDescription) {
        frame = InitCallbackFrameEffect(SemanticFrame, TString{RECOMMEND_FRAME},
                                        COPY_SLOTS_SHOW_DESCRIPTION_RESET_ON_NEGATIVE_FEEDBACK);
    } else {
        frame = InitCallbackFrameEffect(SemanticFrame, TString{RESET_FRAME}, COPY_SLOTS);
    }

    auto& slot = *frame.AddSlots();
    slot.SetName(TString{SLOT_FEEDBACK});
    slot.SetValue(feedbackType);

    action.MutableDirectives()->AddList()->MutableTypeTextSilentDirective()->SetText(suggestText);
    *action.MutableDirectives()->AddList()->MutableCallbackDirective() = ToCallback(frame);

    BodyBuilder.AddAction(feedbackSuggestName, std::move(action));

    if (State.SuggestsState) {
        State.SuggestsState->Suggests.push_back({
            .SuggestId = feedbackSuggestName,
            .SuggestTitle = suggestText,
            .HasPredefinedEffect = true
        });
    } else {
        BodyBuilder.AddActionSuggest(feedbackSuggestName).Title(suggestText);
    }
}

void TAkinatorResponseBuilder::AddBegemotFilledFrameAction(const TStringBuf frameName, const TString& actionName) {
    NScenarios::TFrameAction action;

    auto& nluHint = *action.MutableNluHint();
    *nluHint.MutableFrameName() = frameName;

    BodyBuilder.AddAction(actionName, std::move(action));
}

void TAkinatorResponseBuilder::AddContentInfoToAnalytics(const TFilterCondition& filterCondition) {
    if (filterCondition.ExpectedContentType) {
        ResponseAnalyticsInfo.SetContentType(*filterCondition.ExpectedContentType);
    }
    if (filterCondition.ExpectedGenre) {
        ResponseAnalyticsInfo.SetGenre(*filterCondition.ExpectedGenre);
    }
}

void TAkinatorResponseBuilder::GenerateActionHelp() {
    for (const TString& helpAction : HELP_ACTIONS) {
        const auto applyProb = (helpAction == DIRECTIONS_HELP_ACTION) ? 1. : 1. / (HELP_ACTIONS.size() - 1);
        if (!State.BaseState.ShownHelpActions.contains(helpAction) && Rng.RandomDouble() < applyProb
            || Rng.RandomDouble() < REPEAT_HELP_ACTION_PROBABILITY)
        {
            NlgData.Context["attentions"][helpAction] = true;
            State.BaseState.ShownHelpActions.insert(helpAction);
            return;
        }
    }
}

} // namespace NAlice::NHollywood
