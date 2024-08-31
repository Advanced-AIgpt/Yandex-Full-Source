#pragma once

#include "alice/hollywood/library/framework/framework.h"
#include "alice/hollywood/library/framework/core/semantic_frames.h"
#include "alice/library/sys_datetime/sys_datetime.h"
#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/request/request.h>

namespace NAlice::NHollywood {

constexpr TStringBuf GET_NEWS_FRAME = "personal_assistant.scenarios.get_news";
constexpr TStringBuf GET_FREE_NEWS_FRAME = "personal_assistant.scenarios.get_free_news";
constexpr TStringBuf GET_NEWS_SETTINGS_FRAME = "personal_assistant.scenarios.get_news_settings";
constexpr TStringBuf GET_NEWS_POSTROLL_ANSWER_FRAME = "personal_assistant.scenarios.get_news__postroll_answer";
constexpr TStringBuf COLLECT_CARDS_FRAME = "alice.centaur.collect_cards";
constexpr TStringBuf COLLECT_TEASERS_PREVIEW_FRAME = "alice.centaur.collect_teasers_preview";
constexpr TStringBuf COLLECT_MAIN_SCREEN_FRAME = "alice.centaur.collect_main_screen";
constexpr TStringBuf COLLECT_MAIN_SCREEN_NEWS_FRAME = "alice.centaur.collect_main_screen.widgets.news";
constexpr TStringBuf COLLECT_WIDGET_GALLERY_FRAME = "alice.centaur.collect_widget_gallery";
constexpr TStringBuf GET_DETAILED_NEWS = "alice.search_factoid_src";

constexpr TStringBuf GET_MORE_NEWS_FRAME = "personal_assistant.scenarios.get_news__more";
constexpr TStringBuf GET_MORE_NEWS_BUTTON = "get_news__more";
constexpr TStringBuf GET_PREVIOUS_NEWS_FRAME = "personal_assistant.scenarios.get_news__previous";
constexpr TStringBuf GET_PREVIOUS_NEWS_BUTTON = "get_news__previous";
constexpr TStringBuf GET_MORE_INFO_FRAME = "alice.external_skill.flash_briefing.details";
constexpr TStringBuf GET_MORE_INFO_BUTTON = "get_news__more_info";
constexpr TStringBuf GET_MORE_NEWS_CONFIRM_FRAME = "alice.proactivity.confirm";
constexpr TStringBuf GET_MORE_NEWS_CONFIRM_BUTTON = "get_news__confirm";

constexpr TStringBuf TOPIC_SLOT = "topic";
constexpr TStringBuf TOPIC_NOT_NEWS_SLOT = "topic_not_news";
constexpr TStringBuf TOPIC_SLOT_MEMENTO_RUBRIC_TYPE = "memento.news_topic";
constexpr TStringBuf WHERE_SLOT = "where";
constexpr TStringBuf DATE_SLOT = "date";
constexpr TStringBuf SINGLE_NEWS_SLOT = "single_news"; // <--------- This slot doesn't exist
constexpr TStringBuf SKIP_INTRO_AND_ENDING_SLOT = "skip_intro_and_ending";
constexpr TStringBuf DISABLE_VOICE_BUTTONS_SLOT = "disable_voice_buttons";
constexpr TStringBuf NEWS_IDX_SLOT = "news_idx";

constexpr auto EMPTY_FRAME_MODE = NHollywoodFw::TFrame::EFrameConstructorMode::Empty;

struct TFrameNews: public NHollywoodFw::TFrame {
    // name can be: GET_NEWS_FRAME, GET_FREE_NEWS_FRAME, COLLECT_CARDS_FRAME, COLLECT_MAIN_SCREEN_FRAME, COLLECT_WIDGET_GALLERY_FRAME
    TFrameNews(const NHollywoodFw::TRunRequest& request, const TStringBuf name)
        : TFrame(request, name, NHollywoodFw::TFrame::EFrameConstructorMode::FrameWithCallback)
        , ArrayPosition(this, NEWS_IDX_SLOT)
        , SkipIntroAndEnding(this, SKIP_INTRO_AND_ENDING_SLOT)
        , TopicSlot(this, TOPIC_SLOT)
        , WhereSlot(this, WHERE_SLOT)
        , DateSlot(this, DATE_SLOT)
        , NotNewsSlot(this, TOPIC_NOT_NEWS_SLOT)
    {
    }
    TFrameNews(const NHollywoodFw::TRunRequest& request, const TStringBuf name, const NHollywoodFw::TFrame::EFrameConstructorMode mode)
        : TFrame(request, name, mode)
        , ArrayPosition(this, NEWS_IDX_SLOT)
        , SkipIntroAndEnding(this, SKIP_INTRO_AND_ENDING_SLOT)
        , TopicSlot(this, TOPIC_SLOT)
        , WhereSlot(this, WHERE_SLOT)
        , DateSlot(this, DATE_SLOT)
        , NotNewsSlot(this, TOPIC_NOT_NEWS_SLOT)
    {
    }
    NHollywoodFw::TOptionalSlot<int> ArrayPosition;
    NHollywoodFw::TOptionalSlot<bool> SkipIntroAndEnding;
    NHollywoodFw::TOptionalSlot<TString> TopicSlot;
    NHollywoodFw::TOptionalSlot<TString> WhereSlot;
    NHollywoodFw::TOptionalSlot<TSysDatetimeParser> DateSlot;
    NHollywoodFw::TOptionalSlot<TString> NotNewsSlot;

};

struct TFrameSettings: public NHollywoodFw::TFrame {
    // name can be: GET_NEWS_SETTINGS_FRAME, GET_NEWS_POSTROLL_ANSWER_FRAME
    TFrameSettings(const NHollywoodFw::TRunRequest& request, const TStringBuf name)
        : TFrame(request, name, NHollywoodFw::TFrame::EFrameConstructorMode::FrameWithCallback)
        , Answer(this, "Answer")
        , Topic(this, "Topic")
        , RadioSource(this, "RadioSource")
    {
    }
    NHollywoodFw::TOptionalSlot<TString> Answer;
    NHollywoodFw::TOptionalSlot<TString> Topic;
    NHollywoodFw::TOptionalSlot<TString> RadioSource;
};

TMaybe<TFrame> TryGetNewsFrame(const TScenarioRunRequestWrapper& runRequest);

bool GetHasIntroAndEnding(const TScenarioRunRequestWrapper& runRequest);
bool GetHasIntroAndEnding(const TMaybe<TFrame>& frame, const TScenarioRunRequestWrapper& runRequest);
bool GetDisableVoiceButtons(const TScenarioRunRequestWrapper& runRequest);
bool GetDisableVoiceButtons(const TMaybe<TFrame>& frame);
TMaybe<int> TryGetNewsArrayPosition(const TMaybe<TFrame>& frame);
bool IsSingleNewsRequest(const TMaybe<TFrame>& frame);

} // namespace NAlice::NHollywood
