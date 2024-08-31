#pragma once

#include "alice/hollywood/library/framework/core/request.h"
#include <alice/hollywood/library/frame/callback.h>
#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/library/json/json.h>

#include <util/generic/maybe.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywood {

constexpr TStringBuf NEWS_BLOCK_CALLBACK_TYPE = "news_block";

constexpr TStringBuf CALLBACK_MODE = "mode";
constexpr TStringBuf CALLBACK_MODE_INTRO = "intro";
constexpr TStringBuf CALLBACK_MODE_ENDING = "ending";
constexpr TStringBuf CALLBACK_MODE_ENDING_SHOULD_LISTEN = "ending_should_listen";
constexpr TStringBuf CALLBACK_MODE_NEWS_ITEM = "news_item";
constexpr TStringBuf CALLBACK_MODE_NEWS_LIST_CONTINUE = "news_list_continue";
constexpr TStringBuf CALLBACK_MODE_NEWS_LIST = "news_list";
constexpr TStringBuf CALLBACK_MODE_NOT_NEWS_BLOCK = "not_news_block";
constexpr TStringBuf CALLBACK_MODE_MORE_INFO = "more_info";


class TNextNewsBlockLink {
public:
    TNextNewsBlockLink(TStringBuf mode, int newsIdx);
    void UpdateCallback(NScenarios::TCallbackDirective& callback) const;
    void UpdateFrame(TSemanticFrame& frame) const;
private:
    TStringBuf Mode;
    int NewsArrayPosition;
};

class TNewsBlockVoiceButton : public TNextNewsBlockLink {
public:
    TNewsBlockVoiceButton(TStringBuf frameName, TStringBuf buttonName, TStringBuf mode, int newsIdx);
    TStringBuf GetFrameName() const;
    TStringBuf GetButtonName() const;
private:
    TStringBuf FrameName;
    TStringBuf ButtonName;
};


class TNewsBlock {
public:
    TNewsBlock(
        const NHollywoodFw::TRunRequest& runRequest,
        const NJson::TJsonValue& bassResponse,
        bool isRenderStep);

    bool IsNewsBlock() const;
    bool NeedNewRequest() const;
    const TMaybe<TStringBuf>& GetNewsId() const;
    TStringBuf GetCallbackMode() const;
    const NJson::TJsonValue& GetCurrentNewsItem(const NJson::TJsonValue& bassResponse) const;
    TStringBuf GetNewsUrl(const NJson::TJsonValue& bassResponse) const;
    TStringBuf GetNewsText(const NJson::TJsonValue& bassResponse) const;
    TMaybe<TNextNewsBlockLink> GetNextBlock() const;
    TNewsBlockVoiceButton GetMoreNewsAfterHandoffButton() const;
    void SetVoiceButtons(TVector<TNewsBlockVoiceButton>& voiceButtons) const;
    void UpdateFixList(NJson::TJsonValue& fixList) const;
    bool IsFirstBlock() const;
    bool IsShouldListenBlock() const;
    bool IsEnding() const;

private:
    void AddMoreNewsButton(TVector<TNewsBlockVoiceButton>& voiceButtons) const;
    void AddPreviousNewsButton(TVector<TNewsBlockVoiceButton>& voiceButtons) const;
    void AddMoreInfoButton(TVector<TNewsBlockVoiceButton>& voiceButtons) const;

private:
    TStringBuf Mode;
    int NewsArrayPosition;
    bool HasIntroAndEnding;
    TMaybe<TStringBuf> NewsId;
    int NewsCount;
};

} // namespace NAlice::NHollywood
