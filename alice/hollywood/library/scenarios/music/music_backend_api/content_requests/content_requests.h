#pragma once

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/scenarios/music/biometry/biometry_data.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/system/types.h>

namespace NAlice::NHollywood::NMusic {

NAppHostHttp::THttpRequest PrepareContentRequest(const TScenarioBaseRequestWrapper& request, const TMusicQueueWrapper& mq, const TMusicContext& mCtx,
                                        TAtomicSharedPtr<IRequestMetaProvider> metaProvider, TRTLogger& logger, const TBiometryData& biometryData = {},
                                        const TStringBuf requesterUserId = {}, const TMaybe<NApiPath::TApiPathRequestParams> customRequestParams = Nothing(), 
                                        const TMusicArguments_EPlayerCommand playerCommand = TMusicArguments_EPlayerCommand_None);

} // namespace NAlice::NHollywood::NMusic
