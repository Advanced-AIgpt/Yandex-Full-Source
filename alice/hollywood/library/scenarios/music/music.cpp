#include "apply_prepare_handle.h"
#include "apply_render_handle.h"
#include "commit_prepare_handle.h"
#include "commit_render_handle.h"
#include "common.h"
#include "continue_prepare_handle.h"
#include "continue_render_handle.h"
#include "continue_thin_client_render_handle.h"
#include "fast_data.h"
#include "hardcoded_prepare.h"
#include "music_catalog_prepare_handle.h"
#include "music_commit_async_render.h"
#include "run_render_handle.h"

#include <alice/hollywood/library/scenarios/music/handles/run_prepare/main.h>

#include <alice/hollywood/library/scenarios/music/music_sdk_handles/continue_playlist_setdown_handle.h>
#include <alice/hollywood/library/scenarios/music/music_sdk_handles/continue_prepare_handle.h>
#include <alice/hollywood/library/scenarios/music/music_sdk_handles/continue_render_handle.h>
#include <alice/hollywood/library/scenarios/music/music_sdk_handles/run_prepare_handle.h>
#include <alice/hollywood/library/scenarios/music/music_sdk_handles/run_search_content_post_handle.h>

#include <alice/hollywood/library/scenarios/music/music_backend_api/change_track_version_proxy_prepare.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/find_track_idx_proxy_prepare.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/get_track_url_handles.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_request_init.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/novelty_album_search_prepare.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/playlist_proxy_prepare.h>

#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/registry/secret_registry.h>

#include <alice/hollywood/library/scenarios/music/nlg/register.h>

#include <alice/hollywood/library/music/music_resources.h>

namespace NAlice::NHollywood::NMusic {

namespace {

template<const char* NameParam>
class TDummyHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return TString{NameParam};
    };
    void Do(TScenarioHandleContext&) const override {};
};

constexpr char const PromoRunPrepare[] = "promo_run_prepare";
constexpr char const PromoCommitCommonPrepare[] = "promo_commit_common_prepare";
constexpr char const PromoCommitCommonRender[] = "promo_commit_common_render";
constexpr char const PromoCommitActivationPrepare[] = "promo_commit_activation_prepare";

} // namespace

REGISTER_SCENARIO("music",
                  AddHandle<TRunPrepareHandle>()
                      // dummy handles that do nothing
                      // TODO: remove it after graph releases
                      .AddHandle<TDummyHandle<PromoRunPrepare>>()
                      .AddHandle<TDummyHandle<PromoCommitCommonPrepare>>()
                      .AddHandle<TDummyHandle<PromoCommitCommonRender>>()
                      .AddHandle<TDummyHandle<PromoCommitActivationPrepare>>()

                      // common handles
                      .AddHandle<TBassMusicRenderHandle>()
                      .AddHandle<TBassMusicContinuePrepareHandle>()
                      .AddHandle<TBassMusicContinueRenderHandle>()
                      .AddHandle<TApplyPrepareHandle>()
                      .AddHandle<TApplyRenderHandle>()
                      .AddHandle<TContentProxyPrepareHandle>()
                      .AddHandle<TDownloadInfoPrepareHandle>()
                      .AddHandle<TUrlRequestPrepareHandle>()
                      .AddHandle<TTrackFullInfoProxyPrepareHandle>()
                      .AddHandle<TTrackSearchProxyPrepareHandle>()
                      .AddHandle<TPlaylistSearchProxyPrepareHandle>()
                      .AddHandle<TNoveltyAlbumSearchProxyPrepareHandle>()
                      .AddHandle<TFindTrackIdxProxyPrepareHandle>()
                      .AddHandle<TContinueThinClientRenderHandle>()
                      .AddHandle<TCommitPrepareHandle>()
                      .AddHandle<TMusicCommitAsyncRenderHandle>()
                      .AddHandle<TCommitRenderHandle>()
                      .AddHandle<TCacheAdapterHandle>()

                      // music_sdk_run graph handles
                      .AddHandle<NMusicSdk::TMusicSdkRunPrepareHandle>()
                      .AddHandle<NMusicSdk::TMusicSdkRunSearchContentPostHandle>()

                      // music_sdk_continue graph handles
                      .AddHandle<NMusicSdk::TMusicSdkContinuePrepareHandle>()
                      .AddHandle<NMusicSdk::TMusicSdkContinuePlaylistSetdownHandle>()
                      .AddHandle<NMusicSdk::TMusicSdkContinueRenderHandle>()

                      // music_search subgraph
                      .AddHandle<TMusicCatalogPrepareHandle>()

                      // misc
                      .AddFastData<TMusicFastDataProto, TMusicFastData>("music/music_targeting.pb")
                      .AddFastData<TStationPromoFastDataProto, TStationPromoFastData>("music/station_promo.pb")
                      .AddFastData<TMusicShotsFastDataProto, TMusicShotsFastData>("music/music_shots.pb")
                      .SetResources<TMusicResources>()
                      .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NMusic::NNlg::RegisterAll));

REGISTER_SCENARIO("hardcoded_music", AddHandle<TBassMusicHardcodedPrepareHandle>()
                                     .AddHandle<TBassMusicHardcodedRenderHandle>()
                                     .AddHandle<TCommitMusicHardcodedPrepareHandle>()
                                     .AddHandle<TCommitMusicHardcodedRenderHandle>()
                                     .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NMusic::NNlg::RegisterAll));

REGISTER_SECRET(MUSIC_GUEST_OAUTH_TOKEN_AES_ENCRYPTION_KEY_SECRET, MUSIC_GUEST_OAUTH_TOKEN_AES_ENCRYPTION_KEY_SECRET_DEFAULT_VALUE);

} // namespace NAlice::NHollywood::NMusic
