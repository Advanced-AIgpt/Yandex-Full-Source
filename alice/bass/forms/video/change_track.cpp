#include "change_track.h"
#include "utils.h"

#include <alice/bass/forms/directives.h>

#include <alice/library/video_common/audio_and_subtitle_helper.h>

#include <util/generic/ymath.h>

namespace NBASS::NVideo {

IContinuation::TPtr ChangeTrack(TContext& ctx) {
    if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_CHANGE_TRACK)) {
        const auto errorMsg = TString::Join(
            "Experimental flag \"", EXPERIMENTAL_FLAG_DISABLE_CHANGE_TRACK, "\" is enabled");
        TResultValue error = TError(TError::EType::PROTOCOL_IRRELEVANT, errorMsg);
        return TCompletedContinuation::Make(ctx, error);
    }

    if (GetCurrentScreen(ctx) != EScreenId::VideoPlayer) {
        const TStringBuf errorMsg = "Screen is not supported";
        TResultValue error = TError(TError::EType::PROTOCOL_IRRELEVANT, errorMsg);
        return TCompletedContinuation::Make(ctx, error);
    }

    using TState = NBassApi::TVideoCurrentlyPlaying<TSchemeTraits>::TConst;
    TState state(ctx.Meta().DeviceState().Video().CurrentlyPlaying().GetRawValue());

    if (const auto& error = ValidateChangeTrackSlots(ctx, state); error.Defined()) {
        return TCompletedContinuation::Make(ctx, error);
    }

    AddAnalyticsInfoFromVideoCommand(ctx);

    if (state.Item().AudioStreams().Empty() && state.Item().Subtitles().Empty()) {
        ctx.AddAttention(ATTENTION_VIDEO_IRRELEVANT_PROVIDER);
        return TCompletedContinuation::Make(ctx);
    }

    TMaybe<NSc::TValue> commandAudio;
    TMaybe<NSc::TValue> commandSubtitles;
    TMaybe<NSc::TValue> commandShowVideoSettings;
    if (TryDefineCommandsUsingDefaultSlots(ctx, state, commandAudio, commandSubtitles, commandShowVideoSettings) &&
        TryDefineCommandsUsingNumberSlots(ctx, state, commandAudio, commandSubtitles))
    {
        if (commandAudio) {
            ctx.AddCommand<TChangeAudioDirective>(NAlice::NVideoCommon::COMMAND_CHANGE_AUDIO, commandAudio.GetRef());
        }

        if (commandSubtitles) {
            // if SubtitlesButtonEnable is disabled, then we should not turn off subtitles when a foreign language is playing
            if (!state.Item().PlayerRestrictionConfig().SubtitlesButtonEnable() && !commandSubtitles.GetRef()["enable"].GetBool() &&
                ((commandAudio && !commandAudio.GetRef()["language"].GetString().StartsWith(LANGUAGE_RUS)) ||
                (!commandAudio && !state.AudioLanguage()->StartsWith(LANGUAGE_RUS))))
            {
                ctx.AddAttention(ATTENTION_VIDEO_CANNOT_TURN_OFF_SUBTITLES);
            } else {
                ctx.AddCommand<TChangeSubtitlesDirective>(NAlice::NVideoCommon::COMMAND_CHANGE_SUBTITLES, commandSubtitles.GetRef());
            }
        }
    } else if (commandShowVideoSettings && !ctx.HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_SHOW_VIDEO_SETTINGS)) {
        AddShowVideoSettingsCommandAndShouldListen(ctx);
    }

    return TCompletedContinuation::Make(ctx);
}

TResultValue ValidateChangeTrackSlots(
    TContext& ctx,
    const NBassApi::TVideoCurrentlyPlayingConst<TSchemeTraits>& state)
{
    const auto& audioStreams = state.Item().AudioStreams();
    const auto& subtitles = state.Item().Subtitles();

    for (const auto& slotName : VIDEO_SLOTS_NUMBER) {
        if (const auto* numberSlot = ctx.GetSlot(slotName); !IsSlotEmpty(numberSlot)) {
            ui32 number;
            if (!TryFromString<ui32>(numberSlot->Value.GetString(), number)) {
                const auto errorMsg = TString::Join("Value ", slotName, " is not defined");
                return TError(TError::EType::PROTOCOL_IRRELEVANT, errorMsg);
            }

            if (audioStreams.Empty() && subtitles.Empty()) {
                const TStringBuf errorMsg = "No audioStreams and subtitles for this video, number value is not needed";
                return TError(TError::EType::PROTOCOL_IRRELEVANT, errorMsg);
            }

            if (number > audioStreams.Size() + subtitles.Size() + CHANGE_TRACK_ADDITIONAL_THRESHOLD) {
                const auto errorMsg = TString::Join("Slot ", slotName, " has big number");
                return TError(TError::EType::PROTOCOL_IRRELEVANT, errorMsg);
            }
        }
    }

    return ResultSuccess();
}

