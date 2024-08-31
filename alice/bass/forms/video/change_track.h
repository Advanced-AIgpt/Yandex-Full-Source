#pragma once

#include "defs.h"
#include "player_command.h"
#include "show_video_settings.h"
#include "video_command.h"
#include "video.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/player_command/defs.h>

namespace NBASS::NVideo {

inline constexpr TStringBuf LANGUAGE_ANY = "any";
inline constexpr TStringBuf LANGUAGE_RUS = "ru";

inline constexpr TStringBuf VIDEO_SLOT_AUDIO = "audio";
inline constexpr TStringBuf VIDEO_SLOT_SUBTITLES = "subtitles";

inline const TVector<TStringBuf> VIDEO_SLOTS_NUMBER {
    "firstNumber",
    "secondNumber"
};

IContinuation::TPtr ChangeTrack(TContext& ctx);

TResultValue ValidateChangeTrackSlots(
    TContext& ctx,
    const NBassApi::TVideoCurrentlyPlayingConst<TSchemeTraits>& state);

bool TryDefineCommandsUsingDefaultSlots(
    TContext& ctx,
    const NBassApi::TVideoCurrentlyPlayingConst<TSchemeTraits>& state,
    TMaybe<NSc::TValue>& commandAudio,
    TMaybe<NSc::TValue>& commandSubtitles,
    TMaybe<NSc::TValue>& commandShowVideoSettings);

bool TryDefineCommandsUsingNumberSlots(
    TContext& ctx,
    const NBassApi::TVideoCurrentlyPlayingConst<TSchemeTraits>& state,
    TMaybe<NSc::TValue>& commandAudio,
    TMaybe<NSc::TValue>& commandSubtitles);

bool DefineCommandWithNumber(
    TContext& ctx,
    const TAudioStreamOrSubtitleArrayScheme& audioStreams,
    const TAudioStreamOrSubtitleArrayScheme& subtitles,
    TMaybe<NSc::TValue>& commandAudio,
    TMaybe<NSc::TValue>& commandSubtitles,
    const ui32 number);

TMaybe<TAudioStreamOrSubtitle> FindMostSuitableTrack(
    const TAudioStreamOrSubtitleArrayScheme& tracks,
    TStringBuf language);

bool HasSimilarTracks(
    const TAudioStreamOrSubtitleArrayScheme& tracks,
    TStringBuf language,
    TStringBuf currentlyPlayingLanguage);

NSc::TValue ConstructChangeAudioCommand(
    TStringBuf language,
    TStringBuf title);

NSc::TValue ConstructChangeSubtitlesCommand(
    bool enable,
    TStringBuf language,
    TStringBuf title);

} // namespace NBASS::NVideo
