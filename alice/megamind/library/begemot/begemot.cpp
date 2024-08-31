#include "begemot.h"

#include <alice/library/contacts/contacts.h>
#include <alice/library/experiments/experiments.h>
#include <alice/library/json/json.h>
#include <alice/library/metrics/names.h>
#include <alice/library/network/common.h>
#include <alice/library/video_common/device_helpers.h>
#include <alice/library/video_common/mordovia_webview_helpers.h>

#include <alice/megamind/library/classifiers/formulas/formulas_description.h>
#include <alice/megamind/library/context/wizard_response.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/search/request.h>
#include <alice/megamind/library/sources/request.h>
#include <alice/megamind/library/util/status.h>
#include <alice/megamind/library/worldwide/language/is_alice_worldwide_language.h>

#include <alice/megamind/protos/common/conditional_action.pb.h>
#include <alice/megamind/protos/common/environment_state.pb.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/protos/data/entity_meta/video_nlu_meta.pb.h>
#include <alice/protos/data/language/language.pb.h>
#include <alice/protos/data/external_entity_description.pb.h>
#include <alice/protos/data/search_result/search_result.pb.h>
#include <alice/protos/data/video/video.pb.h>
#include <alice/protos/extensions/extensions.pb.h>

#include <alice/begemot/lib/api/experiments/flags.h>
#include <alice/begemot/lib/api/params/wizextra.h>
#include <alice/begemot/lib/locale/locale.h>
#include <alice/nlu/granet/lib/user_entity/collect_from_context.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

#include <google/protobuf/util/json_util.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <search/begemot/apphost/json.h>
#include <search/begemot/apphost/protos/context.pb.h>
#include <search/begemot/rules/alice/session/proto/alice_session.pb.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/digest/city.h>
#include <util/digest/sequence.h>
#include <util/string/ascii.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/string/subst.h>

#include <regex>

