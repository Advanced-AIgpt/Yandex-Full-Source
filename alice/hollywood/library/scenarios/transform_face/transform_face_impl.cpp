#include "transform_face_impl.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/protos/data/language/language.pb.h>

#include <alice/library/proto/proto.h>

#include <util/random/shuffle.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf FRAME = "alice.transform_face";
constexpr TStringBuf NLG_TEMPLATE_NAME = "transform_face";
constexpr TStringBuf CALLBACK_NAME = "face_transform_request";
constexpr TStringBuf ALLOWED_HOST = "avatars.mds.yandex.net";

const TVector<TTransformFaceContinueImpl::TSuggest> SUGGESTS = {
    {"Аниме", "anime", "https://yastatic.net/s3/milab/2020/data/faces/icons/anime.png",
     {"примени аниме", "фильтр аниме", "стиль аниме", "покажи аниме"},
     {}},
    {"Орк", "orc", "https://yastatic.net/s3/milab/2020/data/faces/icons/orc.png",
     {"примени орка", "фильтр орка", "стиль орка", "покажи орка"},
     {}},
    {"Эльф", "elf", "https://yastatic.net/s3/milab/2020/data/faces/icons/elf.png",
     {"примени эльфа", "фильтр эльфа", "стиль эльфа", "покажи эльфа"},
     {}},
    {"Вампир", "vampire", "https://yastatic.net/s3/milab/2020/data/faces/icons/vampire.png",
     {"примени вампира", "фильтр вампира", "стиль вампира", "покажи вампира"},
     {}},
    {"Моложе", "rejuvenation", "https://yastatic.net/s3/milab/2020/data/faces/icons/young.png",
     {"примени ребенка", "фильтр ребенка", "стиль ребенка", "покажи ребенка"},
     {"моложе"}},
    {"Старше", "aging", "https://yastatic.net/s3/milab/2020/data/faces/icons/old.png",
     {"примени пожилого", "фильтр пожилого", "стиль пожилого", "покажи пожилого"},
     {"старше"}},
    {"Женщина", "female", "https://yastatic.net/s3/milab/2020/data/faces/icons/female.png",
     {"примени женщину", "фильтр женщины", "стиль женщины", "покажи женщину"},
     {"женственнее"}},
    {"Мужчина", "male", "https://yastatic.net/s3/milab/2020/data/faces/icons/male.png",
     {"примени мужчину", "фильтр мужчины", "стиль мужчины", "покажи мужчину"},
     {"мужественнее"}},
};

const TTransformFaceContinueImpl::TSuggest ANY_SUGGEST = {
    "", "any", "",
    {"ещё", "а ещё", "покажи ещё", "другой стиль", "другой фильтр"},
    {"другой стиль"}
};

const THashMap<TString, TString> PHRASES = {
    {"Old", "phrase_old"},
    {"Young", "phrase_young"},
    {"Male", "phrase_male"},
    {"Female", "phrase_female"},
    {"Elf", "phrase_elf"},
    {"Orc", "phrase_orc"},
    {"Vampire", "phrase_vampire"},
    {"Anime", "phrase_anime"},
};

const THashMap<TString, std::pair<TString, TString>> TRANSFORMS = {
    {"aging",        {"Old",     "0"}},
    {"rejuvenation", {"Young",   "0"}},
    {"male",         {"Male",    "0"}},
    {"female",       {"Female",  "0"}},
    {"elf",          {"Elf",     "0"}},
    {"orc",          {"Orc",     "0"}},
    {"vampire",      {"Vampire", "0"}},
    {"anime",        {"Anime",   "0"}},
};

void ToCallback(const TTransformFaceRequest& request, NScenarios::TCallbackDirective* callback) {
    callback->SetName(CALLBACK_NAME.data(), CALLBACK_NAME.size());
    auto& payloadFields = *callback->MutablePayload()->mutable_fields();

    ::google::protobuf::Value image;
    image.set_string_value(request.GetImage());
    payloadFields["image"] = image;

    ::google::protobuf::Value style;
    style.set_string_value(request.GetStyle());
    payloadFields["style"] = style;

    ::google::protobuf::Value name;
    name.set_string_value(request.GetName());
    payloadFields["name"] = name;

    ::google::protobuf::Value type;
    type.set_string_value(request.GetType());
    payloadFields["type"] = type;
}

