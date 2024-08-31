#pragma once


#include <alice/cuttlefish/library/cuttlefish/common/blackbox.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/utils.h>

#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/proto_configs/cuttlefish.cfgproto.pb.h>

#include <alice/cuttlefish/library/protos/events.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

#include <apphost/api/service/cpp/service.h>
#include <apphost/lib/proto_answers/http.pb.h>

#include <util/string/builder.h>


namespace NAlice::NCuttlefish::NAppHostServices {

namespace NSynchronizeState {

class TRequestContext {
public:
    TRequestContext(
        const NAliceCuttlefishConfig::TConfig& config,
        NAppHost::IServiceContext& serviceCtx,
        NAliceProtocol::TSessionContext&& sessionCtx,
        const TLogContext& logContext
    )
        : Config(config)
        , ServiceCtx(serviceCtx)
        , SessionCtx(sessionCtx)
        , LogContext(logContext)
    {}

    const NAliceCuttlefishConfig::TConfig& Config;
    NAppHost::IServiceContext& ServiceCtx;
    NAliceProtocol::TSessionContext SessionCtx;
    TLogContext LogContext;

    void Preprocess();
    void Postprocess();
    void BlackboxSetdown();

protected:
    template <class T>
    inline void LogEvent(const T& ev) {
        LogContext.LogEvent(ev);
    }

    void HandleBlackboxAuthorization(NAliceProtocol::TUserInfo&, const NAliceProtocol::TConnectionInfo&);
    bool HandleBlackboxOAuthResponse(const NAppHostHttp::THttpResponse&);
    void PreprocessWsMessage();
    void PreprocessFlagsDotJsonExperiments(const NAliceProtocol::TSynchronizeStateEvent& event);
    void SetDevicePlatform();
    void PreprocessAppId(const NAliceProtocol::TApplicationInfo&);
    void PreprocessApplicationInfo(const NAliceProtocol::TSynchronizeStateEvent&);
    void PreprocessAppToken(const NAliceProtocol::TSynchronizeStateEvent &event);
    NAliceProtocol::TUserInfo* PreprocessUserInfo(const NAliceProtocol::TSynchronizeStateEvent &event);
    void PreprocessDeviceInfo(const NAliceProtocol::TSynchronizeStateEvent &event);
};

template <void(TRequestContext::*Func)()>
void ServiceFunctionWrap(const NAliceCuttlefishConfig::TConfig& config, NAppHost::IServiceContext& serviceCtx, TLogContext logContext)
{
    try {
        TRequestContext requestCtx{
            config,
            serviceCtx,
            serviceCtx.GetOnlyProtobufItem<NAliceProtocol::TSessionContext>(ITEM_TYPE_SESSION_CONTEXT),
            logContext
        };
        logContext.LogEvent(NEvClass::InfoMessage(TStringBuilder() << "Input session context: " << requestCtx.SessionCtx));
        (requestCtx.*Func)();
        logContext.LogEvent(NEvClass::InfoMessage(TStringBuilder() << "Output session context: " << requestCtx.SessionCtx));
        serviceCtx.AddProtobufItem(requestCtx.SessionCtx, ITEM_TYPE_SESSION_CONTEXT);
    } catch (const NAliceProtocol::TDirective& exc) {
        logContext.LogEventErrorCombo<NEvClass::WarningMessage>(TStringBuilder() << "Failed with directve: " << exc);
        serviceCtx.AddProtobufItem(exc, ITEM_TYPE_DIRECTIVE);
    } catch (const std::exception& exc) {
        logContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Failed with exception: " << exc.what());
        throw;  // or do something?
    }
}

}  // namespace NSynchronizeState

inline void SynchronizeStatePreprocess(const NAliceCuttlefishConfig::TConfig& config, NAppHost::IServiceContext& serviceCtx, TLogContext logContext) {
    NSynchronizeState::ServiceFunctionWrap<&NSynchronizeState::TRequestContext::Preprocess>(config, serviceCtx, std::move(logContext));
}

inline void SynchronizeStatePostprocess(const NAliceCuttlefishConfig::TConfig& config, NAppHost::IServiceContext& serviceCtx, TLogContext logContext) {
    NSynchronizeState::ServiceFunctionWrap<&NSynchronizeState::TRequestContext::Postprocess>(config, serviceCtx, std::move(logContext));
}

inline void SynchronizeStateBlackboxSetdown(const NAliceCuttlefishConfig::TConfig& config, NAppHost::IServiceContext& serviceCtx, TLogContext logContext) {
    NSynchronizeState::ServiceFunctionWrap<&NSynchronizeState::TRequestContext::BlackboxSetdown>(config, serviceCtx, std::move(logContext));
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