namespace NAlice {

namespace NImpl {

namespace {

using namespace NBg::NSerialization;

constexpr size_t MAX_TEXT_TOKEN_COUNT = 64;
constexpr size_t MAX_SESSION_PHRASE_TOKEN_COUNT = 20;
constexpr size_t MAX_EXPECTED_TOKEN_LENGTH = 10;
constexpr int MAX_CLIENT_ENTITY_INSTANCES = 100;

void PushExternalEntitiesDesctiprion(NJson::TJsonValue& entities, const IContext& ctx,
                                     const google::protobuf::RepeatedPtrField<NData::TExternalEntityDescription>& entitiesDescription) {
    for (const auto& externalEntityDescription : entitiesDescription) {
        TClientEntity clientEntity;
        clientEntity.SetName(externalEntityDescription.GetName());
        for (const auto& item : externalEntityDescription.GetItems()) {
            auto& clientEntityItem = (*clientEntity.MutableItems())[item.GetValue().GetStringValue()];
            for (const auto& phrase : item.GetPhrases()) {
                if (clientEntityItem.GetInstances().size() >= MAX_CLIENT_ENTITY_INSTANCES) {
                    ctx.Sensors().IncRate(
                        NSignal::LabelsForDeviceStateEntitiesLimitExceeded(ctx.ClientInfo().Name,
                                                                           externalEntityDescription.GetName()));
                    break;
                }
                auto& instance = *clientEntityItem.AddInstances();
                instance.SetPhrase(phrase.GetPhrase());
                instance.SetLanguage(static_cast<ELang>(ctx.Language()));
            }
            if (item.HasVideoMeta()) {
                clientEntityItem.MutableVideo()->CopyFrom(item.GetVideoMeta());
            }
        }
        entities.AppendValue(JsonFromProto(clientEntity));
    }
}

} // namespace

bool IsEnabledAliceTagger(const IContext& ctx, const ELanguage language) {
    if (language == ELanguage::LANG_UNK || language == ELanguage::LANG_TUR) {
        return false;
    }
    return !ctx.HasExpFlag(EXP_DISABLE_TAGGER);
}

TVector<TString> EnabledBegemotRules(const IContext& ctx, const ELanguage language) {
    TVector<TString> enabledRules;
    if (IsEnabledAliceTagger(ctx, language)) {
        enabledRules.emplace_back("AliceTagger");
    }
    if (ctx.HasExpFlag(EXP_BEGEMOT_REWRITE_ELLIPSIS)) {
        enabledRules.emplace_back("AliceEllipsisRewriter");
    }
    if (ctx.HasExpFlag(EXP_ENABLE_GC_MEMORY_LSTM)) {
        enabledRules.emplace_back("AliceGcMemoryStateUpdater");
    }
    if (ctx.HasExpFlag(EXP_ENABLE_PARTIAL_PRECLASSIFIER)) {
        enabledRules.emplace_back("AlicePartialPreEmbedding");
    }
    if (ctx.HasExpFlag(EXP_USE_SEARCH_QUERY_PREPARE_RULE)) {
        enabledRules.emplace_back("AliceSearchQueryPreparer");
    }
    if (ctx.HasExpFlag(EXP_ENABLE_GC_WIZ_DETECTION)) {
        enabledRules.emplace_back("AliceWizDetection");
    }
    if (const auto enableBegemotRules = GetExperimentValueWithPrefix(ctx.ExpFlags(), EXP_ENABLE_BEGEMOT_RULES_PREFIX)) {
        for (const auto& begemotRule : StringSplitter(enableBegemotRules.GetRef()).Split(';')) {
            enabledRules.emplace_back(begemotRule);
        }
    }
    return enabledRules;
}

bool IsEnabledResolveContextualAmbiguity(const IContext& ctx, const ELanguage language) {
    if (language == ELanguage::LANG_UNK || language == ELanguage::LANG_TUR) {
        return false;
    }
    return ctx.HasExpFlag(EXP_BEGEMOT_REWRITE_ANAPHORA) || ctx.HasExpFlag(EXP_BEGEMOT_REWRITE_ELLIPSIS);
}

TString StripPhrase(const TString& phraseUtf8, size_t maxTokenCount, size_t maxSymbolCount) {
    if (maxTokenCount == 0) {
        return {};
    }
    const TUtf16String phrase = UTF8ToWide<true>(phraseUtf8);

    size_t tokenCount = 0;
    bool wasPrevSymbolPartOfToken = false;
    for (size_t symbolIndex = 0; symbolIndex < phrase.size(); ++symbolIndex) {
        if (symbolIndex == maxSymbolCount) {
            return WideToUTF8(TWtringBuf(phrase, 0, symbolIndex));
        }

        const wchar16 symbol = phrase[symbolIndex];
        const bool isPartOfToken = !IsWhitespace(symbol) && !IsPunct(symbol);
        if (!isPartOfToken && wasPrevSymbolPartOfToken) {
            tokenCount += 1;

            if (tokenCount == maxTokenCount) {
                return WideToUTF8(TWtringBuf(phrase, 0, symbolIndex));
            }
        }
        wasPrevSymbolPartOfToken = isPartOfToken;
    }

    return phraseUtf8;
}

void AddJsonDialogHistoryToWizextra(const TDialogHistory& dialogHistory, THashMap<TString, TString>& wizextra) {
    const auto& dialogTurns = dialogHistory.GetDialogTurns();
    if (dialogTurns.empty()) {
        return;
    }

    NJson::TJsonValue phrasesJson;
    phrasesJson.SetType(NJson::EJsonValueType::JSON_ARRAY);
    NJson::TJsonValue rewrittenRequestJson;
    rewrittenRequestJson.SetType(NJson::EJsonValueType::JSON_ARRAY);

    const auto maxSymbolCount = MAX_SESSION_PHRASE_TOKEN_COUNT * MAX_EXPECTED_TOKEN_LENGTH;
    const auto preprosessAndAppend = [&] (NJson::TJsonValue& phrases, TString phrase) {
        SubstGlobal(phrase, ';', ',');
        phrases.AppendValue(StripPhrase(phrase, MAX_SESSION_PHRASE_TOKEN_COUNT, maxSymbolCount));
    };
    for (const auto& dialogTurn : dialogTurns) {
        preprosessAndAppend(phrasesJson, dialogTurn.Request);
        preprosessAndAppend(phrasesJson, dialogTurn.Response);
        preprosessAndAppend(rewrittenRequestJson, dialogTurn.RewrittenRequest);
    }

    wizextra[WIZEXTRA_KEY_PREVIOUS_PHRASES] = JsonToString(phrasesJson);
    wizextra[WIZEXTRA_KEY_PREVIOUS_REWRITTEN_REQUESTS] = JsonToString(rewrittenRequestJson);
}

NBg::NProto::TAliceAvailableFrameActions GetScenarioAvailableFrameActionsProto(const IContext& ctx) {
    NBg::NProto::TAliceAvailableFrameActions availableFrameActionsProto;
    auto& actionsByNameProto = *availableFrameActionsProto.MutableActionsByName();

    if (const auto* session = ctx.Session(); session != nullptr) {
        for (const auto& [name, action] : session->GetActions()) {
            actionsByNameProto[name].MutableNluHint()->CopyFrom(action.GetNluHint());
        }
    }

    return availableFrameActionsProto;
}

NBg::NProto::TAliceAvailableActions GetDeviceStateAvailableActionsProto(const IContext& ctx) {
    NBg::NProto::TAliceAvailableActions availableActionsProto;
    auto& actionsByNameProto = *availableActionsProto.MutableActionsByName();

    const auto& deviceStateActions = ctx.SpeechKitRequest()->GetRequest().GetDeviceState().GetActions();
    for (const auto& [name, action] : deviceStateActions) {
        actionsByNameProto[name].CopyFrom(action.GetNluHint());
    }

    return availableActionsProto;
}

NBg::NProto::TAliceAvailableActiveSpaceActions GetAvailableActiveSpaceActionsProto(const IContext& ctx) {
    NBg::NProto::TAliceAvailableActiveSpaceActions availableActiveActionsProto;
    const auto& actions = ctx.SpeechKitRequest()->GetRequest().GetDeviceState().GetActiveActions();
    for (const auto& [semanticFrame, _] : actions.GetSemanticFrames()) {
        availableActiveActionsProto.AddSemanticFrames(semanticFrame);
    }
    for (const auto& [_, screenConditionalActions] : actions.GetScreenConditionalActions()) {
        for (const auto& conditionalAction : screenConditionalActions.GetConditionalActions()) {
            const auto& typedSemanticFrame = conditionalAction.GetConditionalSemanticFrame();
            if (const auto typeCase = typedSemanticFrame.GetTypeCase(); typeCase != TTypedSemanticFrame::TYPE_NOT_SET) {
                const auto& frame = typedSemanticFrame.GetReflection()->GetMessage(
                    typedSemanticFrame, typedSemanticFrame.GetDescriptor()->FindFieldByNumber(static_cast<int>(typeCase)));
                const auto& descriptor = *frame.GetDescriptor();
                availableActiveActionsProto.AddSemanticFrames(descriptor.options().GetExtension(SemanticFrameName));
            }
        }
    }
    return availableActiveActionsProto;
}

void WriteEnabledConditionalForms(const IContext& ctx, THashMap<TString, TString>* wizextra) {
    Y_ASSERT(wizextra);
    TSet<TString> forms;
    if (const auto* session = ctx.Session()) {
        for (const auto& [slug, action] : session->GetActions()) {
            const TString form = action.GetNluHint().GetFrameName();
            if (!form.empty()) {
                forms.insert(form);
            }
        }
    }
    if (!forms.empty()) {
        (*wizextra)[WIZEXTRA_KEY_ENABLED_CONDITIONAL_FORMS] = JoinSeq(",", forms);
    }
}

void FillVideoItemMeta(const TVideoItem& item, size_t itemNumber, TVideoGalleryItemMeta& meta) {
    meta.SetDuration(item.GetDuration());
    meta.SetEpisode(item.GetEpisode());
    meta.SetEpisodesCount(item.GetEpisodesCount());
    meta.SetGenre(item.GetGenre());
    meta.SetMinAge(item.GetMinAge());
    meta.SetProviderName(item.GetProviderName());
    meta.SetRating(item.GetRating());
    meta.SetReleaseYear(item.GetReleaseYear());
    meta.SetSeason(item.GetSeason());
    meta.SetSeasonsCount(item.GetSeasonsCount());
    meta.SetType(item.GetType());
    meta.SetViewCount(item.GetViewCount());
    meta.SetPosition(itemNumber);
}

void AddVideoItemToGallery(const TVideoItem& item, size_t itemNumber, TClientEntity& gallery) {
    TNluHint nluHint;
    TVideoGalleryItemMeta& meta = *nluHint.MutableVideo();
    FillVideoItemMeta(item, itemNumber, meta);

    auto instance = nluHint.AddInstances();
    instance->SetLanguage(ELang::L_UNK);
    instance->SetPhrase(item.GetName());
    (*gallery.MutableItems())[ToString(itemNumber)] = nluHint;
}

void AddItemToGallery(const TSearchResultItem& item, TClientEntity& gallery) {
    TNluHint nluHint;
    TVideoGalleryItemMeta& meta = *nluHint.MutableVideo();

    auto instance = nluHint.AddInstances();
    instance->SetLanguage(ELang::L_UNK);

    size_t itemNumber = item.GetNumber();
    if (item.HasVideoItem()) {
        instance->SetPhrase(item.GetVideoItem().GetName());
        FillVideoItemMeta(item.GetVideoItem(), itemNumber, meta);
    } else if (item.HasPersonItem()) {
        instance->SetPhrase(item.GetPersonItem().GetName());
        meta.SetPosition(itemNumber);
    } else if (item.HasCollectionItem()) {
        instance->SetPhrase(item.GetCollectionItem().GetTitle());
        meta.SetPosition(itemNumber);
    }

    (*gallery.MutableItems())[ToString(itemNumber)] = nluHint;
}

TMaybe<TClientEntity> GetContacts(const NAlice::TSpeechKitRequestProto::TContacts& contacts) {
    if (contacts.GetStatus() != "ok") {
        return Nothing();
    }
    const auto& contactsList = contacts.GetData().GetContacts();
    if (contactsList.empty()) {
        return Nothing();
    }

    TClientEntity contactsEntity;
    contactsEntity.SetName(ToString(NAlice::NContacts::CONTACTS_CLIENT_ENTITY_NAME));

    for (const auto& contact : contactsList) {
        if (NAlice::NContacts::IsContactFromMessenger(contact)) {
            continue;
        }
        TNluHint nluHint;
        auto& instance = *nluHint.AddInstances();
        instance.SetPhrase(NAlice::NContacts::GetContactMatchingInfo(contact));
        (*contactsEntity.MutableItems())[NAlice::NContacts::GetContactUniqueKey(contact)] = std::move(nluHint);
    }

    return contactsEntity;
}

TMaybe<TClientEntity> GetVideoGallery(const IContext& ctx, const TString& galleryName) {
    const NAlice::TDeviceState& deviceState = ctx.SpeechKitRequest()->GetRequest().GetDeviceState();
    const NAlice::TDeviceState::TVideo& videoState = deviceState.GetVideo();

    const auto& screenId = NAlice::NVideoCommon::CurrentScreenId(deviceState);
    if (!IsItemSelectionAvailable(screenId)) {
        return Nothing();
    }

    TClientEntity gallery;
    gallery.SetName(galleryName);

    if (screenId == NVideoCommon::EScreenId::Gallery) {
        const NAlice::TDeviceState::TVideo::TScreenState& screenState = videoState.GetScreenState();
        for (const size_t itemIndex : screenState.GetVisibleItems()) {
            if (itemIndex < 0 || itemIndex >= screenState.RawItemsSize()) {
                LOG_ERR(ctx.Logger()) << "Invalid visible item index: " << itemIndex;
                continue;
            }

            size_t itemNumber = itemIndex + 1;
            const TVideoItem& item = screenState.GetRawItems(itemIndex);

            AddVideoItemToGallery(item, itemNumber, gallery);
        }
    } else if (NAlice::NVideoCommon::IsWebViewGalleryScreen(screenId)
        || screenId == NVideoCommon::EScreenId::MordoviaMain) {
        const auto& viewStateProto = videoState.GetViewState();
        NSc::TValue viewState = NSc::TValue::FromJson(NAlice::JsonStringFromProto(viewStateProto));

        for (const auto& visibleItem : NVideoCommon::GetVisibleGalleryItems(viewState)) {
            size_t itemNumber = visibleItem.first;
            try {
                TVideoItem item = JsonToProto<TVideoItem>(visibleItem.second.ToJsonValue());
                AddVideoItemToGallery(item, itemNumber, gallery);
            } catch (yexception e) {
                LOG_ERROR(ctx.Logger()) << "Failed to parse TVideoItem from JSON: " << e.what() << Endl;
            }
        }
    } else if (screenId == NVideoCommon::EScreenId::SearchResults && videoState.HasTvInterfaceState()) {
        const NAlice::TDeviceState::TVideo::TTvInterfaceState& tvInterfaceState = videoState.GetTvInterfaceState();
        if (tvInterfaceState.HasSearchResultsScreen()) {
            const NAlice::TDeviceState::TVideo::TTvInterfaceState::TSearchResultsScreen& searchResultsScreen =
                tvInterfaceState.GetSearchResultsScreen();

            for (size_t galleryIndex = 0; galleryIndex < searchResultsScreen.GalleriesSize(); ++galleryIndex) {
                const NAlice::TSearchResultGallery& searchResultGallery = searchResultsScreen.GetGalleries(galleryIndex);
                if (searchResultGallery.GetVisible()) {
                    for (size_t itemIndex = 0; itemIndex < searchResultGallery.ItemsSize(); ++itemIndex) {
                        const NAlice::TSearchResultItem& item = searchResultGallery.GetItems(itemIndex);
                        if (item.GetVisible()) {
                            AddItemToGallery(item, gallery);
                        }
                    }
                }
            }
        }
    } else if (screenId == NVideoCommon::EScreenId::TvExpandedCollection && videoState.HasTvInterfaceState()) {
        const NAlice::TDeviceState::TVideo::TTvInterfaceState& tvInterfaceState = videoState.GetTvInterfaceState();
        if (tvInterfaceState.HasExpandedCollectionScreen()) {
            const NAlice::TDeviceState::TVideo::TTvInterfaceState::TExpandedCollectionScreen& expandedCollectionScreen =
                tvInterfaceState.GetExpandedCollectionScreen();

            for (size_t galleryIndex = 0; galleryIndex < expandedCollectionScreen.GalleriesSize(); ++galleryIndex) {
                const NAlice::TSearchResultGallery& searchResultGallery = expandedCollectionScreen.GetGalleries(galleryIndex);
                if (searchResultGallery.GetVisible()) {
                    for (size_t itemIndex = 0; itemIndex < searchResultGallery.ItemsSize(); ++itemIndex) {
                        const NAlice::TSearchResultItem& item = searchResultGallery.GetItems(itemIndex);
                        if (item.GetVisible()) {
                            AddItemToGallery(item, gallery);
                        }
                    }
                }
            }
        }
    }

    return gallery;
}

template<typename TProtoMessage>
TString Base64EncodeProto(const TProtoMessage& proto) {
    TString serialized;
    Y_PROTOBUF_SUPPRESS_NODISCARD proto.SerializeToString(&serialized);
    return Base64EncodeUrl(serialized);
}

THashMap<TString, TString> GetWizextra(const IContext& ctx, TStringBuf text, const ELanguage language, const bool addContacts) {
    THashMap<TString, TString> wizextra;

    wizextra["operation_threshold"] = "50";
    wizextra[WIZEXTRA_KEY_ALICE_PREPROCESSING] = "true";
    wizextra[WIZEXTRA_KEY_GRANET_PRINT_SAMPLE_MOCK] = "true";

    if (ctx.HasIoTUserInfo()) {
        wizextra[WIZEXTRA_KEY_IOT_USER_INFO] = Base64EncodeProto(ctx.IoTUserInfo());
    }

    // Real example of ExpFlags (from Amanda Johnson):
    //   {
    //     "bg_fresh_granet_form=f1": "",
    //     "bg_fresh_granet_form=f2": "",
    //     "mm_dont_defer_apply": "1",
    //     "uniproxy_vins_sessions": "1"
    //   }
    TVector<TString> enabledMegamindExperiments;
    for (const auto& [experiment, maybeValue] : ctx.ExpFlags()) {
        if (!ctx.HasExpFlag(experiment)) {
            continue;
        }
        if (experiment.Contains(';')) {
            LOG_WARNING(ctx.Logger()) << "Experiment \"" << experiment << "\" contains symbol ';'."
                << " It is not allowed in 'wizextra', because Begemeot splits that CGI parameter by ';'.";
            continue;
        }
        if (experiment.StartsWith(EXP_BEGEMOT_PREFIX)) {
            // If experiment has prefix 'bg_' use its name as is (for compatibility reasons).
            wizextra[experiment] = "";
        } else {
            // Otherwise write experiment name (without value) to list for WIZEXTRA_KEY_ENABLED_MEGAMIND_EXPERIMENTS
            enabledMegamindExperiments.push_back(experiment);
        }
    }
    // Has to be removed after all formulas update
    if (GetClientType(ctx.ClientFeatures()) == ECT_SMART_SPEAKER) {
        wizextra[TString::Join(EXP_BEGEMOT_GC_CLASSIFIER_CONTEXT_LENGTH, "=3")] = "";
    }
    if (!enabledMegamindExperiments.empty()) {
        wizextra[WIZEXTRA_KEY_ENABLED_MEGAMIND_EXPERIMENTS] = JoinSeq(",", enabledMegamindExperiments);
    }

    const bool resolveContextualAmbiguity = IsEnabledResolveContextualAmbiguity(ctx, language);

    if (!resolveContextualAmbiguity) {
        wizextra[WIZEXTRA_KEY_RESOLVE_CONTEXTUAL_AMBIGUITY] = "false";
        // Make the utterance safe for 'wizextra' param
        // Format of 'wizextra' value: "key1=value1;key2=value2;key3_without_value;key4_without_value"
        TString safeText(text);
        SubstGlobal(safeText, ';', ',');
        wizextra[WIZEXTRA_KEY_ALICE_ORIGINAL_TEXT] = safeText;
    } else {
        // In this case, all the rules should work with the rewritten text, so WIZEXTRA_KEY_ALICE_ORIGINAL_TEXT is not passed
        wizextra[WIZEXTRA_KEY_RESOLVE_CONTEXTUAL_AMBIGUITY] = "true";
    }

    wizextra[WIZEXTRA_KEY_USER_ENTITY_DICTS] =
        NGranet::NUserEntity::CollectDictsAsBase64(ctx.SpeechKitRequest()->GetRequest().GetDeviceState());

    if (!ctx.HasExpFlag(EXP_DISABLE_BEGEMOT_ITEM_SELECTOR)) {
        TClientEntityList galleries;
        if (const TMaybe<TClientEntity> videoGallery = GetVideoGallery(ctx, "video_gallery")) {
            galleries.AddEntities()->CopyFrom(*videoGallery);
        }
        wizextra[WIZEXTRA_KEY_GALLERIES] = Base64EncodeUrl(ToString(JsonFromProto(galleries)));
    }
    if (addContacts) {
        TClientEntityList contacts;
        if (auto contactsEntity = GetContacts(ctx.SpeechKitRequest()->GetContacts())) {
            *contacts.AddEntities() = std::move(*contactsEntity);
        }
        wizextra[WIZEXTRA_KEY_CONTACTS_PROTO] = Base64EncodeUrl(contacts.SerializeAsString());
    }

    { // filling available actions
        SetWizextraProtoMessage(wizextra, WIZEXTRA_KEY_SCENARIO_FRAME_ACTIONS,
                                GetScenarioAvailableFrameActionsProto(ctx));
    }

    { // filling available actions
        SetWizextraProtoMessage(wizextra, WIZEXTRA_KEY_DEVICE_STATE_ACTIONS, GetDeviceStateAvailableActionsProto(ctx));
    }

    { // filling available active actions
        SetWizextraProtoMessage(wizextra, WIZEXTRA_KEY_ACTIVE_SPACE_ACTIONS, GetAvailableActiveSpaceActionsProto(ctx));
    }

    WriteEnabledConditionalForms(ctx, &wizextra);

    wizextra[WIZEXTRA_KEY_IS_SMART_SPEAKER] = ctx.ClientInfo().IsSmartSpeaker() ? "true" : "false";

    const auto& deviceState = ctx.SpeechKitRequest()->GetRequest().GetDeviceState();
    if (deviceState.HasDeviceId()) {
        wizextra[WIZEXTRA_KEY_DEVICE_ID] = deviceState.GetDeviceId();
    }

    const auto* session = ctx.Session();

    if (!session) {
        return wizextra;
    }

    if (resolveContextualAmbiguity || !ctx.HasExpFlag(EXP_DISABLE_BEGEMOT_ANAPHORA_RESOLVER_IN_VINS)) {
        AddJsonDialogHistoryToWizextra(session->GetDialogHistory(), wizextra);
    }

    if (const auto responseFrame = session->GetResponseFrame()) {
        google::protobuf::string jsonString;
        const auto status = google::protobuf::util::MessageToJsonString(*responseFrame, &jsonString);
        Y_ENSURE(status.ok());
        const auto encodedString = Base64EncodeUrl(jsonString);
        wizextra[WIZEXTRA_KEY_SEMANTIC_FRAME] = encodedString;
    }

    NJson::TJsonValue entities{NJson::JSON_ARRAY};
    // Adding custom entities from device_state
    PushExternalEntitiesDesctiprion(entities, ctx, ctx.SpeechKitRequest()->GetRequest().GetDeviceState().GetExternalEntitiesDescription());
    for (const auto& environmentDeviceInfo : ctx.SpeechKitRequest()->GetRequest().GetEnvironmentState().GetDevices()) {
        if (environmentDeviceInfo.HasSpeakerDeviceState()) {
            PushExternalEntitiesDesctiprion(entities, ctx, environmentDeviceInfo.GetSpeakerDeviceState().GetExternalEntitiesDescription());
        }
    }

    if (const auto responseEntities = session->GetResponseEntities(); !responseEntities.empty()) {
        for (const auto& responseEntity : responseEntities) {
            entities.AppendValue(JsonFromProto(responseEntity));
        }
    }
    wizextra[WIZEXTRA_KEY_ENTITIES] = Base64EncodeUrl(JsonToString(entities));

    if (const auto gcMemoryState = session->GetGcMemoryState()) {
        wizextra[WIZEXTRA_KEY_GC_MEMORY_STATE] = Base64EncodeProto(gcMemoryState.GetRef());
    }

    return wizextra;
}

TVector<TString> BuildWizextraVector(const THashMap<TString, TString>& wizextra) {
    TVector<TString> params(Reserve(wizextra.size()));
    for (const auto& [key, value] : wizextra) {
        TString param = key;
        if (!value.empty()) {
            param += '=';
            param += value;
        }
        params.push_back(param);
    }
    Sort(params); // For determinancy.
    return params;
}

TString JoinWizextra(const THashMap<TString, TString>& wizextra) {
    TVector<TString> params = BuildWizextraVector(wizextra);
    return JoinSeq(TStringBuf(";"), params);
}

void SetWizextraProtoMessage(THashMap<TString, TString>& wizextra, const TString& key,
                             const google::protobuf::Message& msg) {
    wizextra[key] = Base64EncodeUrl(ToString(JsonFromProto(msg)));
}

} // namespace NImpl

using namespace NImpl;

TSourcePrepareStatus CreateNativeBegemotRequest(const TString& text, const ELanguage language, const IContext& ctx, NJson::TJsonValue& request) {
    if (text.empty()) {
        return TError() << "Could not fetch wizard: empty utterance";
    }
    NJson::TJsonValue& params = request["params"];
    const EYandexSerpTld tld = NAlice::NAliceLocale::LanguageToTld(language, YST_RU);

    const auto maxSymbolCount = MAX_TEXT_TOKEN_COUNT * MAX_EXPECTED_TOKEN_LENGTH;
    const TString strippedText = StripPhrase(text, MAX_TEXT_TOKEN_COUNT, maxSymbolCount);

    params["text"] = NJson::TJsonArray({strippedText});
    for (auto&& item : BuildWizextraVector(GetWizextra(ctx, strippedText, language, /* addContacts= */ true))) {
        params["wizextra"].AppendValue(item);
    }
    params["lr"] = NJson::TJsonArray({"213"});
    params["uil"] = NJson::TJsonArray({IsoNameByLanguage(language)});
    params["tld"] = NJson::TJsonArray({ToString(tld)});
    params["reqid"] = NJson::TJsonArray({ctx.SpeechKitRequest().RequestId()});
    if (ctx.HasExpFlag(EXP_BEGEMOT_GRANET_LOG)) {
        params["wizdbg"] = NJson::TJsonArray({"2"});
    }

    return ESourcePrepareType::Succeeded;
}

NJson::TJsonValue SetRequiredRules(NJson::TJsonValue request, const IContext& ctx, const ELanguage language) {
    for (auto&& rule : EnabledBegemotRules(ctx, language)) {
        request["params"]["rwr"].AppendValue(rule);
    }
    return request;
}

NJson::TJsonValue CreateBegemotMergerRequest(const NJson::TJsonValue& begemotRequest, const IContext& ctx) {
    NJson::TJsonValue mergerRequest(NJson::JSON_MAP);
    mergerRequest["params"]["wizextra"] = begemotRequest["params"]["wizextra"];
    if (!NMegamind::IsAliceWorldWideLanguage(ctx.Language())) {
        // on world-wide requests we run AlicePolyglotMergeResponse in separate apphost node merger_merger
        // on rus/tur requests we run AlicePolyglotMergeResponse inside simple merger to prevent useless begemot request
        mergerRequest["params"]["rwr"].AppendValue("AlicePolyglotMergeResponse");
    }
    return mergerRequest;
}

NJson::TJsonValue CreatePolyglotBegemotMergerMergerRequest(const NJson::TJsonValue& begemotRequest) {
    NJson::TJsonValue mergerMergerRequest(NJson::JSON_MAP);
    mergerMergerRequest["params"]["wizextra"] = begemotRequest["params"]["wizextra"];
    return mergerMergerRequest;
}

TSourcePrepareStatus CreateBegemotRequest(const TString& text, const ELanguage language, const IContext& ctx, NNetwork::IRequestBuilder& request) {
    if (text.empty()) {
        return TError() << "Could not fetch wizard: empty utterance";
    }
    const EYandexSerpTld tld = NAlice::NAliceLocale::LanguageToTld(language, YST_RU);

    const auto maxSymbolCount = MAX_TEXT_TOKEN_COUNT * MAX_EXPECTED_TOKEN_LENGTH;
    const TString strippedText = StripPhrase(text, MAX_TEXT_TOKEN_COUNT, maxSymbolCount);
    request.AddCgiParam(TStringBuf("text"), strippedText);
    request.AddCgiParam(TStringBuf("wizextra"), JoinWizextra(GetWizextra(ctx, strippedText, language, ctx.HasExpFlag(EXP_ENABLE_BEGEMOT_CONTACTS_LOGS))));
    //TODO: Unhardcode
    request.AddCgiParam(TStringBuf("lr"), TStringBuf("213"));
    request.AddCgiParam(TStringBuf("uil"), IsoNameByLanguage(language));
    request.AddCgiParam(TStringBuf("tld"), ToString(tld));
    request.AddCgiParam(TStringBuf("format"), TStringBuf("json"));
    request.AddCgiParam(TStringBuf("wizclient"), TStringBuf("megamind"));
    request.AddCgiParam(TStringBuf("reqid"), ctx.SpeechKitRequest().RequestId());
    request.AddCgiParam(TStringBuf("rwr"), JoinSeq(",", EnabledBegemotRules(ctx, language)));
    if (ctx.HasExpFlag(EXP_BEGEMOT_GRANET_LOG)) {
        request.AddCgiParam(TStringBuf("wizdbg"), TStringBuf("2"));
    }

    return ESourcePrepareType::Succeeded;
}

TErrorOr<TWizardResponse> ParseBegemotAliceResponse(NBg::NProto::TAlicePolyglotMergeResponseResult&& begemotResponse, const bool needGranetLog) {
    try {
        return TWizardResponse(std::move(begemotResponse), needGranetLog);
    } catch (...) { // Any parsing error.
        return TError() << "Native Begemot response parsing error: " << CurrentExceptionMessage();
    }
}

} // namespace NAlice
