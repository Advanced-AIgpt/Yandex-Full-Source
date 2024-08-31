#include "music_dispatcher.h"

#include "centaur.h"
#include "elari_watch.h"
#include "equalizer.h"
#include "multiroom_redirect.h"
#include "play_less.h"
#include "start_multiroom.h"
#include "tandem_follower.h"

#include <alice/hollywood/library/scenarios/music/scene/common/common_args.h>
#include <alice/hollywood/library/scenarios/music/scene/common/render.h>

#include <alice/hollywood/library/scenarios/music/scene/fm_radio/args_builder.h>
#include <alice/hollywood/library/scenarios/music/scene/fm_radio/fm_radio.h>

#include <alice/hollywood/library/scenarios/music/scene/player_command/common.h>
#include <alice/hollywood/library/scenarios/music/scene/player_command/remove_dislike.h>
#include <alice/hollywood/library/scenarios/music/scene/player_command/remove_like.h>
#include <alice/hollywood/library/scenarios/music/scene/player_command/repeat.h>
#include <alice/hollywood/library/scenarios/music/scene/player_command/rewind.h>
#include <alice/hollywood/library/scenarios/music/scene/player_command/send_song_text.h>
#include <alice/hollywood/library/scenarios/music/scene/player_command/shuffle.h>
#include <alice/hollywood/library/scenarios/music/scene/player_command/songs_by_this_artist.h>
#include <alice/hollywood/library/scenarios/music/scene/player_command/what_album_is_this_song_from.h>
#include <alice/hollywood/library/scenarios/music/scene/player_command/what_is_this_song_about.h>
#include <alice/hollywood/library/scenarios/music/scene/player_command/what_is_playing.h>
#include <alice/hollywood/library/scenarios/music/scene/player_command/what_year_is_this_song.h>

#include <alice/hollywood/library/music/music_resources.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/nlg/register.h>

#include <alice/hollywood/library/environment_state/environment_state.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/music/defs.h>
#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

namespace {

void LogBlackBoxUserInfo(TRTLogger& logger, const NScenarios::TDataSource* dataSource) {
    if (dataSource) {
        // NOTE: We are not allowed to log here any personal data, such as addresses, emails and phones...
        const auto& userInfo = dataSource->GetUserInfo();
        LOG_INFO(logger) << "BlackBoxUserInfo: Uid=" << userInfo.GetUid()
                         << " HasYandexPlus=" << userInfo.GetHasYandexPlus()
                         << " IsStaff=" << userInfo.GetIsStaff()
                         << " IsBetaTester=" << userInfo.GetIsBetaTester()
                         << " HasMusicSubscription=" << userInfo.GetHasMusicSubscription()
                         << " MusicSubscriptionRegionId=" << userInfo.GetMusicSubscriptionRegionId();
    } else {
        LOG_INFO(logger) << "BlackBoxUserInfo is nullptr";
    }
}

} // namespace

