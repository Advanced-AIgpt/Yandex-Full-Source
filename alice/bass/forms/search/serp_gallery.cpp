#include "serp_gallery.h"
#include "serp.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/directives.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <library/cpp/tokenizer/split.h>

#include <util/charset/wide.h>
#include <util/generic/hash_set.h>
#include <util/string/builder.h>
#include <util/string/split.h>
#include <util/string/join.h>

namespace NBASS {
namespace NSerpGallery {

namespace {

constexpr TStringBuf FORM_NAME_GALLERY_CALL = "personal_assistant.scenarios.serp_gallery__call";
constexpr TStringBuf FORM_NAME_GALLERY_CHAT = "personal_assistant.scenarios.serp_gallery__chat";
constexpr TStringBuf FORM_NAME_GALLERY_CONFIRM = "personal_assistant.scenarios.serp_gallery__confirm";
constexpr TStringBuf FORM_NAME_GALLERY_ITEM_CONTINUATION = "personal_assistant.scenarios.serp_gallery__item_continuation";
constexpr TStringBuf FORM_NAME_GALLERY_ITEM_STOP = "personal_assistant.scenarios.serp_gallery__item_stop";
constexpr TStringBuf FORM_NAME_GALLERY_MAP = "personal_assistant.scenarios.serp_gallery__map";
constexpr TStringBuf FORM_NAME_GALLERY_OPEN = "personal_assistant.scenarios.serp_gallery__open";
constexpr TStringBuf FORM_NAME_GALLERY_SWITCH_BY_ID = "personal_assistant.scenarios.serp_gallery__switch_by_id";
constexpr TStringBuf FORM_NAME_GALLERY_SWITCH_TO_BEGIN = "personal_assistant.scenarios.serp_gallery__switch_to_begin";
constexpr TStringBuf FORM_NAME_GALLERY_SWITCH_TO_END = "personal_assistant.scenarios.serp_gallery__switch_to_end";
constexpr TStringBuf FORM_NAME_GALLERY_SWITCH_TO_NEXT = "personal_assistant.scenarios.serp_gallery__switch_to_next";
constexpr TStringBuf FORM_NAME_GALLERY_SWITCH_TO_PREV = "personal_assistant.scenarios.serp_gallery__switch_to_prev";

constexpr TStringBuf SLOT_NAME_ANSWER = "answer";
constexpr TStringBuf SLOT_NAME_ID = "id";
constexpr TStringBuf SLOT_NAME_VOICE_SUGGESTS = "voice_suggests";
constexpr TStringBuf SLOT_TYPE_ANSWER = "answer";
constexpr TStringBuf SLOT_TYPE_NUM = "num";
constexpr TStringBuf SLOT_TYPE_VOICE_SUGGESTS = "voice_suggests";

constexpr TStringBuf ATTENTION_CANT_DO_IT = "serp_gallery_cant_do_it";

void HandleSwitch(TContext& ctx, const TStringBuf intent) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::SERP);
    auto voiceAnswerBuilder = TVoiceAnswerBuilder::Create(ctx);
    if (!voiceAnswerBuilder) {
        return;
    }

