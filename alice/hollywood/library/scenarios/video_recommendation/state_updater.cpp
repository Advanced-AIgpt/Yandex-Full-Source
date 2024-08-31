#include "state_updater.h"

#include <alice/library/client/client_info.h>
#include <alice/library/video_common/defs.h>

#include <alice/protos/data/language/language.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>
#include <util/string/join.h>

namespace NAlice::NHollywood {

namespace {

const TVector<TString> DONT_CARE_PHRASES = {
    "нет",
    "не знаю",
    "не важно",
    "не хочу",
    "без разницы",
    "не имеет значения",
    "любой",
    "какой угодно",
    "какой-нибудь"
};

const TString DONT_CARE_SLOT_TYPE = "dont_care";
const TString DONT_CARE_SLOT_VALUE = "не знаю";

const TString NAVIGATION_FRAME_PREFIX = "personal_assistant.scenarios.quasar.";
const TString GO_BACKWARD = "go_backward";
const TString GO_FORWARD = "go_forward";

constexpr TStringBuf DID_NOT_UNDERSTAND_ATTENTION = "did_not_understand";
constexpr TStringBuf EMPTY_SEARCH_GALLERY_ATTENTION = "empty_search_gallery";
constexpr TStringBuf NO_TV_ATTENTION = "no_tv";
const TString ASK_ATTENTION_PREFIX = "recommendation_ask_";

TMaybe<TSemanticFrame> FindSemanticFrame(const NScenarios::TScenarioRunRequest& requestProto, const TString& formName) {
    for (const auto& semanticFrame : requestProto.GetInput().GetSemanticFrames()) {
        if (semanticFrame.GetName() == formName) {
            return semanticFrame;
        }
    }
    return Nothing();
}

bool GetIsTvPluggedIn(const NScenarios::TScenarioRunRequest& requestProto) {
    const auto& baseRequest = requestProto.GetBaseRequest();
    const TClientInfo clientInfo(baseRequest.GetClientInfo());

    return clientInfo.IsSmartSpeaker() && baseRequest.GetInterfaces().GetIsTvPlugged();
}

bool HasCallback(const NScenarios::TScenarioRunRequest& requestProto, const TStringBuf callbackName) {
    const auto& input = requestProto.GetInput();
    if (input.HasCallback()) {
        return input.GetCallback().GetName() == callbackName;
    }
    return false;
}

TNluHint GetDontCareHint() {
    TNluHint dontCareHint;

    for (const auto& phrase : DONT_CARE_PHRASES) {
        TNluPhrase& nluPhrase = *dontCareHint.AddInstances();
        nluPhrase.SetLanguage(ELang::L_RUS);
        nluPhrase.SetPhrase(phrase);
    }

    return dontCareHint;
}

bool HasSlotValue(const TSlots& slots, const TString& slotName) {
    const auto iterator = FindIf(slots, [&slotName](const auto& slot) { return slot.GetName() == slotName; });
    if (iterator == slots.end()) {
        return false;
    }
    return iterator->GetValue() || iterator->GetTypedValue().GetString();
}

bool HasEntity(const ::google::protobuf::RepeatedPtrField<TClientEntity>& entities, const TStringBuf entityName) {
    for (const auto& entity : entities) {
        if (entity.GetName() == entityName) {
            return true;
        }
    }
    return false;
}

void SetDontCareEntity(TClientEntity& entity) {
    entity.SetName(DONT_CARE_SLOT_TYPE);

    auto& items = *entity.MutableItems();
    items[DONT_CARE_SLOT_VALUE] = GetDontCareHint();
}

void ClearIsRequestedFlag(TSlots& slots) {
    for (auto& slot : slots) {
        if (slot.GetIsRequested() && slot.GetIsFilled()) {
            slot.SetIsRequested(false);
        }
    }
}

TMaybe<TString> GetEmptyRequestedSlot(const TSlots& slots) {
    for (const auto& slot : slots) {
        if (slot.GetIsRequested() && !slot.GetIsFilled()) {
            return slot.GetName();
        }
    }
    return Nothing();
}

TMaybe<TString> GetEmptyMandatorySlot(const TStateUpdater::TFormDescription& formDescription,
                                      TSemanticFrame& semanticFrame) {
    const auto& slots = semanticFrame.GetSlots();

    for (const auto& slotDescription : formDescription) {
        if (HasSlotValue(slots, slotDescription.SlotName)) {
            continue;
        }

        Y_ENSURE(!slotDescription.AcceptedTypes.empty());

        auto* requestedSlot = semanticFrame.AddSlots();
        requestedSlot->SetName(slotDescription.SlotName);
        for (const auto& acceptedType : slotDescription.AcceptedTypes) {
            requestedSlot->AddAcceptedTypes(acceptedType);
        }
        requestedSlot->AddAcceptedTypes(DONT_CARE_SLOT_TYPE);
        requestedSlot->SetIsRequested(true);

        return slotDescription.SlotName;
    }

    return Nothing();
}

TString SerializeVideoItem(const TVideoItem& item) {
    return TStringBuilder{} << '"' << item.GetName() << '"'
        << " (" << item.GetGenre() << ", " << item.GetReleaseYear() << ")";
}

void AddNavigationAction(const TString& actionName, THashMap<TString, NScenarios::TFrameAction>& frameActions) {
    auto& navigationAction = frameActions[actionName];
    navigationAction.MutableNluHint()->SetFrameName(NAVIGATION_FRAME_PREFIX + actionName);
    navigationAction.MutableCallback()->SetName(actionName);
}

} // namespace

TStateUpdater::TStateUpdater(const TString& formName, const TFormDescription& formDescription,
                             const TVideoDatabase& database,
                             const NScenarios::TScenarioRunRequest& requestProto,
                             size_t gallerySize)
    : FormDescription(formDescription)
    , VideoDatabase(database)
    , ClientInfo(requestProto.GetBaseRequest().GetClientInfo())
    , IsTvPluggedIn(GetIsTvPluggedIn(requestProto))
    , GallerySize(gallerySize)
    , NlgContext(NJson::EJsonValueType::JSON_MAP)
{
    const auto& rawState = requestProto.GetBaseRequest().GetState();
    if (!rawState.Is<TVideoRecommendationState>()) {
        const bool isFrameFoundInRequest = TryLoadFrameFromRequest(requestProto, formName);

        Y_ENSURE(isFrameFoundInRequest, "SemanticFrame wasn't found neither in request nor in state");
        return;
    }

    rawState.UnpackTo(&State);
    UpdateStateByRequest(requestProto, formName);
}

bool TStateUpdater::TryLoadFrameFromRequest(const NScenarios::TScenarioRunRequest& requestProto,
                                            const TString& formName)
{
    if (const auto maybeSemanticFrame = FindSemanticFrame(requestProto, formName)) {
        *State.AddStateHistory()->MutableSemanticFrame() = *maybeSemanticFrame;
        return true;
    }
    return false;
}

void TStateUpdater::UpdateStateByRequest(const NScenarios::TScenarioRunRequest& requestProto,
                                         const TString& formName)
{
    const bool isGoBackward = HasCallback(requestProto, GO_BACKWARD);
    const bool isGoForward = HasCallback(requestProto, GO_FORWARD);

    if (isGoForward) {
        UpdatePositionInGallery(GallerySize);
    } else if (isGoBackward) {
        if (GetCurrentState().GetGalleryFirstElementIndex() >= GallerySize) {
            UpdatePositionInGallery(-GallerySize);
        } else {
            Y_ENSURE(State.StateHistorySize() > 1, "Cannot go backward anymore");
            State.MutableStateHistory()->RemoveLast();
        }
    } else if (!TryLoadFrameFromRequest(requestProto, formName)) {
        Y_ENSURE(State.StateHistorySize() > 0, "SemanticFrame wasn't found neither in request nor in state");
        AddAttention(DID_NOT_UNDERSTAND_ATTENTION);
    }
}

void TStateUpdater::UpdatePositionInGallery(int offset) {
    auto& currentState = GetCurrentState();
    currentState.SetGalleryFirstElementIndex(currentState.GetGalleryFirstElementIndex() + offset);
}

THashMap<TString, NScenarios::TFrameAction> TStateUpdater::GetFrameActions() const {
    THashMap<TString, NScenarios::TFrameAction> frameActions;

    AddNavigationAction(GO_FORWARD, frameActions);
    if (CanGoBackward()) {
        AddNavigationAction(GO_BACKWARD, frameActions);
    }

    return frameActions;
}

bool TStateUpdater::CanGoBackward() const {
    return State.StateHistorySize() > 1
        || State.StateHistorySize() == 1 && GetCurrentState().GetGalleryFirstElementIndex() > 0;
}

void TStateUpdater::Update() {
    auto& semanticFrame = *GetCurrentState().MutableSemanticFrame();

    const auto recommendationFeatures = ConvertSlotsToFeatures(semanticFrame.GetSlots(), ClientInfo,
                                                               /* filterValue = */ DONT_CARE_SLOT_VALUE);
    const auto recommendedItems = VideoDatabase.Recommend(recommendationFeatures, GallerySize,
                                                          GetCurrentState().GetGalleryFirstElementIndex());

    if (recommendedItems.empty()) {
        AddAttention(EMPTY_SEARCH_GALLERY_ATTENTION);
    } else if (IsTvPluggedIn) {
        BuildShowGalleryDirective(recommendedItems);
    } else {
        AddAttention(NO_TV_ATTENTION);
        BuildPlainTextRecommendations(recommendedItems);
    }

    ClearIsRequestedFlag(*semanticFrame.MutableSlots());
    if (VideoDatabase.RecommendableItemCount(recommendationFeatures) > GallerySize) {
        RequestSlot();
    }

    if (!HasEntity(GetCurrentState().GetEntities(), DONT_CARE_SLOT_TYPE)) {
        SetDontCareEntity(*GetCurrentState().AddEntities());
    }
}

TVideoRecommendationStateElement& TStateUpdater::GetCurrentState() {
    Y_ENSURE(State.StateHistorySize() > 0);
    return *State.MutableStateHistory(State.StateHistorySize() - 1);
}

const TVideoRecommendationStateElement& TStateUpdater::GetCurrentState() const {
    Y_ENSURE(State.StateHistorySize() > 0);
    return State.GetStateHistory(State.StateHistorySize() - 1);
}

void TStateUpdater::BuildShowGalleryDirective(const TVector<TVideoItem>& recommendedItems) {
    Directive.ConstructInPlace();

    Directive->SetName("video_show_gallery");
    for (const auto& item : recommendedItems) {
        *Directive->AddItems() = item;
    }
}

void TStateUpdater::BuildPlainTextRecommendations(const TVector<TVideoItem>& recommendedItems) {
    TVector<TString> elements;
    for (const auto& item : recommendedItems) {
        elements.push_back(SerializeVideoItem(item));
    }

    NlgContext["plain_text_recommendations"] = JoinSeq(", ", elements);
}

void TStateUpdater::RequestSlot() {
    auto& semanticFrame = *GetCurrentState().MutableSemanticFrame();

    if (const auto maybeRequestedSlotName = GetEmptyRequestedSlot(semanticFrame.GetSlots())) {
        AddAttention(ASK_ATTENTION_PREFIX + *maybeRequestedSlotName);
        return;
    }

    if (const auto maybeRequestedSlotName = GetEmptyMandatorySlot(FormDescription, semanticFrame)) {
        AddAttention(ASK_ATTENTION_PREFIX + *maybeRequestedSlotName);
    }
}

void TStateUpdater::AddAttention(const TStringBuf attention) {
    NlgContext["attentions"][attention] = true;
}

} // namespace NAlice::NHollywood