TMusicScenario::TMusicScenario()
    : TScenario{"music"}
{
    // register dispatcher
    Register(&TMusicScenario::Dispatch);

    // register common renderers
    RegisterRenderer(&CommonRender);

    // register scenes
    RegisterScene<TMusicScenarioSceneCentaur>([this]() {
        RegisterSceneFn(&TMusicScenarioSceneCentaur::Main);
        RegisterSceneFn(&TMusicScenarioSceneCentaur::ContinueSetup);
        RegisterSceneFn(&TMusicScenarioSceneCentaur::Continue);
    });

    RegisterScene<TMusicScenarioSceneElariWatch>([this]() {
        RegisterSceneFn(&TMusicScenarioSceneElariWatch::Main);
    });

    RegisterScene<TMusicScenarioSceneEqualizer>([this]() {
        RegisterSceneFn(&TMusicScenarioSceneEqualizer::Main);
    });

    RegisterScene<TMusicScenarioSceneMultiroomRedirect>([this]() {
        RegisterSceneFn(&TMusicScenarioSceneMultiroomRedirect::Main);
    });

    RegisterScene<NFmRadio::TMusicScenarioSceneFmRadio>([this]() {
        RegisterSceneFn(&NFmRadio::TMusicScenarioSceneFmRadio::Main);
        RegisterSceneFn(&NFmRadio::TMusicScenarioSceneFmRadio::ContinueSetup);
        RegisterSceneFn(&NFmRadio::TMusicScenarioSceneFmRadio::Continue);
    });

    RegisterScene<TMusicScenarioScenePlayLess>([this]() {
        RegisterSceneFn(&TMusicScenarioScenePlayLess::Main);
    });

    RegisterScene<TMusicScenarioSceneTandemFollower>([this]() {
        RegisterSceneFn(&TMusicScenarioSceneTandemFollower::Main);
    });

    // player_command
    {
        RegisterScene<TMusicScenarioScenePlayerCommandRemoveLike>([this]() {
            RegisterSceneFn(&TMusicScenarioScenePlayerCommandRemoveLike::Main);
            RegisterSceneFn(&TMusicScenarioScenePlayerCommandRemoveLike::CommitSetup);
            RegisterSceneFn(&TMusicScenarioScenePlayerCommandRemoveLike::Commit);
        });
        RegisterScene<TMusicScenarioScenePlayerCommandRemoveDislike>([this]() {
            RegisterSceneFn(&TMusicScenarioScenePlayerCommandRemoveDislike::Main);
            RegisterSceneFn(&TMusicScenarioScenePlayerCommandRemoveDislike::CommitSetup);
            RegisterSceneFn(&TMusicScenarioScenePlayerCommandRemoveDislike::Commit);
        });
        RegisterScene<TMusicScenarioScenePlayerCommandRepeat>([this]() {
            RegisterSceneFn(&TMusicScenarioScenePlayerCommandRepeat::Main);
        });
        RegisterScene<TMusicScenarioScenePlayerCommandRewind>([this]() {
            RegisterSceneFn(&TMusicScenarioScenePlayerCommandRewind::Main);
        });
        RegisterScene<TMusicScenarioScenePlayerCommandSendSongText>([this]() {
            RegisterSceneFn(&TMusicScenarioScenePlayerCommandSendSongText::Main);
        });
        RegisterScene<TMusicScenarioScenePlayerCommandShuffle>([this]() {
            RegisterSceneFn(&TMusicScenarioScenePlayerCommandShuffle::Main);
        });
        RegisterScene<TMusicScenarioScenePlayerCommandSongsByThisArtist>([this]() {
            RegisterSceneFn(&TMusicScenarioScenePlayerCommandSongsByThisArtist::Main);
            RegisterSceneFn(&TMusicScenarioScenePlayerCommandSongsByThisArtist::ContinueSetup);
            RegisterSceneFn(&TMusicScenarioScenePlayerCommandSongsByThisArtist::Continue);
        });
        RegisterScene<TMusicScenarioScenePlayerCommandWhatAlbumIsThisSongFrom>([this]() {
            RegisterSceneFn(&TMusicScenarioScenePlayerCommandWhatAlbumIsThisSongFrom::Main);
        });
        RegisterScene<TMusicScenarioScenePlayerCommandWhatIsThisSongAbout>([this]() {
            RegisterSceneFn(&TMusicScenarioScenePlayerCommandWhatIsThisSongAbout::Main);
        });
        RegisterScene<TMusicScenarioScenePlayerCommandWhatIsPlaying>([this]() {
            RegisterSceneFn(&TMusicScenarioScenePlayerCommandWhatIsPlaying::Main);
        });
        RegisterScene<TMusicScenarioScenePlayerCommandWhatYearIsThisSong>([this]() {
            RegisterSceneFn(&TMusicScenarioScenePlayerCommandWhatYearIsThisSong::Main);
        });
    }

    RegisterScene<TMusicScenarioSceneStartMultiroom>([this]() {
        RegisterSceneFn(&TMusicScenarioSceneStartMultiroom::Main);
    });

    // register graph
    SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NMusic::NNlg::RegisterAll);
    SetApphostGraph(ScenarioRequest() >>
                    NodeRun("prepare") >>
                    NodeMain("render") >>
                    ScenarioResponse());
    SetApphostGraph(ScenarioContinue() >>
                    NodeContinueSetup("continue_prepare") >>
                    NodeContinue("continue_render") >>
                    ScenarioResponse());
    SetApphostGraph(ScenarioCommit() >>
                    NodeCommitSetup("commit_prepare") >>
                    NodeCommit("commit_render") >>
                    ScenarioResponse());

    SetResources<NHollywood::NMusic::TMusicResources>();

    // DivRender is prepared at `Main` stage
    SetDivRenderMode(EDivRenderMode::PrepareForOutsideMerge);
}