    const TSlot* itemsSlot = ctx.GetSlot(SLOT_NAME_ITEMS, SLOT_TYPE_ITEMS);
    const TSlot* idSlot = ctx.GetSlot(SLOT_NAME_ID, SLOT_TYPE_NUM);
    if (intent == FORM_NAME_GALLERY_SWITCH_BY_ID) {
        if (!IsSlotEmpty(itemsSlot) && !IsSlotEmpty(idSlot) && idSlot->Value.IsIntNumber()) {
            voiceAnswerBuilder->Build(ctx, itemsSlot->Value, idSlot->Value.GetIntNumber());
        }
    } else if (intent == FORM_NAME_GALLERY_SWITCH_TO_NEXT) {
        if (!IsSlotEmpty(itemsSlot) && !IsSlotEmpty(idSlot) && idSlot->Value.IsIntNumber()) {
            voiceAnswerBuilder->Build(ctx, itemsSlot->Value, idSlot->Value.GetIntNumber() + 1);
        }
    } else if (intent == FORM_NAME_GALLERY_SWITCH_TO_PREV) {
        if (!IsSlotEmpty(itemsSlot) && !IsSlotEmpty(idSlot) && idSlot->Value.IsIntNumber()) {
            voiceAnswerBuilder->Build(ctx, itemsSlot->Value, idSlot->Value.GetIntNumber() - 1);
        }
    } else if (intent == FORM_NAME_GALLERY_SWITCH_TO_BEGIN) {
        if (!IsSlotEmpty(itemsSlot)) {
            voiceAnswerBuilder->Build(ctx, itemsSlot->Value, 1);
        }
    } else if (intent == FORM_NAME_GALLERY_SWITCH_TO_END) {
        if (!IsSlotEmpty(itemsSlot) && itemsSlot->Value.IsArray()) {
            voiceAnswerBuilder->Build(ctx, itemsSlot->Value, itemsSlot->Value.ArraySize());
        }
    }
}

void HandleSwitch(TContext& ctx) {
    HandleSwitch(ctx, ctx.FormName());
}

void HandleItemStop(TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::SERP);
    ctx.AddAttention("serp_gallery_stop");
    ctx.AddStopListeningBlock();
}

void HandleItemContinuation(TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::SERP);
    TSlot* answerSlot = ctx.GetSlot(SLOT_NAME_ANSWER, SLOT_TYPE_ANSWER);
    if (IsSlotEmpty(answerSlot)) {
        return;
    }

    answerSlot->Value["tts"] = answerSlot->Value.TrySelect("tts_continuation");
    answerSlot->Value["tts_url"].SetNull();
    answerSlot->Value["tts_continuation"].SetNull();

    TSlot* itemsSlot = ctx.GetSlot(SLOT_NAME_ITEMS, SLOT_TYPE_ITEMS);
    TSlot* idSlot = ctx.GetSlot(SLOT_NAME_ID, SLOT_TYPE_NUM);
    auto voiceAnswerBuilder = TVoiceAnswerBuilder::Create(ctx);
    if (!IsSlotEmpty(itemsSlot) && !IsSlotEmpty(idSlot) && idSlot->Value.IsIntNumber() && voiceAnswerBuilder) {
        voiceAnswerBuilder->SetVoiceSuggests(TStringBuf() /* ttsContinuation */, itemsSlot->Value, idSlot->Value.GetIntNumber(), ctx);
    }
}

void SwitchToGeneralConversation(TContext& ctx) {
    ctx.SetResponseForm("personal_assistant.general_conversation.general_conversation", false /* setCurrentFormAsCallback */);
    ctx.RunResponseFormHandler();
}

void HandleConfirmation(TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::SERP);
    TSlot* voiceSuggestsSlot = ctx.GetSlot(SLOT_NAME_VOICE_SUGGESTS, SLOT_TYPE_VOICE_SUGGESTS);
    if (IsSlotEmpty(voiceSuggestsSlot) || !voiceSuggestsSlot->Value.IsArray() || voiceSuggestsSlot->Value.ArraySize() != 1) {
        SwitchToGeneralConversation(ctx);
        return;
    }

    TStringBuf action = voiceSuggestsSlot->Value.Get(0).GetString();
    if (action == FORM_NAME_GALLERY_ITEM_CONTINUATION) {
        HandleItemContinuation(ctx);
    } else if (action == FORM_NAME_GALLERY_SWITCH_TO_NEXT) {
        HandleSwitch(ctx, FORM_NAME_GALLERY_SWITCH_TO_NEXT);
    }
}