bool TryDefineCommandsUsingDefaultSlots(
    TContext& ctx,
    const NBassApi::TVideoCurrentlyPlayingConst<TSchemeTraits>& state,
    TMaybe<NSc::TValue>& commandAudio,
    TMaybe<NSc::TValue>& commandSubtitles,
    TMaybe<NSc::TValue>& commandShowVideoSettings)
{
    const auto& audioStreams = state.Item().AudioStreams();
    const auto& subtitles = state.Item().Subtitles();

    bool result = true;
    if (const auto* audioSlot = ctx.GetSlot(VIDEO_SLOT_AUDIO); !IsSlotEmpty(audioSlot)) {
        const TStringBuf language = audioSlot->Value.GetString();

        if (const auto& bestTrack = FindMostSuitableTrack(audioStreams, language); bestTrack.Defined()) {
            commandAudio = ConstructChangeAudioCommand(
                bestTrack.GetRef()->Language(),
                bestTrack.GetRef()->Title());
        } else {
            ctx.AddAttention(ATTENTION_VIDEO_NO_SUCH_AUDIO_STREAM);
            if (HasSimilarTracks(audioStreams, language, state.AudioLanguage())) {
                ctx.AddAttention(ATTENTION_VIDEO_HAS_SIMILAR_AUDIO_STREAMS);
                commandShowVideoSettings = NSc::TValue{};
            }
            result = false;
        }
    }

    if (const auto* subtitlesSlot = ctx.GetSlot(VIDEO_SLOT_SUBTITLES); !IsSlotEmpty(subtitlesSlot)) {
        const TStringBuf language = subtitlesSlot->Value.GetString();

        if (language == NAlice::NVideoCommon::LANGUAGE_SUBTITLE_OFF) {
            commandSubtitles = ConstructChangeSubtitlesCommand(
                false /* enable */,
                TStringBuf{} /* language */,
                TStringBuf{} /* title */);
        } else if (language == LANGUAGE_ANY) {
            if (subtitles.Size() > 1) {
                commandSubtitles = ConstructChangeSubtitlesCommand(
                    true /* enable */,
                    subtitles[1]->Language(),
                    subtitles[1]->Title());
            } else {
                ctx.AddAttention(ATTENTION_VIDEO_NO_ANY_SUBTITLES);
                result = false;
            }
        } else if (const auto& bestTrack = FindMostSuitableTrack(subtitles, language); bestTrack.Defined()) {
            commandSubtitles = ConstructChangeSubtitlesCommand(
                true /* enable */,
                bestTrack.GetRef()->Language(),
                bestTrack.GetRef()->Title());
        } else {
            ctx.AddAttention(ATTENTION_VIDEO_NO_SUCH_SUBTITLE);
            if (HasSimilarTracks(subtitles, language, state.SubtitlesLanguage())) {
                ctx.AddAttention(ATTENTION_VIDEO_HAS_SIMILAR_SUBTITLES);
                commandShowVideoSettings = NSc::TValue{};
            }
            result = false;
        }
    }

    if (const auto* audioSlot = ctx.GetSlot(VIDEO_SLOT_AUDIO); !IsSlotEmpty(audioSlot)) {
        if (const auto* subtitlesSlot = ctx.GetSlot(VIDEO_SLOT_SUBTITLES); !IsSlotEmpty(subtitlesSlot)) {
            ctx.AddAttention(ATTENTION_VIDEO_BOTH_TRACK_TYPES);
            if (!result) {
                commandShowVideoSettings = NSc::TValue{};
            }
        }
    }

    return result;
}

bool TryDefineCommandsUsingNumberSlots(
    TContext& ctx,
    const NBassApi::TVideoCurrentlyPlayingConst<TSchemeTraits>& state,
    TMaybe<NSc::TValue>& commandAudio,
    TMaybe<NSc::TValue>& commandSubtitles)
{
    const auto& audioStreams = state.Item().AudioStreams();
    const auto& subtitles = state.Item().Subtitles();

    bool result = true;
    /* NOTE(amullanurov@): Now there are only two separate slots: "firstNumber" and "secondNumber",
    because MM gets only one value for each slot, in future this will be changed
    to one slot "Number" with list of values.
    Value number is 1-indexed */
    for (const auto& slotName : VIDEO_SLOTS_NUMBER) {
        if (const auto* numberSlot = ctx.GetSlot(slotName); !IsSlotEmpty(numberSlot)) {
            ui32 number = FromString<ui32>(numberSlot->Value.GetString());
            result = DefineCommandWithNumber(ctx, audioStreams, subtitles, commandAudio, commandSubtitles, number);
        }
    }

    return result;
}

