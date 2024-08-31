#pragma once

#include <apphost/api/service/cpp/service_loop.h>

#include <alice/gproxy/library/gsetup/subsystem_logging.h>
#include <alice/gproxy/library/protos/metadata.pb.h>
#include <alice/gproxy/library/protos/gsetup.pb.h>
#include <alice/gproxy/library/events/gproxy.ev.pb.h>


namespace NGProxy {

template <typename T>
class TBasicHandler {
public:
    TBasicHandler(TLoggingSubsystem& logging)
        : Logging_(logging)
    { }


    void Integrate(uint16_t port, NAppHost::TLoop& loop) {
        loop.Add(port, T::Path, [this](NAppHost::IServiceContext& ctx) {
            auto logContext = Logging_.CreateLogContext(ctx);
            static_cast<T*>(this)->Handle(ctx, logContext);
        });
    }


    void AddError(NAppHost::IServiceContext& ctx, const TString& method, const TString& component, const TString& message, TLogContext& logContext) {
        NGProxy::GSetupError error;
        error.SetMethod(method);
        error.SetComponent(component);
        error.SetMessage(message);
        ctx.AddProtobufItem(error, "error");

        logContext.LogEventInfoCombo<NEvClass::GSetupError>(message);
    }

protected:
    void LogRequestInfo(const NGProxy::GSetupRequestInfo& info, const NGProxy::TMetadata& meta, const TString& component, TLogContext& logContext) {
        logContext.LogEventInfoCombo<NEvClass::GSetupRequestInfo>(meta.GetSessionId(), meta.GetRequestId(), meta.GetUuid(), info.GetMethod(), component);
    }

private:
    TLoggingSubsystem& Logging_;
};

}   // namespace NGProxy
