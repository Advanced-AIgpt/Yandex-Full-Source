#include "ondemand.h"

#include "semantic_frames.h"

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/megamind/protos/common/frame.pb.h>

#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/frame/slot.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/library/music/defs.h>

#include <alice/library/json/json.h>

#include <cstddef>
#include <util/generic/strbuf.h>
#include <util/string/join.h>
#include <util/string/strip.h>

namespace NAlice::NHollywood::NMusic {

namespace {

TSemanticFrame::TSlot CreateSearchTextSlotForFairytale(const TString& fairyTale) {
    // теггеры music_play и music_fairy_tale по-разному обрабатывают слово "сказка":
    // music_fairy_tale никогда его не захватывает, поэтому его нужно подклеить к поисковому запросу
    // music_play почти всегда захватывает это слово и его аналоги ("аудиосказка", "сказочка"), поэтому подклеивать слово "сказка" не нужно
    const TString searchText = fairyTale.find("сказ") == TString::npos
        ? "сказка " + fairyTale
        : fairyTale;
    return CreateProtoSlot(
        TString{NAlice::NMusic::SLOT_SEARCH_TEXT},
        TString{NAlice::NMusic::SLOT_SEARCH_TEXT_TYPE},
        searchText);
}

TSemanticFrame::TSlot CreateFilterGenreSlot() {
    return CreateProtoSlot(
        TString{NAlice::NMusic::SLOT_IS_FAIRY_TALE_FILTER_GENRE},
        TString{NAlice::NMusic::SLOT_IS_FAIRY_TALE_FILTER_GENRE_TYPE},
        "true"
    );
}

TMaybe<TSemanticFrame::TSlot> CreateOffsetSlot(const TFrame& musicFairyTaleFrame) {
    const auto offsetSlot = musicFairyTaleFrame.FindSlot(NAlice::NMusic::SLOT_OFFSET);
    if (offsetSlot == nullptr) {
        return Nothing();
    }
    return CreateProtoSlot(
        TString{NAlice::NMusic::SLOT_OFFSET},
        TString{NAlice::NMusic::SLOT_OFFSET_TYPE},
        offsetSlot->Value.AsString());
}

TMaybe<TSemanticFrame> ProcessInputFrame(TRTLogger& logger, const TFrame& inputFrame, const TStringBuf searchTextSlotName) {
    const auto searchTextSlots = inputFrame.FindSlots(searchTextSlotName);
    if (searchTextSlots == nullptr || searchTextSlots->empty()) {
        LOG_INFO(logger) << "Cannot create ondemand fairy tale frame: fairy tale name is empty";
        return Nothing();
    }

    TVector<TStringBuf> searchTextSlotValues;
    for (const auto& searchTextSlot: *searchTextSlots) {
        searchTextSlotValues.emplace_back(searchTextSlot.Value.AsString());
    }
    const TString searchText = StripString(JoinSeq(" ", searchTextSlotValues));
    if (searchText.Empty()) {
        LOG_INFO(logger) << "Cannot create ondemand fairy tale frame: all slots are empty";
        return Nothing();
    }

    TSemanticFrame bassFrame;
    bassFrame.SetName(TString{MUSIC_PLAY_FRAME});
    *bassFrame.AddSlots() = CreateSearchTextSlotForFairytale(searchText);
    *bassFrame.AddSlots() = CreateFilterGenreSlot();
    if (auto offsetSlot = CreateOffsetSlot(inputFrame); offsetSlot.Defined()) {
        *bassFrame.AddSlots() = offsetSlot.GetRef();
    }
    LOG_DEBUG(logger) << "Created fairy tale ondemand frame: " << JsonStringFromProto(bassFrame);
    return bassFrame;
}

} // namespace

TMaybe<TSemanticFrame> TryCreateOnDemandFairyTaleFrame(
    TRTLogger& logger,
    const TFrame* musicFairyTale,
    const TFrame* musicPlay)
{
    if (musicFairyTale) {
        LOG_INFO(logger) << "Using music_fairy_tale frame to create fairy tale search frame: " << musicFairyTale->ToProto().AsJSON();
        return ProcessInputFrame(logger, *musicFairyTale, NAlice::NMusic::SLOT_FAIRY_TALE);
    } else if (musicPlay) {
        LOG_INFO(logger) << "Using music_play frame to create fairy tale search frame";
        return ProcessInputFrame(logger, *musicPlay, NAlice::NMusic::SLOT_SEARCH_TEXT);
    } else {
        return Nothing();
    }
}

} // namespace NAlice::NHollywood::NMusic