TRetScene TMusicScenario::Dispatch(const TRunRequest& req, const TStorage& storage, const TSource& source) const {
    // get and log scenario state
    TScenarioRequestData requestData{.Request = req, .Storage = const_cast<TStorage&>(storage), .Source = &source};
    TScenarioStateData state{requestData};
    state.DontSaveChangedState();

    if (state.HaveState) {
        LOG_DEBUG(req.Debug().Logger()) << "Scenario state: " << JsonStringFromProto(state.ScenarioState);
    } else {
        LOG_DEBUG(req.Debug().Logger()) << "No scenario state";
    }

    const auto* resources = dynamic_cast<const NHollywood::NMusic::TMusicResources*>(GetResources());
    Y_ENSURE(resources);

    LogBlackBoxUserInfo(req.Debug().Logger(), req.GetDataSource(NAlice::EDataSourceType::BLACK_BOX));

    // try to handle centaur frames if present
    if (req.Input().HasSemanticFrame(NHollywood::NMusic::CENTAUR_COLLECT_MAIN_SCREEN_FRAME)) {
        return TReturnValueScene<TMusicScenarioSceneCentaur>(TMusicScenarioSceneArgsCentaur{});
    }

    // elari watches don't support music scenario
    if (req.Client().GetClientInfo().IsElariWatch()) {
        return TReturnValueScene<TMusicScenarioSceneElariWatch>(
            TMusicScenarioSceneArgsElariWatch{},
            "personal_assistant.scenarios.common.prohibition_error"
        );
    }

    // try to handle start multiroom frame if present
    if (TStartMultiroomSemanticFrame frame; req.Input().FindTSF(frame)) {
        if (const auto sceneArgs = TryCreateMultiroomRedirectSceneArgs(requestData, frame, /* addPlayerFeature = */ true,
                                                                       /* onlyToMasterDevice = */ true))
        {
            return TReturnValueScene<TMusicScenarioSceneMultiroomRedirect>(*sceneArgs);
        }
        if (CanProcessStartMultiroom(req)) {
            return TReturnValueScene<TMusicScenarioSceneStartMultiroom>(BuildStartMultiroomSceneArgs(frame));
        } else {
            return TReturnValueRenderIrrelevant("music_play", "render_error__notsupported");
        }
    }

    // try to handle command frames if present
    if (auto retScene = DispatchPlayerCommand(requestData, state)) {
        // don't process thick client requests now (we won't win post-predict anyway)
        if (!IsPlayerCommandRelevant(req)) {
            return TReturnValueRenderIrrelevant("music_play", "render_error__notsupported");
        }
        return std::move(*retScene);
    }

    // try to handle equalizer frames if present
    if (req.Input().HasSemanticFrame(NHollywood::NMusic::GET_EQUALIZER_SETTINGS_FRAME)) {
        return TReturnValueScene<TMusicScenarioSceneEqualizer>(TMusicScenarioSceneArgsEqualizer{});
    }

    // try to handle music_play_less frames if present
    if (req.Input().HasSemanticFrame(NHollywood::NMusic::MUSIC_PLAY_LESS_FRAME) &&
        !req.Flags().IsExperimentEnabled(NExperiments::EXP_HOLLYWOOD_NO_MUSIC_PLAY_LESS))
    {
        return TReturnValueScene<TMusicScenarioScenePlayLess>(TMusicScenarioSceneArgsPlayLess{});
    }

    // response with stub if we ask for music and the device is a tandem follower
    if (req.Input().HasSemanticFrame(NHollywood::NMusic::MUSIC_PLAY_FRAME) &&
        NHollywood::TEnvironmentStateHelper{req}.IsDeviceTandemFollower() &&
        !req.Flags().IsExperimentEnabled(NExperiments::EXP_HW_MUSIC_DISABLE_TANDEM_FOLLOWER_BAN))
    {
        return TReturnValueScene<TMusicScenarioSceneTandemFollower>(
            TMusicScenarioSceneArgsTandemFollower{},
            "personal_assistant.scenarios.common.prohibition_error"
        );
    }

    // try to handle fm radio frames if present
    const auto& fmRadioResources = resources->GetFmRadioResources();
    if (const auto args = NFmRadio::TryBuildSceneArgs(req, fmRadioResources, state.MusicQueue)) {
        return TReturnValueScene<NFmRadio::TMusicScenarioSceneFmRadio>(*args);
    }

    // we may have to redirect music_play frame to another device
    if (TMusicPlaySemanticFrame frame; req.Input().FindTSF(frame)) {
        if (const auto sceneArgs = TryCreateMultiroomRedirectSceneArgs(requestData, frame)) {
            return TReturnValueScene<TMusicScenarioSceneMultiroomRedirect>(*sceneArgs);
        }
    }

    // fallback to old run-prepare
    return TReturnValueDo{};
}