bool DefineCommandWithNumber(
    TContext& ctx,
    const TAudioStreamOrSubtitleArrayScheme& audioStreams,
    const TAudioStreamOrSubtitleArrayScheme& subtitles,
    TMaybe<NSc::TValue>& commandAudio,
    TMaybe<NSc::TValue>& commandSubtitles,
    const ui32 number)
{
    const ui32 index = number - 1;

    if (index >= audioStreams.Size() + subtitles.Size()) {
        ctx.AddAttention(ATTENTION_VIDEO_IRRELEVANT_NUMBER);
        return false;
    }

    if (index < audioStreams.Size()) {
        if (!commandAudio) {
            commandAudio = ConstructChangeAudioCommand(
                audioStreams[index]->Language(),
                audioStreams[index]->Title());
        }
    } else if (index < audioStreams.Size() + subtitles.Size()) {
        if (!commandSubtitles) {
            ui32 subtitlesIndex = index - audioStreams.Size();
            const TStringBuf language = subtitles[subtitlesIndex]->Language();

            if (language == NAlice::NVideoCommon::LANGUAGE_SUBTITLE_OFF) {
                NSc::TValue commandOff;
                commandSubtitles = ConstructChangeSubtitlesCommand(
                    false /* enable */,
                    TStringBuf{} /* language */,
                    TStringBuf{} /* title */);
            } else {
                NSc::TValue commandOn;
                commandSubtitles = ConstructChangeSubtitlesCommand(
                    true /* enable */,
                    language,
                    subtitles[subtitlesIndex]->Title());
            }
        }
    }

    return true;
}

TMaybe<TAudioStreamOrSubtitle> FindMostSuitableTrack(
    const TAudioStreamOrSubtitleArrayScheme& tracks,
    TStringBuf language)
{
    TAudioStreamOrSubtitle result;

    for (const auto& track : tracks) {
        const TStringBuf currentLanguage = track.Language();

        if (currentLanguage.EndsWith(NAlice::NVideoCommon::SUFFIX_LANG_18PLUS) &&
            language == NAlice::NVideoCommon::LANGUAGE_18PLUS)
        {
            result->Language() = currentLanguage;
            result->Title() = track.Title();
            return result;
        }

        if (currentLanguage.StartsWith(language) &&
            (result.IsNull() || currentLanguage.size() < result->Language()->size()))
        {
            result->Language() = currentLanguage;
            result->Title() = track.Title();
        }
    }

    if (!result.IsNull()) {
        return result;
    }

    if (NAlice::NVideoCommon::SHORT_LANGUAGE_FORMAT.contains(language)) {
        return FindMostSuitableTrack(tracks, NAlice::NVideoCommon::SHORT_LANGUAGE_FORMAT.at(language));
    }

    return Nothing();
}

/**
 * Similar track should have the same main language, i.e. first 3 letters ("rus", "eng" and etc.).
 * Currently playing track will not be considered as similar.
 * It is needed for context menu with information about ohter possible languages.
 */
bool HasSimilarTracks(
    const TAudioStreamOrSubtitleArrayScheme& tracks,
    TStringBuf language,
    TStringBuf currentlyPlayingLanguage)
{
    const TStringBuf mainLanguage = language.Head(3);

    for (const auto& track : tracks) {
        const TStringBuf currentLanguage = track.Language();

        if (language != currentLanguage &&
            currentlyPlayingLanguage != currentLanguage &&
            mainLanguage == currentLanguage.Head(3))
        {
            return true;
        }
    }

    return false;
}

NSc::TValue ConstructChangeAudioCommand(
    TStringBuf language,
    TStringBuf title)
{
    NSc::TValue command;
    command["language"].SetString(language);
    command["title"].SetString(title);
    return command;
}

NSc::TValue ConstructChangeSubtitlesCommand(
    bool enable,
    TStringBuf language,
    TStringBuf title)
{
    NSc::TValue command;
    if (!enable) {
        command["enable"].SetBool(false);
        return command;
    }

    command["enable"].SetBool(true);
    command["language"].SetString(language);
    command["title"].SetString(title);
    return command;
}

} // namespace NBASS::NVideo
