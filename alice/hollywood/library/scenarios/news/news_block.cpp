#include "news_block.h"

#include "alice/library/logger/logger.h"
#include "bass.h"
#include "frame.h"

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/fast_command/common.h>

#include <alice/library/json/json.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

//---------- TNewsBlock ----------

TStringBuf GetMode(const TCallbackDirective& directive) {
    return directive.GetPayload().fields().at(TString(CALLBACK_MODE)).string_value();
}

TStringBuf GetType(const TCallbackDirective& directive) {
    auto& fields = directive.GetPayload().fields();
    if (fields.count("type")) {
        return directive.GetPayload().fields().at("type").string_value();
    }
    return TStringBuf();
}

bool IsNewsBlockScenario(const NHollywoodFw::TRunRequest& runRequest, const NHollywood::TPtrWrapper<NScenarios::TCallbackDirective>& callback) {
    return (!callback || GetType(*callback) == NEWS_BLOCK_CALLBACK_TYPE)
        && runRequest.Client().GetInterfaces().GetHasDirectiveSequencer() && !runRequest.Flags().IsExperimentEnabled("news_disable_block_mode");
}

} // anonimous namespace

TNewsBlock::TNewsBlock(
    const NHollywoodFw::TRunRequest& runRequest,
    const NJson::TJsonValue& bassResponse,
    bool isRenderStep)
{
    const NHollywood::TPtrWrapper<NScenarios::TCallbackDirective> callback = runRequest.Input().FindCallback();

    if (!IsNewsBlockScenario(runRequest, callback)) {
        Mode = CALLBACK_MODE_NOT_NEWS_BLOCK;
        return;
    }

    TFrameNews newFrame(runRequest, GET_NEWS_FRAME);
    NewsArrayPosition = newFrame.ArrayPosition.Value.GetOrElse(0);
    if (newFrame.SkipIntroAndEnding.Defined()) {
        HasIntroAndEnding = !*newFrame.SkipIntroAndEnding;
    } else {
        HasIntroAndEnding = !runRequest.Flags().IsExperimentEnabled("alice_news_skip_intro_and_ending");
    }

    const auto& newsSlot = GetNewsSlot(bassResponse);
    NewsCount = GetNewsCount(newsSlot);

    if (!callback) {
        if (!isRenderStep && !newFrame.ArrayPosition.Value) {
            Mode = CALLBACK_MODE_NEWS_LIST;
        }
        else if (!newFrame.ArrayPosition.Value && HasIntroAndEnding) {
            Mode = CALLBACK_MODE_INTRO;
        }
        else {
            Mode = (NewsArrayPosition < NewsCount ? CALLBACK_MODE_NEWS_ITEM : CALLBACK_MODE_NEWS_LIST_CONTINUE);
        }
    }
    else {
        Mode = GetMode(*callback);
        if (Mode == CALLBACK_MODE_NEWS_LIST_CONTINUE && isRenderStep) {
            Mode = (NewsArrayPosition < NewsCount ? CALLBACK_MODE_NEWS_ITEM : CALLBACK_MODE_ENDING);
        }
    }

    if (Mode == CALLBACK_MODE_NEWS_ITEM && NewsArrayPosition < NewsCount) {
        NewsId = newsSlot.GetArray()[NewsArrayPosition]["id"].GetString();
    }
}

bool TNewsBlock::IsNewsBlock() const {
    return Mode != CALLBACK_MODE_NOT_NEWS_BLOCK;
}

bool TNewsBlock::NeedNewRequest() const {
    return Mode == CALLBACK_MODE_NEWS_LIST || Mode == CALLBACK_MODE_NEWS_LIST_CONTINUE || Mode == CALLBACK_MODE_MORE_INFO;
}

const TMaybe<TStringBuf>& TNewsBlock::GetNewsId() const {
    return NewsId;
}

TStringBuf TNewsBlock::GetCallbackMode() const {
    return Mode;
}