const NSc::TValue& TrySelectItem(const TContext& ctx) {
    const TSlot* itemsSlot = ctx.GetSlot(SLOT_NAME_ITEMS, SLOT_TYPE_ITEMS);
    const TSlot* idSlot = ctx.GetSlot(SLOT_NAME_ID, SLOT_TYPE_NUM);

    if (IsSlotEmpty(itemsSlot) || !itemsSlot->Value.IsArray() || IsSlotEmpty(idSlot) || !idSlot->Value.IsIntNumber()) {
        return NSc::Null();
    }

    i64 id = idSlot->Value.GetIntNumber();
    size_t idx = id - 1;
    if (!itemsSlot->Value.IsArray() || id < 1 || itemsSlot->Value.ArraySize() <= idx) {
        return NSc::Null();
    }

    return itemsSlot->Value.Get(idx);
}

const NSc::TValue& TrySelectFromItem(const TContext& ctx, const TStringBuf field) {
    return TrySelectItem(ctx).TrySelect(field);
}

void HandleCall(TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::SERP);
    NSc::TValue callURI = TrySelectFromItem(ctx, FIELD_CALL_URI);
    if (callURI.IsNull()) {
        ctx.AddAttention(ATTENTION_CANT_DO_IT);
        return;
    }

    NSc::TValue data;
    data["uri"] = callURI;
    ctx.AddCommand<TSerpGalleryCallDirective>(TStringBuf("open_uri"), data);
}

void HandleOpen(TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::SERP);
    NSc::TValue url = TrySelectFromItem(ctx, FIELD_URL);
    Y_ASSERT(!url.IsNull());

    NSc::TValue data;
    data["uri"] = url;
    ctx.AddCommand<TSerpGalleryOpenDirective>(TStringBuf("open_uri"), data);
}

void HandleChat(TContext& ctx) {
    SwitchToGeneralConversation(ctx); // it will be removed when the functionality starts to be supported
}

void HandleMap(TContext& ctx) {
    SwitchToGeneralConversation(ctx); // it will be removed when the functionality starts to be supported
}

} // namespace

TVoiceAnswerBuilder::TVoiceAnswerBuilder(TContext& ctx)
    : MaxNumSentenses(3)
    , MaxNumChars(150)
    , EnableNavigation(false)
{
    if (ctx.HasExpFlag("enable_serp_gallery_navigation") || ctx.HasExpFlag(EXP_FLAG_ENABLE_SERP_GALLERY_DEBUG)) {
        EnableNavigation = true;
    }
}

TMaybe<TVoiceAnswerBuilder> TVoiceAnswerBuilder::Create(TContext& ctx) {
    static const THashSet<TStringBuf> galleryNavigationFormNames({
        FORM_NAME_GALLERY_SWITCH_TO_NEXT,
        FORM_NAME_GALLERY_SWITCH_TO_PREV,
        FORM_NAME_GALLERY_SWITCH_TO_BEGIN,
        FORM_NAME_GALLERY_SWITCH_TO_END,
    });

    TVoiceAnswerBuilder builder(ctx);
    if (!builder.EnableNavigation && galleryNavigationFormNames.contains(ctx.FormName())) {
        SwitchToGeneralConversation(ctx);
        return Nothing();
    }

    return std::move(builder);
}

void TVoiceAnswerBuilder::PrepareTTS(const NSc::TValue& serpItem, TString& firstPart, TString& secondPart) {
    const NSc::TValue& voiceText = serpItem.TrySelect("voice_text");
    const NSc::TValue& voiceTextContinuation = serpItem.TrySelect("voice_text_continuation");
    if (voiceText.IsString() && !voiceText.StringEmpty()) {
        firstPart = voiceText.GetString();
        secondPart = voiceTextContinuation.IsString() ? voiceTextContinuation.GetString() : TString();
        return;
    }

    const NSc::TValue& text = serpItem.TrySelect("text");
    if (!text.IsString() || text.StringEmpty()) {
        return;
    }

    TVector<TUtf16String> sentences = SplitIntoSentences(UTF8ToWide(text.GetString()));

    TStringBuilder firstPartBuilder, secondPartBuilder;

    size_t i;
    size_t numChars = 0;
    for (i = 0; i < sentences.size() && i < MaxNumSentenses && (numChars + sentences[i].size() < MaxNumChars || i == 0); ++i) {
        numChars += sentences[i].size();
        firstPartBuilder << sentences[i] << ' ';
    }

    while (i < sentences.size()) {
        secondPartBuilder << sentences[i++] << ' ';
    }

    firstPart = firstPartBuilder;
    secondPart = secondPartBuilder;
}

