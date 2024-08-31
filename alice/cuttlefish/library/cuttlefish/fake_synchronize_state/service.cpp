#include "service.h"

#include <alice/cuttlefish/library/cuttlefish/common/common_items.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/cuttlefish/synchronize_state/service.h>

#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/protos/wsevent.pb.h>
#include <alice/cuttlefish/library/utils/string_utils.h>
#include <util/string/builder.h>
#include <util/string/util.h>

#include <contrib/libs/protobuf/src/google/protobuf/util/json_util.h>

using namespace NAliceProtocol;
using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices;

namespace {
    const unsigned MIN_SPPECHKIT_VERSION_FOR_SYNCHRONIZE_STATE_RESPONSE = 4006000;  // 4.6.0

    bool NeedBlackBoxRequest(const TUserInfo& userInfo) {
        return userInfo.GetAuthTokenType() == TUserInfo::OAUTH;
    }

    unsigned SpeechkitVersionAsNumber(TStringBuf ver) {
        unsigned major, minor, micro;
        if (!TrySplitAndCast(ver, '.', major, minor, micro)) {
            return 0;
        }

        if (major >= 1000 || minor >= 1000 || micro >= 1000) {
            return 0;
        }

        return major * 1000000 + minor * 1000 + micro;
    }


    class TFakeRequestContext: public NSynchronizeState::TRequestContext {
    public:

        TFakeRequestContext(
            const NAliceCuttlefishConfig::TConfig& config,
            NAppHost::IServiceContext& serviceCtx,
            NAliceProtocol::TSessionContext&& sessionCtx,
            const TLogContext& logContext
        )
            : TRequestContext(config, serviceCtx, std::move(sessionCtx), logContext)
        {}
        void FakePostProcess() {
            LogEvent(NEvClass::InfoMessage(TStringBuilder() << "POSTPROCESS SessionID=" << SessionCtx.GetSessionId()));

            bool gotTvmtoolResponse = true;

            const TUserInfo& userInfo = SessionCtx.GetUserInfo();

            // check DoNotUseLogs
            TUserOptions& userOptions = *SessionCtx.MutableUserOptions();
            if (NeedBlackBoxRequest(userInfo)) {
                if (userInfo.HasPuid()) {  // BB succeeded
                    if (!userOptions.HasDoNotUseLogs()) {  // ...but DataSync failed
                        userOptions.SetDoNotUseLogs(true);
                    }
                } else {  // BB failed
                    if (!userOptions.HasDoNotUseLogs()) {
                        userOptions.SetDoNotUseLogs(true);
                    }
                }
            } else {  // there was no BB request
                if (!userOptions.HasDoNotUseLogs()) {
                    userOptions.SetDoNotUseLogs(false);
                }
            }

            // TODO: very nasty make up something better
            if (!gotTvmtoolResponse && SessionCtx.GetUserInfo().HasTvmServiceTicket()) {
                LogEvent(NEvClass::WarningMessage("TvmTool doesn't respond - reject guid"));
                SessionCtx.MutableUserInfo()->ClearGuid();
                SessionCtx.MutableUserInfo()->ClearGuidType();
            }

            if (SessionCtx.HasSpeechkitVersion()) {
                if (SpeechkitVersionAsNumber(SessionCtx.GetSpeechkitVersion()) >= MIN_SPPECHKIT_VERSION_FOR_SYNCHRONIZE_STATE_RESPONSE) {
                    ServiceCtx.AddProtobufItem(
                        CreateSynchronizeStateResponse(SessionCtx.GetSessionId(), userInfo.GetGuid(), SessionCtx.GetInitialMessageId()),
                        ITEM_TYPE_DIRECTIVE
                    );
                }
            }
        }
    };
}

void NAlice::NCuttlefish::NAppHostServices::FakeSynchronizeState(const NAliceCuttlefishConfig::TConfig& config, NAppHost::IServiceContext& serviceCtx, TLogContext logContext) {
    try {
        TFakeRequestContext requestCtx{
            config,
            serviceCtx,
            serviceCtx.GetOnlyProtobufItem<NAliceProtocol::TSessionContext>(ITEM_TYPE_SESSION_CONTEXT),
            logContext
        };
        logContext.LogEvent(NEvClass::InfoMessage(TStringBuilder() << "Input session context: " << requestCtx.SessionCtx));
        requestCtx.Preprocess();
        requestCtx.FakePostProcess();
        logContext.LogEvent(NEvClass::InfoMessage(TStringBuilder() << "Output session context: " << requestCtx.SessionCtx));
        serviceCtx.AddProtobufItem(requestCtx.SessionCtx, ITEM_TYPE_SESSION_CONTEXT);
    } catch (const NAliceProtocol::TDirective& exc) {
        logContext.LogEvent(NEvClass::WarningMessage(TStringBuilder() << "Failed with directve: " << exc));
        serviceCtx.AddProtobufItem(exc, ITEM_TYPE_DIRECTIVE);
    } catch (const std::exception& exc) {
        logContext.LogEvent(NEvClass::ErrorMessage(TStringBuilder() << "Failed with exception: " << exc.what()));
        throw;  // or do something?
    }
}