const NJson::TJsonValue& TNewsBlock::GetCurrentNewsItem(const NJson::TJsonValue& bassResponse) const {
    const auto& newsSlot = GetNewsSlot(bassResponse);
    return newsSlot.GetArray()[NewsArrayPosition];
}

TStringBuf TNewsBlock::GetNewsUrl(const NJson::TJsonValue& bassResponse) const {
    return GetCurrentNewsItem(bassResponse)["url"].GetString();
}

TStringBuf TNewsBlock::GetNewsText(const NJson::TJsonValue& bassResponse) const {
    return GetCurrentNewsItem(bassResponse)["text"].GetString();
}

TMaybe<TNextNewsBlockLink> TNewsBlock::GetNextBlock() const {
    if (Mode == CALLBACK_MODE_INTRO) {
        return TNextNewsBlockLink(CALLBACK_MODE_NEWS_ITEM, 0);
    }
    else if (Mode == CALLBACK_MODE_NEWS_ITEM && NewsArrayPosition < NewsCount - 1) {
        return TNextNewsBlockLink(CALLBACK_MODE_NEWS_ITEM, NewsArrayPosition + 1);
    }
    else if (Mode == CALLBACK_MODE_NEWS_ITEM && HasIntroAndEnding) {
        return TNextNewsBlockLink(CALLBACK_MODE_ENDING, NewsArrayPosition);
    }
    else if (Mode == CALLBACK_MODE_ENDING && HasIntroAndEnding) {
        return TNextNewsBlockLink(CALLBACK_MODE_ENDING_SHOULD_LISTEN, NewsArrayPosition);
    }
    return Nothing();
}

void TNewsBlock::AddMoreNewsButton(TVector<TNewsBlockVoiceButton>& buttons) const {
    TStringBuf mode{};
    int newsIdx = NewsArrayPosition + 1;

    if (Mode == CALLBACK_MODE_INTRO) {
        mode = CALLBACK_MODE_NEWS_ITEM;
        newsIdx = 0;
    }
    else if (Mode == CALLBACK_MODE_NEWS_ITEM) {
        mode = (newsIdx < NewsCount ? CALLBACK_MODE_NEWS_ITEM : CALLBACK_MODE_NEWS_LIST_CONTINUE);
    }
    else if (Mode == CALLBACK_MODE_ENDING || Mode == CALLBACK_MODE_ENDING_SHOULD_LISTEN) {
        mode = CALLBACK_MODE_NEWS_LIST_CONTINUE;
    }

    if (mode) {
        buttons.push_back(TNewsBlockVoiceButton(GET_MORE_NEWS_FRAME, GET_MORE_NEWS_BUTTON, mode, newsIdx));
    }
}

TNewsBlockVoiceButton TNewsBlock::GetMoreNewsAfterHandoffButton() const {
    TStringBuf mode{};
    int newsIdx = 0;

    if (Mode == CALLBACK_MODE_MORE_INFO && NewsArrayPosition < NewsCount - 1) {
        mode = CALLBACK_MODE_NEWS_ITEM;
        newsIdx = NewsArrayPosition + 1;
    } else if (Mode == CALLBACK_MODE_MORE_INFO) {
        mode = CALLBACK_MODE_NEWS_LIST_CONTINUE;
        newsIdx = 0;
    }

    return TNewsBlockVoiceButton(GET_MORE_NEWS_CONFIRM_FRAME, GET_MORE_NEWS_CONFIRM_BUTTON, mode, newsIdx);
}

