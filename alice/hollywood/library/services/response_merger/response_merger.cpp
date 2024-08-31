#include "response_merger.h"
#include "utils.h"

#include <alice/library/logger/logger.h>

#include <alice/protos/api/renderer/api.pb.h>
#include <alice/protos/div/div2card.pb.h>
#include <alice/protos/div/div2patch.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <google/protobuf/struct.pb.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NResponseMerger {

namespace {

const TString HANDLE_NAME = "merge_render_results";
constexpr TStringBuf RENDER_RESULT = "render_result";
const TString DEFAULT_CHROME_LAYER_RENDER_KEY = "chrome.layer.type.default";
using RenderResponse = NRenderer::TRenderResponse;
using RenderResponses = THashMap<TString, RenderResponse>;
using DivPatchResponses = THashMap<std::pair<TString, TString>, RenderResponse>;

RenderResponses CardIdToRenderResponse(TVector<RenderResponse>&& renderResponses) {
    RenderResponses renderResponse;
    for (auto& renderResponse_ : renderResponses) {
        if(!renderResponse_.HasDiv2PatchBody()) {
            renderResponse[renderResponse_.GetCardId()] = std::move(renderResponse_);
        }
    }
    return renderResponse;
}

DivPatchResponses Div2IdToDivPatchResponse(TVector<RenderResponse>&& renderResponses) {
    DivPatchResponses divPatchResponse;
    for (auto& renderResponse_ : renderResponses) {
        if(renderResponse_.HasDiv2PatchBody()) {
            divPatchResponse[std::make_pair(renderResponse_.GetCardName(), renderResponse_.GetCardId())] = std::move(renderResponse_);
        }
    }
    return divPatchResponse;
}

TMaybe<EDirectiveType> ResolveDirectiveToRenderType(const TDirective& directive) {
    if (directive.HasShowViewDirective() && !directive.GetShowViewDirective().HasDiv2Card()) {
        return EDirectiveType::ShowView;
    } else if (directive.HasAddCardDirective() && (!directive.GetAddCardDirective().HasDiv2Card() || !directive.GetAddCardDirective().HasChromeDivCard())) {
        return EDirectiveType::AddCard;
    } else if (directive.HasSetMainScreenDirective()) {
        return EDirectiveType::SetMainScreen;
    } else if (directive.HasSetUpperShutterDirective()) {
        return EDirectiveType::SetUpperShutter;
    } else if (directive.HasPatchViewDirective()) {
        return EDirectiveType::PatchView;
    } else {
        return TMaybe<EDirectiveType>();
    }
}

class TRenderDirectiveWrapper {

public:
    TRenderDirectiveWrapper(TDirective& directive,
                            EDirectiveType type,
                            const RenderResponses& renderResponse, 
                            const DivPatchResponses& divPatchResponse,
                            TRTLogger& Logger)
        : Directive(directive)
        , Type(type)
        , RenderResponse(renderResponse)
        , DivPatchResponse(divPatchResponse)
        , Logger(Logger)
    {
    }