TString ConstructShareUrl(const TString& original, const TString& transform) {
    TString url = "https://yandex.ru/images/touch/search/alicetransform?";
    TCgiParameters params = {
        std::make_pair("original_image", original),
        std::make_pair("transform_image", transform),
        std::make_pair("rpt", "imageview"),
    };
    url += params.Print();
    return url;
}

} // namespace



// RUN RENDERS

TTransformFaceRunImpl::TTransformFaceRunImpl(TScenarioHandleContext &ctx)
    : Ctx(ctx)
    , RequestProto(GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM))
    , Request(RequestProto, ctx.ServiceCtx)
{}

bool TTransformFaceRunImpl::IsFaceTransformRequest() const {
    return Request.Input().FindSemanticFrame(FRAME);
}

void TTransformFaceRunImpl::RequestPhoto() {
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(Ctx.Ctx.Nlg(), Request, Ctx.Rng, Ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    const TFrame frame(Request.Input().CreateRequestFrame(FRAME));
    auto& bodyBuilder = builder.CreateResponseBodyBuilder(&frame);

    TTransformFaceRequest state;
    {
        TString slotValue;
        if (const auto fromPtr = frame.FindSlot(TStringBuf("transform_type"))) {
            slotValue = fromPtr->Value.AsString();
        }

        auto iter = TRANSFORMS.find(slotValue);
        if (iter == TRANSFORMS.end()) {
            const auto idx = Ctx.Rng.RandomInteger(TRANSFORMS.size());
            iter = TRANSFORMS.begin();
            std::advance(iter, idx);
        }

        const auto& type = iter->first;
        const auto& [name, style] = iter->second;

        state.SetName(name);
        state.SetType(type);
        state.SetStyle(style);
    }
    bodyBuilder.SetState(state);

    TNlgData nlgData{Ctx.Ctx.Logger(), Request};
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(
        NLG_TEMPLATE_NAME,
        "render_photo_request",
        {},
        nlgData
    );

    NJson::TJsonValue directiveValue;
    directiveValue["camera_type"] = "front";
    bodyBuilder.AddClientActionDirective("start_image_recognizer", directiveValue);

    bodyBuilder.SetExpectsRequest(true);
    Ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
}

void TTransformFaceRunImpl::Continue(const TTransformFaceRequest& args) const {
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(Ctx.Ctx.Nlg(), Request, Ctx.Rng, Ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    builder.SetContinueArguments(args);
    Ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
}

void TTransformFaceRunImpl::Reject(const TStringBuf& text, const bool isIrrelevant) {
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(Ctx.Ctx.Nlg(), Request, Ctx.Rng, Ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    TNlgData nlgData{Ctx.Ctx.Logger(), Request};
    nlgData.Context["tts"] = text;
    nlgData.Context["text"] = text;
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_TEMPLATE_NAME, "standard_response", /* buttons= */ {}, nlgData);

    if (isIrrelevant) {
        builder.SetIrrelevant();
    }

    Ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
}

TTransformFaceRequest TTransformFaceRunImpl::GetTransformRequest() const {
    TTransformFaceRequest request;

    const auto* callback = Request.Input().GetCallback();
    if (callback && callback->GetName() == CALLBACK_NAME) {
        const auto& payloadFields = callback->GetPayload().fields();
        request.SetImage(payloadFields.at("image").string_value());
        request.SetStyle(payloadFields.at("style").string_value());
        request.SetType(payloadFields.at("type").string_value());
        request.SetName(payloadFields.at("name").string_value());
        return request;
    }

    const auto& input = Request.Input().Proto();
    if (input.GetEventCase() == NScenarios::TInput::kImage) {
        const auto& baseRequest = RequestProto.GetBaseRequest();
        Y_ENSURE(baseRequest.GetState().UnpackTo(&request), "failed to unpack state");
        Y_ENSURE(request.HasType() && request.HasName() && request.HasStyle(), "got corrupted state");

        request.SetImage(input.GetImage().GetUrl());

        NUri::TUri uri;
        NUri::TState::EParsed uriResult = uri.Parse(request.GetImage(), NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeFlexible);

        Y_ENSURE(uriResult == NUri::TState::EParsed::ParsedOK, "corrupted image url");
        Y_ENSURE(uri.GetHost() == ALLOWED_HOST, "image url contains prohibited host");
    }

    return request;
}

// CONTINUE RENDERS

TTransformFaceContinueImpl::TTransformFaceContinueImpl(TScenarioHandleContext &ctx)
    : Ctx(ctx)
    , RequestProto(GetOnlyProtoOrThrow<TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM))
    , Request(RequestProto, ctx.ServiceCtx)
    , Transform(Request.UnpackArguments<TTransformFaceRequest>())
{}

void TTransformFaceContinueImpl::RenderNoFaceFound() {
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(Ctx.Ctx.Nlg(), Request, Ctx.Rng, Ctx.UserLang);
    TApplyResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    TNlgData nlgData{Ctx.Ctx.Logger(), Request};

    bodyBuilder.AddRenderedTextWithButtonsAndVoice(
        NLG_TEMPLATE_NAME,
        "phrase_no_face",
        {},
        nlgData
    );

    Ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
}

void TTransformFaceContinueImpl::RenderResult(const TString& original, const TString& originalPreview,
                                              const TString& result, const TString& resultPreview) {
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(Ctx.Ctx.Nlg(), Request, Ctx.Rng, Ctx.UserLang);
    TApplyResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    int action = 0;
    {
        auto random = ANY_SUGGEST;
        TVector<const TSuggest*> suggests;
        for (const auto& item : SUGGESTS) {
            if (item.Transform != Transform.GetType()) {
                suggests.push_back(&item);
            }
        }
        random.Transform = RandomChoice(suggests)->Transform;
        AddSuggest(bodyBuilder, random, action++, true);
    }

    NJson::TJsonValue gallery;
    for (const auto& suggest : SUGGESTS) {
        NJson::TJsonMap value;
        AddSuggest(bodyBuilder, suggest, action++, suggest.Transform != Transform.GetType(), &value);
        gallery.AppendValue(std::move(value));
    }

    TNlgData nlgData{Ctx.Ctx.Logger(), Request};
    nlgData.Context["original"] = originalPreview;
    nlgData.Context["original_full"] = original;
    nlgData.Context["result"] = resultPreview;
    nlgData.Context["result_full"] = result;
    nlgData.Context["render_share"] = !Request.ClientInfo().IsYaBrowserDesktop();
    nlgData.Context["share_transform"] = ConstructShareUrl(original, result);
    nlgData.Context["suggests"] = gallery;

    bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_TEMPLATE_NAME, PHRASES.at(Transform.GetName()), {}, nlgData);
    bodyBuilder.AddRenderedDivCard(NLG_TEMPLATE_NAME, "render_result", nlgData);
    bodyBuilder.AddRenderedDivCard(NLG_TEMPLATE_NAME, "render_gallery", nlgData);

    Ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
}