void TNewsBlock::AddPreviousNewsButton(TVector<TNewsBlockVoiceButton>& buttons) const {
    TStringBuf mode{};
    int newsIdx = 0;

    if (Mode == CALLBACK_MODE_NEWS_ITEM && NewsArrayPosition > 0) {
        mode = CALLBACK_MODE_NEWS_ITEM;
        newsIdx = NewsArrayPosition - 1;
    }
    else if (Mode == CALLBACK_MODE_ENDING || Mode == CALLBACK_MODE_ENDING_SHOULD_LISTEN) {
        mode = CALLBACK_MODE_NEWS_ITEM;
        newsIdx = NewsCount - 1;
    }

    if (mode) {
        buttons.push_back(TNewsBlockVoiceButton(GET_PREVIOUS_NEWS_FRAME, GET_PREVIOUS_NEWS_BUTTON, mode, newsIdx));
    }
}

void TNewsBlock::AddMoreInfoButton(TVector<TNewsBlockVoiceButton>& buttons) const {
    TStringBuf mode{};
    int newsIdx = 0;

    if (Mode == CALLBACK_MODE_NEWS_ITEM) {
        mode = CALLBACK_MODE_MORE_INFO;
        newsIdx = NewsArrayPosition;
    }

    if (mode) {
        buttons.push_back(TNewsBlockVoiceButton(GET_MORE_INFO_FRAME, GET_MORE_INFO_BUTTON, mode, newsIdx));
    }
}

void TNewsBlock::SetVoiceButtons(TVector<TNewsBlockVoiceButton>& buttons) const {
    AddMoreNewsButton(buttons);
    AddPreviousNewsButton(buttons);
    AddMoreInfoButton(buttons);
}

void TNewsBlock::UpdateFixList(NJson::TJsonValue& fixList) const {
    fixList["block_mode"] = Mode;
    fixList["block_news_item_idx"] = NewsArrayPosition;
}

bool TNewsBlock::IsFirstBlock() const {
    return NewsArrayPosition == 0 && !HasIntroAndEnding || Mode == CALLBACK_MODE_INTRO;
}

bool TNewsBlock::IsShouldListenBlock() const {
    return Mode == CALLBACK_MODE_ENDING_SHOULD_LISTEN;
}

bool TNewsBlock::IsEnding() const {
    return Mode == CALLBACK_MODE_ENDING || Mode == CALLBACK_MODE_ENDING_SHOULD_LISTEN;
}

namespace {

::google::protobuf::Value MakeStringValue(TStringBuf s) {
    ::google::protobuf::Value value;
    value.set_string_value(TString{s});
    return value;
}

} // namespace

//---------- TNextNewsBlockLink ----------

TNextNewsBlockLink::TNextNewsBlockLink(TStringBuf mode, int newsIdx) :
    Mode(mode),
    NewsArrayPosition(newsIdx)
{
}

void TNextNewsBlockLink::UpdateCallback(NScenarios::TCallbackDirective& callback) const {
    auto& payloadFields = *callback.MutablePayload()->mutable_fields();
    payloadFields[TString(CALLBACK_MODE)] = MakeStringValue(Mode);
    payloadFields["type"] = MakeStringValue(NEWS_BLOCK_CALLBACK_TYPE);
}

void TNextNewsBlockLink::UpdateFrame(TSemanticFrame& frame) const {
    EraseIf(*frame.MutableSlots(), [](const auto& slot) { return slot.GetName() == NEWS_IDX_SLOT; });
    auto& slot = *frame.AddSlots();
    slot.SetName(TString{NEWS_IDX_SLOT});
    slot.SetType("num");
    slot.SetValue(ToString(NewsArrayPosition));
}

//---------- TNewsBlockVoiceButton ----------

TNewsBlockVoiceButton::TNewsBlockVoiceButton(TStringBuf frameName, TStringBuf buttonName, TStringBuf mode, int newsIdx) :
    TNextNewsBlockLink(mode, newsIdx),
    FrameName(frameName),
    ButtonName(buttonName)
{
}

TStringBuf TNewsBlockVoiceButton::GetFrameName() const {
    return FrameName;
}

TStringBuf TNewsBlockVoiceButton::GetButtonName() const {
    return ButtonName;
}

} // namespace NAlice::NHollywood