void TVoiceAnswerBuilder::SetVoiceSuggests(const TStringBuf ttsContinuation, const NSc::TValue& items, const i64 id, TContext& ctx) {
    NSc::TValue actions;
    actions.SetArray();
    if (!ttsContinuation.empty()) {
        actions.Push(FORM_NAME_GALLERY_ITEM_CONTINUATION);
    }
    if (EnableNavigation && items.IsArray() && id < static_cast<i64>(items.ArraySize())) {
        actions.Push(FORM_NAME_GALLERY_SWITCH_TO_NEXT);
    }
    ctx.CreateSlot(SLOT_NAME_VOICE_SUGGESTS, SLOT_TYPE_VOICE_SUGGESTS, true /* optional */, actions);

    if (items.ArrayEmpty()) {
        ctx.AddStopListeningBlock();
    }
}

void TVoiceAnswerBuilder::Build(TContext& ctx, const NSc::TValue& items, const i64 itemId, const bool splitTTS, const bool addReadableTTSUrl) {
    if (!items.IsArray() || itemId < 1 || static_cast<i64>(items.ArraySize() + 1) <= itemId) {
        ctx.AddAttention(ATTENTION_CANT_DO_IT);
        return;
    }

    size_t idx = itemId - 1;
    TString firstTTS, secondTTS;
    PrepareTTS(items.Get(idx), firstTTS, secondTTS);

    if (!splitTTS && !secondTTS.empty()) {
        firstTTS = TStringBuilder() << firstTTS << " " << secondTTS;
        secondTTS.clear();
    }

    TSlot* answerSlot = ctx.CreateSlot(SLOT_NAME_ANSWER, SLOT_TYPE_ANSWER);
    answerSlot->Value["tts"] = firstTTS;
    answerSlot->Value["tts_continuation"] = secondTTS;
    answerSlot->Value["tts_url"] = items.Get(idx).TrySelect("tts_url");

    if (addReadableTTSUrl && answerSlot->Value["tts_url"].IsString()) {
        const auto urlParts = StringSplitter(answerSlot->Value["tts_url"].GetString()).Split(' ').ToList<TStringBuf>();
        answerSlot->Value["readable_tts_url"] = JoinSeq(".", urlParts);
    }

    ctx.CreateSlot(SLOT_NAME_ID, SLOT_TYPE_NUM, true /* optional */, itemId);

    SetVoiceSuggests(secondTTS, items, itemId, ctx);
}

const TVector<TFormHandlerPair> FORM_HANDLER_PAIRS({
    {FORM_NAME_GALLERY_CALL, HandleCall},
    {FORM_NAME_GALLERY_CHAT, HandleChat},
    {FORM_NAME_GALLERY_CONFIRM, HandleConfirmation},
    {FORM_NAME_GALLERY_ITEM_CONTINUATION, HandleItemContinuation},
    {FORM_NAME_GALLERY_ITEM_STOP, HandleItemStop},
    {FORM_NAME_GALLERY_MAP, HandleMap},
    {FORM_NAME_GALLERY_OPEN, HandleOpen},
    {FORM_NAME_GALLERY_SWITCH_BY_ID, HandleSwitch},
    {FORM_NAME_GALLERY_SWITCH_TO_BEGIN, HandleSwitch},
    {FORM_NAME_GALLERY_SWITCH_TO_END, HandleSwitch},
    {FORM_NAME_GALLERY_SWITCH_TO_NEXT, HandleSwitch},
    {FORM_NAME_GALLERY_SWITCH_TO_PREV, HandleSwitch},
});

} // NSerpGallery
} // NBASS