TMaybe<TRetScene> TMusicScenario::DispatchPlayerCommand(TScenarioRequestData& requestData, TScenarioStateData& state) const {
    const auto& req = *static_cast<const TRunRequest*>(&requestData.Request);

    // process commands only for thin client
    const bool isThinClient = req.Flags().IsExperimentEnabled(NExperiments::EXP_HW_MUSIC_THIN_CLIENT) ||
                              req.Flags().IsExperimentEnabled(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_GENERATIVE) ||
                              req.Flags().IsExperimentEnabled(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_FM_RADIO);
    if (!req.Client().GetInterfaces().GetHasAudioClient() || !isThinClient ||
        !NHollywood::NMusic::IsAudioPlayerVsMusicAndBluetoothTheLatest(requestData.GetProto<TDeviceState>()))
    {
        return Nothing();
    }

    // what_is_playing
    if (req.Input().HasSemanticFrame(NAlice::NMusic::PLAYER_WHAT_IS_PLAYING)) {
        static const TMusicScenarioSceneArgsPlayerCommandWhatIsPlaying sceneArgs;
        return TReturnValueScene<TMusicScenarioScenePlayerCommandWhatIsPlaying>(sceneArgs);
    }

    // send_song_text
    if (req.Input().HasSemanticFrame(NAlice::NMusic::PLAYER_SEND_SONG_TEXT)
        && req.Flags().IsExperimentEnabled(NHollywood::EXP_HW_MUSIC_SEND_SONG_TEXT))
    {
        static const TMusicScenarioSceneArgsPlayerCommandSendSongText sceneArgs;
        return TReturnValueScene<TMusicScenarioScenePlayerCommandSendSongText>(sceneArgs);
    }

    // what_year_is_this_song
    if (req.Input().HasSemanticFrame(NAlice::NMusic::PLAYER_WHAT_YEAR_IS_THIS_SONG)
        && req.Flags().IsExperimentEnabled(NHollywood::EXP_HW_MUSIC_WHAT_YEAR_IS_THIS_SONG))
    {
        static const TMusicScenarioSceneArgsPlayerCommandWhatYearIsThisSong sceneArgs;
        return TReturnValueScene<TMusicScenarioScenePlayerCommandWhatYearIsThisSong>(sceneArgs);
    }

    // what_album_is_this_song_from
    if (req.Input().HasSemanticFrame(NAlice::NMusic::PLAYER_WHAT_ALBUM_IS_THIS_SONG_FROM)
        && req.Flags().IsExperimentEnabled(NHollywood::EXP_HW_MUSIC_WHAT_ALBUM_IS_THIS_SONG_FROM))
    {
        static const TMusicScenarioSceneArgsPlayerCommandWhatAlbumIsThisSongFrom sceneArgs;
        return TReturnValueScene<TMusicScenarioScenePlayerCommandWhatAlbumIsThisSongFrom>(sceneArgs);
    }

    // what_is_this_song_about
    if (req.Input().HasSemanticFrame(NAlice::NMusic::PLAYER_WHAT_IS_THIS_SONG_ABOUT)
        && req.Flags().IsExperimentEnabled(NHollywood::EXP_HW_MUSIC_WHAT_IS_THIS_SONG_ABOUT))
    {
        static const TMusicScenarioSceneArgsPlayerCommandWhatIsThisSongAbout sceneArgs;
        return TReturnValueScene<TMusicScenarioScenePlayerCommandWhatIsThisSongAbout>(sceneArgs);
    }

    // songs_by_this_artist
    if (req.Input().HasSemanticFrame(NAlice::NMusic::PLAYER_SONGS_BY_THIS_ARTIST)
        && req.Flags().IsExperimentEnabled(NHollywood::EXP_HW_MUSIC_SONGS_BY_THIS_ARTIST))
    {
        static const TMusicScenarioSceneArgsPlayerCommandSongsByThisArtist sceneArgs;
        return TReturnValueScene<TMusicScenarioScenePlayerCommandSongsByThisArtist>(sceneArgs);
    }

    // remove_like
    if (TPlayerRemoveLikeSemanticFrame frame; req.Input().FindTSF(frame)) {
        TMusicScenarioSceneArgsPlayerCommandRemoveLike sceneArgs;
        FillCommonArgs(*sceneArgs.MutableCommonArgs(), req, frame);
        return TReturnValueScene<TMusicScenarioScenePlayerCommandRemoveLike>(sceneArgs);
    }

    // remove_dislike
    if (TPlayerRemoveDislikeSemanticFrame frame; req.Input().FindTSF(frame)) {
        TMusicScenarioSceneArgsPlayerCommandRemoveDislike sceneArgs;
        FillCommonArgs(*sceneArgs.MutableCommonArgs(), req, frame);
        return TReturnValueScene<TMusicScenarioScenePlayerCommandRemoveDislike>(sceneArgs);
    }

    // other commands cannot be processed if scenarios state is empty
    // TODO(sparkle): fallback to radio request after porting it to HwFramework:
    // https://a.yandex-team.ru/svn/trunk/arcadia/alice/hollywood/library/scenarios/music/audio_player_commands.cpp?rev=r9615604#L857-880
    const auto& contentId = state.ScenarioState.GetQueue().GetPlaybackContext().GetContentId();
    const bool shouldFallback = contentId.GetId().empty() && contentId.GetIds().empty();

    // change_track_version (NOT PORTED YET)
    if (req.Input().HasSemanticFrame(NAlice::NMusic::MUSIC_PLAYER_CHANGE_TRACK_VERSION)
        && req.Flags().IsExperimentEnabled(NHollywood::EXP_HW_MUSIC_SEND_SONG_TEXT))
    {
        return Nothing();
    }

    // next_track (NOT PORTED YET)
    if (TPlayerNextTrackSemanticFrame frame; req.Input().FindTSF(frame)) {
        if (const auto sceneArgs = TryCreateMultiroomRedirectSceneArgs(requestData, frame, /* addPlayerFeature = */ true)) {
            return TReturnValueScene<TMusicScenarioSceneMultiroomRedirect>(*sceneArgs);
        }

        TMaybe<NHollywood::NMusic::ETrackChangeResult> result;
        try {
            result = state.MusicQueue.ChangeToNextTrack();
        } catch (...) {
            // do nothing...
        }

        if (result) {
            if (const auto sceneArgs = NFmRadio::TryBuildSceneArgsFromNextTrackCommand(req, frame, state.MusicQueue, *result)) {
                return TReturnValueScene<NFmRadio::TMusicScenarioSceneFmRadio>(*sceneArgs);
            }
        }

        return Nothing();
    }

    // prev_track (NOT PORTED YET)
    if (TPlayerPrevTrackSemanticFrame frame; req.Input().FindTSF(frame)) {
        if (const auto sceneArgs = TryCreateMultiroomRedirectSceneArgs(requestData, frame, /* addPlayerFeature = */ true)) {
            return TReturnValueScene<TMusicScenarioSceneMultiroomRedirect>(*sceneArgs);
        }

        TMaybe<NHollywood::NMusic::ETrackChangeResult> result;
        try {
            result = state.MusicQueue.ChangeToPrevTrack();
        } catch (...) {
            // do nothing...
        }

        if (result) {
            if (const auto sceneArgs = NFmRadio::TryBuildSceneArgsFromPrevTrackCommand(req, frame, state.MusicQueue, *result)) {
                return TReturnValueScene<NFmRadio::TMusicScenarioSceneFmRadio>(*sceneArgs);
            }
        }

        return Nothing();
    }

    // change_track_number (NOT PORTED YET)
    if (req.Input().HasSemanticFrame(NAlice::NMusic::MUSIC_PLAYER_CHANGE_TRACK_NUMBER)
        && req.Flags().IsExperimentEnabled(NHollywood::EXP_HW_MUSIC_CHANGE_TRACK_NUMBER))
    {
        return Nothing();
    }

    // continue (NOT PORTED YET)
    if (TPlayerContinueSemanticFrame frame; req.Input().FindTSF(frame)
        || req.Input().HasSemanticFrame(NAlice::NMusic::MUSIC_PLAYER_CONTINUE))
    {
        if (const auto sceneArgs = TryCreateMultiroomRedirectSceneArgs(requestData, frame, /* addPlayerFeature = */ true)) {
            return TReturnValueScene<TMusicScenarioSceneMultiroomRedirect>(*sceneArgs);
        }
        if (const auto sceneArgs = NFmRadio::TryBuildSceneArgsFromContinueCommand(req, frame, state.MusicQueue)) {
            LOG_INFO(req.Debug().Logger()) << "Handling Continue player command for FM Radio stream...";
            return TReturnValueScene<NFmRadio::TMusicScenarioSceneFmRadio>(*sceneArgs);
        }
        return Nothing();
    }

    // what_year_is_this_song (NOT PORTED YET)
    if (req.Input().HasSemanticFrame(NAlice::NMusic::PLAYER_WHAT_YEAR_IS_THIS_SONG)
        && req.Flags().IsExperimentEnabled(NHollywood::EXP_HW_MUSIC_WHAT_YEAR_IS_THIS_SONG))
    {
        return Nothing();
    }

    // like (NOT PORTED YET)
    if (TPlayerLikeSemanticFrame frame; req.Input().FindTSF(frame)) {
        if (const auto sceneArgs = TryCreateMultiroomRedirectSceneArgs(requestData, frame, /* addPlayerFeature = */ true)) {
            return TReturnValueScene<TMusicScenarioSceneMultiroomRedirect>(*sceneArgs);
        }
        return Nothing();
    }

    // dislike (NOT PORTED YET)
    if (TPlayerDislikeSemanticFrame frame; req.Input().FindTSF(frame)) {
        if (const auto sceneArgs = TryCreateMultiroomRedirectSceneArgs(requestData, frame, /* addPlayerFeature = */ true)) {
            return TReturnValueScene<TMusicScenarioSceneMultiroomRedirect>(*sceneArgs);
        }
        return Nothing();
    }

    // shuffle (NOT PORTED YET)
    if (TPlayerShuffleSemanticFrame frame; req.Input().FindTSF(frame)) {
        if (const auto sceneArgs = TryCreateMultiroomRedirectSceneArgs(requestData, frame, /* addPlayerFeature = */ true)) {
            return TReturnValueScene<TMusicScenarioSceneMultiroomRedirect>(*sceneArgs);
        }
#ifdef HW_FRAMEWORK_SHUFFLE
        if (shouldFallback) {
            return Nothing();
        }
        static const TMusicScenarioSceneArgsPlayerCommandShuffle sceneArgs;
        return TReturnValueScene<TMusicScenarioScenePlayerCommandShuffle>(sceneArgs);
#else
        return Nothing();
#endif // HW_FRAMEWORK_SHUFFLE
    }

    // unshuffle (NOT PORTED YET)
    if (TPlayerUnshuffleSemanticFrame frame; req.Input().FindTSF(frame)) {
        if (const auto sceneArgs = TryCreateMultiroomRedirectSceneArgs(requestData, frame, /* addPlayerFeature = */ true)) {
            return TReturnValueScene<TMusicScenarioSceneMultiroomRedirect>(*sceneArgs);
        }
        return Nothing();
    }

    // replay (NOT PORTED YET)
    if (TPlayerReplaySemanticFrame frame; req.Input().FindTSF(frame)) {
        if (const auto sceneArgs = TryCreateMultiroomRedirectSceneArgs(requestData, frame, /* addPlayerFeature = */ true)) {
            return TReturnValueScene<TMusicScenarioSceneMultiroomRedirect>(*sceneArgs);
        }
        return Nothing();
    }

    // rewind
    if (TPlayerRewindSemanticFrame frame; req.Input().FindTSF(frame)) {
        if (const auto sceneArgs = TryCreateMultiroomRedirectSceneArgs(requestData, frame, /* addPlayerFeature = */ true)) {
            return TReturnValueScene<TMusicScenarioSceneMultiroomRedirect>(*sceneArgs);
        } else if (shouldFallback) {
            return Nothing();
        }

        TMusicScenarioSceneArgsPlayerCommandRewind sceneArgs;

        const auto& rewindType = frame.GetRewindType();
        if (rewindType.HasRewindTypeValue()) {
            sceneArgs.SetRewindType(rewindType.GetRewindTypeValue());
        } else if (rewindType.HasStringValue()) {
            sceneArgs.SetRewindType(rewindType.GetStringValue());
        }

        const auto& time = frame.GetTime();
        if (time.HasUnitsTimeValue()) {
            sceneArgs.SetTime(time.GetUnitsTimeValue());
        } else if (time.HasStringValue()) {
            sceneArgs.SetTime(time.GetStringValue());
        }

        return TReturnValueScene<TMusicScenarioScenePlayerCommandRewind>(sceneArgs);
    }

    // repeat
    if (TPlayerRepeatSemanticFrame frame; req.Input().FindTSF(frame)) {
        if (const auto sceneArgs = TryCreateMultiroomRedirectSceneArgs(requestData, frame, /* addPlayerFeature = */ true)) {
            return TReturnValueScene<TMusicScenarioSceneMultiroomRedirect>(*sceneArgs);
        } else if (shouldFallback) {
            return Nothing();
        }

        TMusicScenarioSceneArgsPlayerCommandRepeat sceneArgs;
        FillCommonArgs(*sceneArgs.MutableCommonArgs(), req, frame);
        sceneArgs.SetRepeatMode(frame.GetMode().GetEnumValue());
        return TReturnValueScene<TMusicScenarioScenePlayerCommandRepeat>(sceneArgs);
    }

    return Nothing();
}

HW_REGISTER(TMusicScenario);

} // namespace NAlice::NHollywoodFw::NMusic