    TMaybe<TString> TryRender() {
        switch (Type) {
            case EDirectiveType::ShowView: {
                const auto showViewCardId = Directive.GetShowViewDirective().GetCardId();
                if (const auto* renderResponse = RenderResponse.FindPtr(showViewCardId)) {
                    *Directive.MutableShowViewDirective()->MutableDiv2Card() = Div2CardFromRenderResponse(renderResponse, Logger);
                } else {
                    return TStringBuilder{} << "no " << showViewCardId << " card in RenderResponse";
                }
                break;
            }
            case EDirectiveType::AddCard: {
                if (!Directive.GetAddCardDirective().HasDiv2Card()) {
                    const auto addCardCardId = Directive.GetAddCardDirective().GetCardId();
                    const auto* renderResponse = RenderResponse.FindPtr(addCardCardId);
                    if (renderResponse) {
                        if (renderResponse->HasDiv2Body()) {
                            const auto& div2Body = renderResponse->GetDiv2Body().fields().find("card");
                            if (div2Body != renderResponse->GetDiv2Body().fields().end()) {
                                Directive.MutableAddCardDirective()->MutableDiv2Card()->MutableBody()->CopyFrom(div2Body->second.struct_value());
                            } else {
                                // Fill with empty struct for not sending null as Div2Card.Body
                                Directive.MutableAddCardDirective()->MutableDiv2Card()->MutableBody();
                            }
                            const auto& templates = renderResponse->GetDiv2Body().fields().find("templates");
                            if (templates != renderResponse->GetDiv2Body().fields().end()) {
                                Directive.MutableAddCardDirective()->MutableDiv2Templates()->CopyFrom(templates->second.struct_value());
                            } else {
                                // Fill with empty struct for not sending null as Templates
                                Directive.MutableAddCardDirective()->MutableDiv2Templates();
                            }
                        } else if (renderResponse->HasStringDiv2Body()) {
                            *Directive.MutableAddCardDirective()->MutableDiv2Card()->MutableStringBody() = std::move(renderResponse->GetStringDiv2Body());
                        } else { 
                            LOG_ERROR(Logger) << "Div2Card has neither Div2Body nor StringDiv2Body";
                        }
                    } else {
                        return TStringBuilder{} << "no " << addCardCardId << " card in RenderResponse";
                    }
                }

                const auto chromeLayerType = Directive.GetAddCardDirective().GetChromeLayerType();
                if (chromeLayerType != TAddCardDirective_EChromeLayerType::TAddCardDirective_EChromeLayerType_None 
                    && !Directive.GetAddCardDirective().HasChromeDivCard()) {
                    if (chromeLayerType == TAddCardDirective_EChromeLayerType::TAddCardDirective_EChromeLayerType_Default) {
                        if (const auto* renderResponse = RenderResponse.FindPtr(DEFAULT_CHROME_LAYER_RENDER_KEY)) {
                            if (renderResponse->HasDiv2Body()) {
                                Directive.MutableAddCardDirective()->MutableChromeDivCard()->MutableBody()->CopyFrom(renderResponse->GetDiv2Body());
                            } else if (renderResponse->HasStringDiv2Body()) {
                                auto& chromeDivCard = *Directive.MutableAddCardDirective()->MutableChromeDivCard();
                                *chromeDivCard.MutableStringBody() = std::move(renderResponse->GetStringDiv2Body());
                            } else {
                                LOG_ERROR(Logger) << "Div2Card has neither Div2Body nor StringDiv2Body";
                            }
                        } else {
                            return "Declared chrome layer  with type=Default but rendered card not found";
                        }
                    } else {
                        return "Found unsupported chrome layer type";
                    }
                }

                break;
            }
            case EDirectiveType::SetMainScreen: {
                for (auto& tab : *Directive.MutableSetMainScreenDirective()->MutableTabs()) {
                    for (auto& block : *tab.MutableBlocks()) {
                        if (block.HasHorizontalMediaGalleryBlock()) {
                            for (auto& card : *block.MutableHorizontalMediaGalleryBlock()->MutableCards()) {
                                const auto cardId = card.GetId();
                                if (const auto* renderResponse = RenderResponse.FindPtr(cardId)) {
                                    *card.MutableCard() = Div2CardFromRenderResponse(renderResponse, Logger);
                                } else {
                                    return TStringBuilder{} << "no " << cardId << " card in RenderResponse";
                                }
                            }
                        } else if (block.HasWidgetBlock()) {
                            for (auto& widget : *block.MutableWidgetBlock()->MutableWidgets()) {
                                const auto cardId = widget.GetCardId();
                                if (const auto* renderResponse = RenderResponse.FindPtr(cardId)) {
                                    *widget.MutableCard() = Div2CardFromRenderResponse(renderResponse, Logger);
                                } else {
                                    return TStringBuilder{} << "no " << cardId << " widget in RenderResponse";
                                }
                            }
                        } else if (block.HasDivBlock()) {
                            auto& divBlock = *block.MutableDivBlock();
                            const auto cardId = divBlock.GetCardId();
                            if (const auto* renderResponse = RenderResponse.FindPtr(cardId)) {
                                *divBlock.MutableCard() = Div2CardFromRenderResponse(renderResponse, Logger);
                            } else {
                                return TStringBuilder{} << "no " << cardId << " divblock in RenderResponse";
                            }   
                        }
                    }
                }
                break;
            }
            case EDirectiveType::SetUpperShutter: {
                const auto upperShutterCardId = Directive.GetSetUpperShutterDirective().GetCardId();
                if (const auto* renderResponse = RenderResponse.FindPtr(upperShutterCardId)) {
                    *Directive.MutableSetUpperShutterDirective()->MutableDiv2Card() = Div2CardFromRenderResponse(renderResponse, Logger);
                } else {
                    return TStringBuilder{} << "no " << upperShutterCardId << " card in RenderResponse";
                }
                break;
            }
            case EDirectiveType::PatchView: {
                const auto div2Id = Directive.GetPatchViewDirective().GetApplyTo();
                if (const auto* divPatchResponse = DivPatchResponse.FindPtr(std::make_pair(div2Id.GetCardName(), div2Id.GetCardId()))) {
                    NAlice::TDiv2Patch div2patch;
                    if (divPatchResponse->HasDiv2PatchBody()) {
                        auto div2patchBody = divPatchResponse->GetDiv2PatchBody();
                        if (div2patchBody.HasDiv2PatchBody()) {
                            *div2patch.MutableBody() = div2patchBody.GetDiv2PatchBody();
                        } else if (div2patchBody.HasStringDiv2PatchBody()) {
                            div2patch.SetStringBody((div2patchBody.GetStringDiv2PatchBody()));
                        } else {
                            LOG_ERROR(Logger) << "Div2PatchBody has neither Body nor StringBody";
                            break;
                        }
                        div2patch.SetTemplates(div2patchBody.GetTemplates());
                    } else { 
                        LOG_ERROR(Logger) << "DivPatchResponse has no divPatchBody";
                        break;
                    }
                    *Directive.MutablePatchViewDirective()->MutableDiv2Patch() = div2patch;
                } else {
                    return TStringBuilder{} << "No div patch with card name " << div2Id.GetCardName() << " and card id  " << div2Id.GetCardId() << " in card in RenderResponse";
                }
                break;
            }
        }

        return Nothing();
    }

private:
    TDirective& Directive;
    const EDirectiveType Type;
    const RenderResponses& RenderResponse;
    const DivPatchResponses& DivPatchResponse;
    TRTLogger& Logger;
};

template <typename TScenarioResponseProto>
class TResponseMerger {
public:
    TResponseMerger(THwServiceContext& ctx, TScenarioResponseProto& scenarioResponseProto)
        : Logger(ctx.Logger())
        , ScenarioResponseProto_(scenarioResponseProto)
        , RenderResponse_(CardIdToRenderResponse(ctx.GetProtos<RenderResponse>(RENDER_RESULT)))
        , DivPatchResponse_(Div2IdToDivPatchResponse(ctx.GetProtos<RenderResponse>(RENDER_RESULT)))
    {
    }