void TTransformFaceContinueImpl::AddSuggest(TResponseBodyBuilder& bodyBuilder,
                                            const TSuggest& suggest, int actionIdx, bool addSuggest,
                                            NJson::TJsonMap* galleryItem) const {
    NScenarios::TFrameAction action;
    NScenarios::TCallbackDirective* callback = action.MutableCallback();
    {
        TTransformFaceRequest request;
        request.SetImage(Transform.GetImage());
        const auto& type = suggest.Transform;
        request.SetType(type);
        const auto& [name, style] = TRANSFORMS.at(type);
        request.SetName(name);
        request.SetStyle(style);
        ToCallback(request, callback);
    }

    TString actionId = "transform_request_";
    actionId += ToString(actionIdx);

    if (!suggest.NluHints.empty()) {
        TFrameNluHint nluHint;
        nluHint.SetFrameName(actionId);
        for (const auto& hint : suggest.NluHints) {
            NAlice::TNluPhrase& nluPhrase = *nluHint.AddInstances();
            nluPhrase.SetLanguage(NAlice::ELang::L_RUS);
            nluPhrase.SetPhrase(hint);
        }
        *action.MutableNluHint() = std::move(nluHint);
    }

    if (galleryItem) {
        galleryItem->InsertValue("caption", suggest.Caption);
        galleryItem->InsertValue("url", suggest.Url);
        galleryItem->InsertValue("action", actionId);
    }

    if (!suggest.Suggests.empty() && addSuggest) {
        const auto& title = RandomChoice(suggest.Suggests);
        bodyBuilder.AddActionSuggest(actionId).Title(title);
    }

    bodyBuilder.AddAction(actionId, std::move(action));
}

} // namespace NAlice::NHollywood
