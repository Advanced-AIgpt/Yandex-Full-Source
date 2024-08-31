#include "commands.h"

#include <alice/hollywood/library/scenarios/music/common.h>

#include <alice/library/music/defs.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf NO_EXP = {};

const TVector<std::tuple<TStringBuf, TMusicArguments::EPlayerCommand, TStringBuf>> PLAYER_FRAMES_ORDER = {
    {NAlice::NMusic::MUSIC_PLAYER_CHANGE_TRACK_VERSION, TMusicArguments_EPlayerCommand_ChangeTrackVersion, EXP_HW_MUSIC_CHANGE_TRACK_VERSION},
    {NAlice::NMusic::PLAYER_NEXT_TRACK, TMusicArguments_EPlayerCommand_NextTrack, NO_EXP},
    {NAlice::NMusic::PLAYER_PREV_TRACK, TMusicArguments_EPlayerCommand_PrevTrack, NO_EXP},
    {NAlice::NMusic::MUSIC_PLAYER_CHANGE_TRACK_NUMBER, TMusicArguments_EPlayerCommand_ChangeTrackNumber, EXP_HW_MUSIC_CHANGE_TRACK_NUMBER},
    {NAlice::NMusic::PLAYER_CONTINUE, TMusicArguments_EPlayerCommand_Continue, NO_EXP},
    {NAlice::NMusic::MUSIC_PLAYER_CONTINUE, TMusicArguments_EPlayerCommand_Continue, NO_EXP},
    {NAlice::NMusic::PLAYER_LIKE, TMusicArguments_EPlayerCommand_Like, NO_EXP},
    {NAlice::NMusic::PLAYER_DISLIKE, TMusicArguments_EPlayerCommand_Dislike, NO_EXP},
    {NAlice::NMusic::PLAYER_SHUFFLE, TMusicArguments_EPlayerCommand_Shuffle, NO_EXP},
    {NAlice::NMusic::PLAYER_UNSHUFFLE, TMusicArguments_EPlayerCommand_Unshuffle, NO_EXP},
    {NAlice::NMusic::PLAYER_REPLAY, TMusicArguments_EPlayerCommand_Replay, NO_EXP},
    {NAlice::NMusic::PLAYER_REWIND, TMusicArguments_EPlayerCommand_Rewind, NO_EXP},
    {NAlice::NMusic::PLAYER_REPEAT, TMusicArguments_EPlayerCommand_Repeat, NO_EXP},
};

} // namespace

TMusicArguments::EPlayerCommand FindPlayerCommand(const TScenarioBaseRequestWithInputWrapper& request) {
    const auto& input = request.Input();
    for (const auto& [frameName, command, exp] : PLAYER_FRAMES_ORDER) {
        if (exp && !request.HasExpFlag(exp)) {
            continue;
        }
        if (input.FindSemanticFrame(frameName)) {
            return command;
        }
    }
    return TMusicArguments_EPlayerCommand_None;
}

const TPtrWrapper<NAlice::TSemanticFrame> FindPlayerFrame(const TScenarioBaseRequestWithInputWrapper& request) {
    const auto& input = request.Input();
    for (const auto& [frameName, command, exp] : PLAYER_FRAMES_ORDER) {
        if (exp && !request.HasExpFlag(exp)) {
            continue;
        }
        if (const auto frame = input.FindSemanticFrame(frameName)) {
            return frame;
        }
    }
    return TPtrWrapper<NAlice::TSemanticFrame>(nullptr, "FindPlayerFrame");
}

} // namespace NAlice::NHollywood::NMusic