    TScenarioResponseProto MakeResponse() && {
        Run();
        return std::move(ScenarioResponseProto_);
    }

private:
    void Run() {
        auto& directives = *ScenarioResponseProto_.MutableResponseBody()->MutableLayout()->MutableDirectives();
        for (auto& directive : directives) {
            const auto type = ResolveDirectiveToRenderType(directive);
            if (type) {
                TRenderDirectiveWrapper renderDirectiveWrapper(directive, *type, RenderResponse_, DivPatchResponse_, Logger);
                if (const auto errorMessage = renderDirectiveWrapper.TryRender(); !errorMessage.Defined()) {
                    LOG_INFO(Logger) << "Filled directive " << *type <<  " successfully";
                } else {
                    LOG_INFO(Logger) << "Failed to fill directive " << *type << ": " << *errorMessage;
                }
            }
        }
    }

private:
    TRTLogger& Logger;
    TScenarioResponseProto ScenarioResponseProto_;
    const RenderResponses RenderResponse_;
    const DivPatchResponses DivPatchResponse_;
};

} // namespace

void TResponseMergerHandle::Do(THwServiceContext& ctx) const {
    auto runResponseProto = ctx.GetMaybeProto<TScenarioRunResponse>(RESPONSE_ITEM)
        .OrElse(ctx.GetMaybeProto<TScenarioRunResponse>(RUN_RESPONSE_ITEM));
    if (runResponseProto.Defined()) {
        ctx.AddProtobufItemToApphostContext(TResponseMerger{ctx, runResponseProto.GetRef()}.MakeResponse(),
                                            RENDERED_RESPONSE_ITEM);
        return;
    }    
    auto continueResponseProto = ctx.GetMaybeProto<TScenarioContinueResponse>(CONTINUE_RESPONSE_ITEM);
    if (continueResponseProto.Defined()) {
        ctx.AddProtobufItemToApphostContext(TResponseMerger{ctx, continueResponseProto.GetRef()}.MakeResponse(),
                                            RENDERED_RESPONSE_ITEM);
        return;
    }
    auto applyResponseProto = ctx.GetMaybeProto<TScenarioApplyResponse>(APPLY_RESPONSE_ITEM);
    if (applyResponseProto.Defined()) {
        ctx.AddProtobufItemToApphostContext(TResponseMerger{ctx, applyResponseProto.GetRef()}.MakeResponse(),
                                            RENDERED_RESPONSE_ITEM);
        return;
    }
    ythrow yexception() << "Neither of: " << RESPONSE_ITEM << " or " << CONTINUE_RESPONSE_ITEM << " or "
                        << APPLY_RESPONSE_ITEM << " found in context";
}

const TString& TResponseMergerHandle::Name() const {
    return HANDLE_NAME;
}

} // namespace NAlice::NHollywood::NResponseMerger
