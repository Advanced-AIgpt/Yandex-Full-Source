//
// HOLLYWOOD FRAMEWORK
// Render object implementation
// Internal class - not to use outside framework
//

#include "nlg_helper.h"
#include "render_impl.h"
#include "render.h"
#include "node_caller.h"

#include <alice/hollywood/library/util/service_context.h>

#include <alice/megamind/protos/scenarios/frame.pb.h>

#include <alice/library/json/json.h>

#include <util/string/printf.h>

namespace NAlice::NHollywoodFw::NPrivate {

namespace {

bool JsonToStruct(const NJson::TJsonValue& json, TMaybe<google::protobuf::Struct>& result) {
    if (!json.IsDefined()) {
        return true;
    }

    google::protobuf::Struct tmp;
    if (!JsonToProto(json, tmp).ok()) {
        return false;
    }

    result.ConstructInPlace();
    result->Swap(&tmp);

    return true;
}

} // anonimous namespace

/*
    TRender implementation ctor
*/
TRenderImpl::TRenderImpl(const TRequest& request, const TMaybe<NHollywood::TCompiledNlgComponent>& nlg)
    : Request_(request)
    , Nlg_(nlg)
    , ComplexRoot_(NJson::JSON_MAP)
    , NlgRenderHistoryRecordStorage_(request.Flags().GetValue(NHollywood::EXP_DUMP_NLG_RENDER_CONTEXT, false))
{

}

/*
*/
TRenderImpl::~TRenderImpl() {
}

/*
    Add suggestion button to output
*/
void TRenderImpl::AddSuggestion(TStringBuf nlgName, TStringBuf suggestPhraseName, TStringBuf typeText, TStringBuf name, TStringBuf imageUrl) {
   if (!Nlg_.Defined()) {
        LOG_ERROR(Request_.Debug().Logger()) << "Can not render suggest: NLG is not defined";
        return;
    }
    NNlg::TRenderPhraseResult output = RenderPhrase(nlgName, suggestPhraseName, NJson::TJsonValue{});
    if (output.Text.Empty()) {
        LOG_WARNING(Request_.Debug().Logger()) << "Can not add suggest result of phrase '" <<
                    suggestPhraseName << "' is empty";
    } else {
        Suggests_.push_back(TSuggestData{output.Text, TString(typeText), TString(name), TString(imageUrl)});
    }
}

void TRenderImpl::AppendDiv2FromNlg(TStringBuf nlgName, TStringBuf cardName, const google::protobuf::Message& proto, bool hideBorders) {
    using TStruct = google::protobuf::Struct;

    auto nlgData = ConstructNlgData(Request_, Proto2Json(proto), ComplexRoot_);
    const auto result = Nlg_->RenderCard(nlgName, cardName,Request_.Input().GetUserLanguage(),
                                                      Request_.System().Random(), nlgData);

    TStruct protoCard;
    if (!JsonToProto(result.Card["card"], protoCard).ok()) {
        LOG_ERROR(Request_.Debug().Logger()) << "Unable to convert div2 card json to Struct: " << result.Card["card"];
        return;
    }

    TMaybe<TStruct> protoTemplates;
    if (!JsonToStruct(result.Card["templates"], protoTemplates)) {
        LOG_ERROR(Request_.Debug().Logger()) << "Unable to convert div2 templates json to Struct: " << result.Card["templates"];
        return;
    }

    TMaybe<TStruct> protoPalette;
    if (!JsonToStruct(result.Card["palette"], protoPalette)) {
        LOG_ERROR(Request_.Debug().Logger()) << "Unable to convert div2 palette json to Struct: " << result.Card["palette"];
        return;
    }

    {
        auto& card = Cards_.emplace_back();
        auto& div2 = *card.MutableDiv2CardExtended();
        div2.MutableBody()->Swap(&protoCard);
        div2.SetHideBorders(hideBorders);
    }

    if (protoTemplates.Defined()) {
        if (!Div2Templates_.Defined()) {
            Div2Templates_.ConstructInPlace();
        }
        Div2Templates_->MergeFrom(*protoTemplates);
    }

    if (protoPalette.Defined()) {
        if (!Div2Palette_.Defined()) {
            Div2Palette_.ConstructInPlace();
        }
        Div2Palette_->MergeFrom(*protoPalette);
    }
}

NNlg::TRenderPhraseResult TRenderImpl::RenderPhrase(TStringBuf nlgName, TStringBuf phraseName, const NJson::TJsonValue& jsonContext) {
    if (!Nlg_.Defined()) {
        LOG_ERROR(Request_.Debug().Logger()) << "Can not render result: NLG is not defined";
        return NNlg::TRenderPhraseResult{};
    }
    NHollywood::TNlgData nlgData = ConstructNlgData(Request_, jsonContext, ComplexRoot_);

    const ELanguage language = Request_.Input().GetUserLanguage();
    IRng& rng = Request_.System().Random();
    NlgRenderHistoryRecordStorage_.TrackRenderPhrase(nlgName, phraseName, nlgData, language);
    const auto ret = Nlg_->RenderPhrase(nlgName, phraseName, language, rng, nlgData);

    if (ret.Text.Empty() && ret.Voice.Empty()) {
        LOG_WARNING(Request_.Debug().Logger()) << "You called NLG renderer but both text and voice are empty. " <<
            "NlgName: " << nlgName << "; Phrase: " << phraseName << "; Language: " << NameByLanguage(language);
    }
    return ret;
}

void TRenderImpl::SetPhraseOutput(const NNlg::TRenderPhraseResult& output) {
    if (!TextResponse.Empty()) {
        LOG_WARNING(Request_.Debug().Logger()) << "Overwrite previously stored text value. Was: '" <<
                TextResponse << "', new: '" << output.Text << '\'';
    }
    if (!VoiceResponse.Empty()) {
        LOG_WARNING(Request_.Debug().Logger()) << "Overwrite previously stored audio value. Was: '" <<
                VoiceResponse << "', new: '" << output.Voice  << '\'';
    }

    TextResponse = output.Text;
    VoiceResponse = output.Voice;
}

/*
*/
void TRenderImpl::BuildAnswer(NAlice::NScenarios::TScenarioResponseBody* response, NAppHost::IServiceContext& ctx, const TNodeCaller& caller) {
    const auto& nlgHistoryRecords = NlgRenderHistoryRecordStorage_.GetTrackedRecords();
    if (response && nlgHistoryRecords) {
        response->MutableAnalyticsInfo()->MutableNlgRenderHistoryRecords()->
            Add(nlgHistoryRecords.cbegin(), nlgHistoryRecords.cend());
    }

    // Add suggests
    if (response) {
        for (const auto& it : Suggests_) {
            NScenarios::TDirective directive;
            NScenarios::TTypeTextDirective& typeTextDirective = *directive.MutableTypeTextDirective();
            typeTextDirective.SetName(it.Name);
            typeTextDirective.SetText(it.TypeText ? it.TypeText : it.Suggest);
            auto& actions = *response->MutableFrameActions();
            TString actionId;
            sprintf(actionId, "suggest_%zu", actions.size());
            *actions[actionId].MutableDirectives()->AddList() = std::move(directive);
            actions[actionId].MutableNluHint()->SetFrameName(actionId);

            auto* suggestButton = response->MutableLayout()->AddSuggestButtons()->MutableActionButton();
            suggestButton->SetActionId(actionId);
            suggestButton->SetTitle(it.Suggest);
            if (!it.ImageUrl.Empty()) {
                suggestButton->MutableTheme()->SetImageUrl(it.ImageUrl);
            }
        }

        // Add ellipsis frames
        for (auto& frame : EllipsisFrames) {
            NScenarios::TFrameAction frameAction;

            frame.ToFrameAction(frameAction);

            TString keyMap = frame.ActionName;
            auto& responseActions = *response->MutableFrameActions();
            // Find unique name for key in map
            if (keyMap.Empty()) {
                size_t ellipsisCount = responseActions.size();
                do {
                    sprintf(keyMap, "ellipsis_%zu", ellipsisCount++);
                } while (responseActions.find(keyMap) != responseActions.end());
            }
            // Final check for user-defined keymap
            if (responseActions.find(keyMap) != responseActions.end()) {
                LOG_WARNING(Request_.Debug().Logger()) << "Duplicated key found: '" << keyMap << "'. First FrameAction will be lost.";
            }

            responseActions[keyMap].Swap(&frameAction);
        }

        if (!TextResponse.Empty()) {
            // TODO
            // AddRenderedText/*WithButtons*/(result.Text/*, buttons*/);
            auto& layout = *response->MutableLayout();
            layout.AddCards()->SetText(TextResponse);
        }
        if (!VoiceResponse.Empty()) {
            // AddRenderedVoice(result.Voice);
            auto& layout = *response->MutableLayout();
            if (layout.GetOutputSpeech()) {
                LOG_WARNING(Request_.Debug().Logger()) << "Overwrite previously stored audio value. Was: '" <<
                    layout.GetOutputSpeech() << "', new: '" << VoiceResponse << '\'';
            }
            layout.SetOutputSpeech(VoiceResponse);
        }
        if (ShouldListen) {
            auto& layout = *response->MutableLayout();
            layout.SetShouldListen(true);
        }

        if (!Cards_.empty()) {
            auto& cards = *response->MutableLayout()->MutableCards();
            for (auto& card : Cards_) {
                cards.Add()->Swap(&card);
            }
            Cards_.clear();
        }

        if (Div2Templates_.Defined()) {
            response->MutableLayout()->MutableDiv2Templates()->Swap(Div2Templates_.Get());
            Div2Templates_.Clear();
        }

        if (Div2Palette_.Defined()) {
            response->MutableLayout()->MutableDiv2Palette()->Swap(Div2Palette_.Get());
            Div2Palette_.Clear();
        }

        // Add other directives
        for (const auto& it : Directives) {
            it->Attach(*response);
        }
    }

    // TODO: OBSOLETE, will be removed soon
    for (const auto& it : DivRenderData) {
        ctx.AddProtobufItem(it, NAlice::NHollywood::RENDER_DATA_ITEM);
        if (caller.IsNeedDumpRenderData()) {
            caller.DebugDump(it, "RENDER DATA");
        }
    }

    if (response) {
        if (auto* scenarioData = ScenarioData.Get()) {
            *response->MutableScenarioData() = std::move(*scenarioData);
        }
        if (auto* stackEngine = StackEngine.Get()) {
            *response->MutableStackEngine() = std::move(*stackEngine);
        }
    }

    if (!ActionSpacesData.empty()) {
        auto* actionSpaces = response->MutableActionSpaces();
        for (auto const& [name, actionSpace] : ActionSpacesData) {
            (*actionSpaces)[TString(name)] = std::move(actionSpace);
        }
    }
}

// TRenderImpl::TEllipsis -------------------------------------------------------
void TRenderImpl::TEllipsis::ToFrameAction(NScenarios::TFrameAction& frameAction) {
    TFrameNluHint ellipsisNluHint;
    ellipsisNluHint.SetFrameName(Ellipsis);
    frameAction.MutableNluHint()->Swap(&ellipsisNluHint);

    if (Tsf.Defined()) {
        frameAction.MutableParsedUtterance()->MutableTypedSemanticFrame()->Swap(Tsf.Get());
    } else if (Callback.Defined()) {
        frameAction.MutableCallback()->Swap(Callback.Get());
    }
}

} // namespace NAlice::NHollywoodFw
