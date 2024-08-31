#include "parser1x.h"

#include "abuse.h"
#include "ifs_map.h"
#include "util.h"
#include "validator.h"

#include <alice/bass/forms/context/context.h>

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/string/subst.h>

namespace NBASS {
namespace NExternalSkill {
namespace {

class TCard {
public:
    using TResponseScheme = TSkillParserVersion1x::TSkillResponseScheme;
    using TResponseConst = TResponseScheme::TResponseConst;

public:
    TCard(const TResponseConst& responseScheme, const TContext& ctx);
    virtual ~TCard() = default;

    void InsertIntoContext(TContext* ctx) const;

    TErrorBlock::TResult Process();

    static TErrorBlock::TResult Create(const TContext& ctx, const TResponseConst& response, std::unique_ptr<TCard>* card);

protected:
    enum class EImageType { Big, Small };

    virtual TStringBuf Type() const = 0;
    virtual TErrorBlock::TResult Do() = 0;

    NSc::TValue& Data();
    NSc::TValue PrepareItemJson(const TResponseScheme::TCardConst::TItemConst& item);

    // FIXME not so good to use ButtonConst, since button can be null
    void PrepareDivButton(const TResponseScheme::TDivButtonConst& button, NSc::TValue* dst, TStringBuf name, TStringBuf defaultValue);
    void PrepareFooter(const TResponseScheme::TCardConst card, NSc::TValue* dst);

protected:
    const TResponseConst& ResponseScheme;
    const TContext& Ctx;

private:
    TMaybe<NSc::TValue> Block;
};

class TBigImageCard : public TCard {
public:
    using TCard::TCard;

private:
    TErrorBlock::TResult Do() override;
    TStringBuf Type() const override { return TStringBuf("BigImage"); }
};

class TItemsListCard : public TCard {
public:
    using TCard::TCard;

private:
    TErrorBlock::TResult Do() override;
    TStringBuf Type() const override { return TStringBuf("ItemsList"); }
};

TCard::TCard(const TResponseConst& responseScheme, const TContext& ctx)
    : ResponseScheme(responseScheme)
    , Ctx(ctx)
{
}

void TCard::InsertIntoContext(TContext* ctx) const {
    Y_ASSERT(ctx);

    if (Y_UNLIKELY(!Block.Defined())) {
        return;
    }

    ctx->AddDivCardBlock(Type(), *Block);
}

NSc::TValue& TCard::Data() {
    if (!Block.Defined()) {
        Block.ConstructInPlace();
    }

    return *Block;
}

// static
void TCard::PrepareFooter(const TResponseScheme::TCardConst card, NSc::TValue* dst) {
    if (card.HasFooter()) {
        NSc::TValue& footer = (*dst)["footer"];
        footer["text"].SetString(card.Footer().Text());
        PrepareDivButton(card.Footer().Button(), &footer, TStringBuf("button"), card.Footer().Text());
    }
}

// static
void TCard::PrepareDivButton(const TResponseScheme::TDivButtonConst& button, NSc::TValue* dst, TStringBuf name, TStringBuf defaultValue) {
    if (button.IsNull()) {
        return;
    }

    NSc::TValue& buttonJson = (*dst)[name];
    if (button.HasPayload()) {
        buttonJson["payload"].SetString(button->Payload().GetRawValue()->ToJson());
    }

    if (button.HasUrl()) {
        buttonJson["url"].SetString(WrapUrlWithRedirector(Ctx, button.Url()));
    }

    if (!button.HasText()) {
        buttonJson["text"].SetString(defaultValue);
    }
    else if (button.Text()->length()) {
        buttonJson["text"].SetString(button.Text());
    }
}

NSc::TValue TCard::PrepareItemJson(const TResponseScheme::TCardConst::TItemConst& item) {
    NSc::TValue itemJson;

    if (item.HasTitle()) {
        itemJson["title"].SetString(item.Title());
    }
    if (item.HasDescription()) {
        itemJson["description"].SetString(item.Description());
    }

    if (item.HasImageId()) {
        itemJson["image_url"].SetString(
            TSkillDescription::CreateImageUrl(Ctx, item.ImageId(), item.ImageSize(), item.MdsNamespace()));
    }

    PrepareDivButton(item.Button(), &itemJson, TStringBuf("button"), item.Title());

    return itemJson;
}

TErrorBlock::TResult TCard::Process() {
    TErrorBlock::TResult err = Do();
    if (err) {
        return err;
    }

    TStringBuf cardId = ResponseScheme.Card().CardId();
    if (cardId) {
        Data()["card_id"].SetString(cardId);
    }

    return TErrorBlock::Ok;
}

// static
TErrorBlock::TResult TCard::Create(const TContext& ctx,
                                   const TSkillParserVersion1x::TSkillResponseScheme::TResponseConst& responseScheme,
                                   std::unique_ptr<TCard>* divCard) {
    Y_ASSERT(divCard);

    if (!ctx.ClientFeatures().SupportsDivCards() || !responseScheme.HasCard()) {
        return TErrorBlock::Ok;
    }

    TStringBuf type = responseScheme.Card().Type();
    if (TStringBuf("BigImage") == type) {
        *divCard = std::make_unique<TBigImageCard>(responseScheme, ctx);
    }
    else if (TStringBuf("ItemsList") == type) {
        *divCard = std::make_unique<TItemsListCard>(responseScheme, ctx);
    }

    return *divCard ? TErrorBlock::Ok : TErrorBlock("Invalid card type", ERRTYPE_CARD_TYPE, "card/type");
}

TErrorBlock::TResult TBigImageCard::Do() {
    const auto& card = ResponseScheme.Card();
    NSc::TValue& divCard = Data();
    divCard["image_url"].SetString(
        TSkillDescription::CreateImageUrl(Ctx, card.ImageId(), card.ImageSize(), card.MdsNamespace()));
    if (card.HasTitle()) {
        divCard["title"].SetString(card.Title());
    }
    if (card.HasDescription()) {
        divCard["description"].SetString(card.Description());
    }
    PrepareFooter(card, &divCard);
    PrepareDivButton(card.Button(), &divCard, TStringBuf("button"), card.Title());

    return TErrorBlock::Ok;
}

TErrorBlock::TResult TItemsListCard::Do() {
    NSc::TValue& divCard = Data();
    const auto& card = ResponseScheme.Card();

    if (card.HasHeader()) {
        divCard["header"]["text"].SetString(card.Header().Text());
    }
    PrepareFooter(card, &divCard);
    NSc::TArray& itemsJson = divCard["items"].GetArrayMutable();
    for (const auto& item : card.Items()) {
        itemsJson.push_back(PrepareItemJson(item));
    }
    return TErrorBlock::Ok;
}

} // anon namespace

constexpr TStringBuf TSkillParserVersion1x::Version = TStringBuf("1.0");


TSkillParserVersion1x::TSkillParserVersion1x(const TSession& session, const TSkillDescription& skill, NSc::TValue response)
    : Session(session)
    , Skill(skill)
    , Response(std::move(response))
{
}

// static
TErrorBlock::TResult TSkillParserVersion1x::PrepareRequest(
    const TContext& ctx,
    const TSkillDescription& skill,
    const TSession& session,
    ESkillRequestType requestType,
    NSc::TValue* body)
{
    if (requestType == ESkillRequestType::AccountLinkingComplete) {
        (*body)["account_linking_complete_event"].SetDict();
    } else if (requestType == ESkillRequestType::SkillsPurchaseComplete) {
        (*body)["purchase_complete_event"].SetDict();
    } else {
        const TSlot* slotRequest = ctx.GetSlot("request");
        if (!slotRequest) {
            return TErrorBlock(TError::EType::INVALIDPARAM, "Slot request is mandatory");
        }

        TErrorBlock::TResult err;
        if (!PrepareUtterance(ctx, *slotRequest, body, &err)) {
            if (!PrepareButton(ctx, *slotRequest, body, &err)) {
                err.ConstructInPlace(TError::EType::INVALIDPARAM, "neither utterance nor button slot found");
            }
        }

        if (err) {
            return err;
        }

        NSc::TValue& bodyRequest = (*body)["request"];

        if (skill.Scheme().UseNLU() && ctx.HasExpFlag(EXPERIMENTAL_FLAG_ENABLE_NER_FOR_SKILLS)) {
            if (const NSc::TValue* nerInfo = skill.GetNerInfo()) {
                bodyRequest["nlu"] = (*nerInfo)["nlu"];
            } else {
                NSc::TValue& nlu = bodyRequest["nlu"];
                nlu["tokens"].SetArray();
                nlu["entities"].SetArray();
            }
        }

        AddMarkup(ctx, &bodyRequest);
    }
    (*body)["version"].SetString(Version);

    NSc::TValue& bodySession = (*body)["session"];
    bodySession["new"].SetBool(session.IsNew());
    bodySession["session_id"].SetString(session.Id());
    bodySession["message_id"].SetIntNumber(session.SeqNum());
    bodySession["skill_id"].SetString(skill.Scheme().Id());

    const auto& ctxMeta = ctx.Meta();

    bodySession["user_id"].SetString(HmacSha256Hex(skill.Scheme().Salt(), ctxMeta.UUID()));
    LOG(INFO) << "Salted user_id '" << bodySession["user_id"].GetString() << "', '" << ctxMeta.UUID() << "' to skill '" << skill.Scheme().Id() << '\'' << Endl;

    NSc::TValue& bodyMeta = (*body)["meta"];
    bodyMeta["locale"].SetString(ctx.MetaLocale().ToString());
    bodyMeta["timezone"].SetString(ctxMeta.TimeZone());
    bodyMeta["client_id"].SetString(ctxMeta.ClientId());
    bodyMeta["interfaces"] = CreateHookInterfaces(ctx);

    if (ctx.MetaClientInfo().IsQuasar() && (skill.Scheme().Id() == "098f47c9-88dd-4616-81da-1362649b166d" ||
            Find(skill.Scheme().FeatureFlags(), "send_device_id") != skill.Scheme().FeatureFlags().end())) {
        bodyMeta["device_id"].SetString(ctx.Meta().DeviceState().DeviceId());
    }

    if (skill.Scheme().IsInternal()) {
        if (!ctx.ClientFeatures().SupportsDivCards()) {
            bodyMeta["flags"].Push(TStringBuf("no_cards_support"));
        }

        auto exps = ctx.Meta().Experiments().GetRawValue()->Clone();
        if (!exps.IsNull()) {
            bodyMeta["experiments"] = std::move(exps);
        }
    }

    return TErrorBlock::Ok;
}

// static
bool TSkillParserVersion1x::PrepareUtterance(const TContext& ctx, const TSlot& slot, NSc::TValue* request, TErrorBlock::TResult* /*rval*/) {
    if (TStringBuf("string") != slot.Type) {
        return false;
    }

    const NSc::TValue& slotValue = slot.Value;
    NSc::TValue& data = (*request)["request"];

    data["type"].SetString("SimpleUtterance");
    data["command"].SetString(slotValue.GetString());

    TStringBuf utterance = ctx.Meta().Utterance();
    data["original_utterance"].SetString(utterance ? utterance : slotValue.GetString());

    return true;
}

// static
bool TSkillParserVersion1x::PrepareButton(const TContext& /*ctx*/, const TSlot& slot, NSc::TValue* request, TErrorBlock::TResult* /*rval*/) {
    if (TStringBuf("button") != slot.Type) {
        return false;
    }

    const NSc::TValue& slotValue = slot.Value;
    NSc::TValue& data = (*request)["request"];

    data["type"].SetString("ButtonPressed");

    NSc::TValue payload = NSc::TValue::FromJson(slotValue["payload"].GetString());
    if (!payload.IsNull()) {
        data["payload"].Swap(payload);
    }

    return true;
}

// static
void TSkillParserVersion1x::AddMarkup(const TContext& ctx, NSc::TValue* request) {
    const TSlot* slot = ctx.GetSlot("vins_markup");
    if (IsSlotEmpty(slot)) {
        return;
    }

    NSc::TValue markup;

    static const TVector<TStringBuf> keysWhiteList({ "dangerous_context" });
    for (const TStringBuf key : keysWhiteList) {
        const NSc::TValue& value = slot->Value.Get(key);
        if (!value.IsNull()) {
            markup[key] = value;
        }
    }

    if (!markup.DictEmpty()) {
        (*request)["markup"].Swap(markup);
    }
}

TErrorBlock::TResult TSkillParserVersion1x::CreateVinsAnswer(TContext& ctx, TContext** respCtx, std::function<void(TContext&)> beforeCb) {
    *respCtx = &ctx;

    TSkillResponseScheme responseScheme(&Response);
    TSkillValidateHelper errCollector(Skill.Scheme().IsInternal());
    responseScheme.Validate("", false, std::ref(errCollector), &errCollector);
    if (errCollector.Error) {
        LOG(ERR) << "skill response validate error: " << Response << Endl;
        return errCollector.Error;
    }

    ApplyAbuse(errCollector, ctx);

    TStringBuf newFormName;
    if (responseScheme.Response().EndSession()) {
        if (Session.IsNew() && Skill.OpenInNewTab(ctx)) {
            newFormName = PROCESS_EXTERNAL_SKILL_ACTIVATE_ONLY;
        }
        else {
            newFormName = PROCESS_EXTERNAL_SKILL_DEACTIVATE;
        }
    }
    else if (Skill.OpenInNewTab(ctx) && Session.IsNew() && ctx.Meta().DialogId()->empty()) {
        newFormName = PROCESS_EXTERNAL_SKILL_ACTIVATE_ONLY;
    }

    if (newFormName) {
        TContext::TPtr newCtx = ctx.SetResponseForm(newFormName, false);
        Y_ENSURE(newCtx);
        // it is ok that we get raw pointer because the real owner is original context (ctx in this case)
        *respCtx = newCtx.Get();
        (*respCtx)->CopySlotsFrom(ctx, { TStringBuf("skill_id"), TStringBuf("skill_debug"), TStringBuf("request") });
        if (!responseScheme.Response().EndSession()) {
            (*respCtx)->CopySlotsFrom(ctx, { TStringBuf("session_id") });
        }
    }

    if (beforeCb) {
        beforeCb(**respCtx);
    }

    return DrawVinsAnswer(responseScheme, *respCtx);
}

TErrorBlock::TResult TSkillParserVersion1x::DrawVinsAnswer(const TSkillResponseScheme& wholeResponseScheme, TContext* ctx) const {
    Y_ASSERT(ctx);

    const auto& responseScheme = wholeResponseScheme.Response();

    std::unique_ptr<TCard> divCard;
    if (TErrorBlock::TResult err = TCard::Create(*ctx, responseScheme, &divCard)) {
        return err;
    }

    if (divCard) {
        if (TErrorBlock::TResult err = divCard->Process()) {
            return err;
        }

        // FIXME TODO change action buttons to suggest ones
        divCard->InsertIntoContext(ctx);
    }

    // response slot
    NSc::TValue responseJson;

    TString rs(responseScheme.Text());
    SubstGlobal(rs, "\n", "\\n");
    responseJson["text"].SetString(rs);

    if (responseScheme.HasTTS()) {
        responseJson["voice"].SetString(responseScheme.TTS());
    }

    ctx->CreateSlot("response", "response", true, std::move(responseJson));

    for (const auto& button : responseScheme.Buttons()) {
        NSc::TValue buttonJson;
        buttonJson["title"].SetString(button.Title());
        buttonJson["hide"].SetBool(button.Hide());

        if (button.HasPayload()) {
            buttonJson["payload"].SetString(button.Payload().GetRawValue()->ToJson());
        }

        if (button.HasUrl()) {
            buttonJson["url"].SetString(WrapUrlWithRedirector(*ctx, button.Url()));
        }

        ctx->AddSuggest("external_skill", std::move(buttonJson));
    }

    return TErrorBlock::Ok;
}

void TSkillParserVersion1x::ApplyAbuse(const TSkillValidateHelper& helper, TContext& ctx) {
    TAbuse abuse;
    for (const auto& ap : helper.AbusePaths) {
        TStringBuf key{ap.Path};
        if (!key.empty() && key[0] == '/') {
            key.Skip(1);
        }
        if (NSc::TValue* ptr = Response.TrySelectOrAdd(key)) {
            if (ptr->GetString()) {
                abuse.AddString(*ptr, ap.IsForTTS);
            }
        }
    }

    abuse.Substitute(ctx);
}

} // namespace NExternalSkill
} // namespace NBASS
