#pragma once

#include <alice/bass/forms/registrator.h>
#include <alice/bass/forms/vins.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>

namespace NBASS {

class TContext;

namespace NSerpGallery {

inline constexpr TStringBuf SLOT_NAME_ITEMS = "serp_items";
inline constexpr TStringBuf SLOT_TYPE_ITEMS = "serp_items";

inline constexpr TStringBuf FIELD_CALL_URI = "call_uri";
inline constexpr TStringBuf FIELD_MAP_URI = "map_uri";
inline constexpr TStringBuf FIELD_URL = "url";

inline constexpr TStringBuf EXP_FLAG_ENABLE_SERP_GALLERY_DEBUG = "enable_serp_gallery_debug";

extern const TVector<TFormHandlerPair> FORM_HANDLER_PAIRS;

class TVoiceAnswerBuilder {
public:
    static TMaybe<TVoiceAnswerBuilder> Create(TContext& ctx);

    void Build(TContext& ctx, const NSc::TValue& items, const i64 itemId, const bool splitTTS = false, const bool addReadableTTSUrl = false);
    void SetVoiceSuggests(const TStringBuf ttsContinuation, const NSc::TValue& items, const i64 id, TContext& ctx);

private:
    explicit TVoiceAnswerBuilder(TContext& ctx);

    void PrepareTTS(const NSc::TValue& serpItem, TString& firstPart, TString& secondPart);

private:
    size_t MaxNumSentenses;
    size_t MaxNumChars;

    // Debug option activated by experimental flags
    bool EnableNavigation;
};

class TSerpGalleryBuilder {
public:
    TContext* Ctx;

    static TMaybe<TSerpGalleryBuilder> Create(TContext& ctx);

    void Build(const TStringBuf query, const NSc::TValue& searchResult);

private:
    explicit TSerpGalleryBuilder(TVoiceAnswerBuilder&& voiceAnswerBuilder, TContext& ctx);

    TMaybe<NSc::TValue> BuildSerpGalleryItem(const NSc::TValue& document);

private:
    TVoiceAnswerBuilder VoiceAnswerBuilder;

    // Options activated by experimental flags
    bool MarkAsVoice;
    bool AddPronounceButton;
    bool AddExtraButtons;
    bool NoVoiceAtStart;
    bool EnableLogIdForUrl;
};

} // NSerpGallery
} // NBASS
