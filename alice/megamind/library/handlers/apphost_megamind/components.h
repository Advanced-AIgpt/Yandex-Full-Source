#pragma once

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/apphost_request/protos/client.pb.h>
#include <alice/megamind/library/request_composite/client/client.h>
#include <alice/megamind/library/request_composite/composite.h>
#include <alice/megamind/library/request_composite/event.h>
#include <alice/megamind/library/speechkit/request_build.h>
#include <alice/megamind/library/util/status.h>

#include <alice/megamind/protos/common/device_state.pb.h>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>

namespace NAlice::NMegamind {

class TFromAppHostInitContext : public TSpeechKitInitContext {
public:
    TFromAppHostInitContext(IAppHostCtx& ahCtx, const TCgiParameters& cgi, const THttpHeaders& headers, const TString& path);

public:
    IAppHostCtx& AhCtx;
};

class TFromAppHostEventComponent final : public TEventComponent {
public:
    static TErrorOr<TFromAppHostEventComponent> Create(TFromAppHostInitContext& ahCtx);

    const TEventProto& Event() const override {
        return *EventProtoPtr_;
    }

    TEventWrapper EventWrapper() const override {
        return Event_;
    }

private:
    explicit TFromAppHostEventComponent(TEventProtoConstPtr eventProtoPtr)
        : EventProtoPtr_{eventProtoPtr}
        , Event_{IEvent::CreateEvent(*EventProtoPtr_).release()}
    {
    }

private:
    TEventProtoConstPtr EventProtoPtr_;
    TEventWrapper Event_;
};

class TFromAppHostClientComponent final : public TClientComponent {
public:
    static TErrorOr<TFromAppHostClientComponent> Create(TFromAppHostInitContext& ahCtx);

    const TString* ClientIp() const override {
        return ClientIp_.Get();
    }

    const TString* AuthToken() const override {
        return AuthToken_.Get();
    }

    const TClientFeatures& ClientFeatures() const override {
        return ClientFeatures_;
    }

    const TExpFlags& ExpFlags() const override {
        return ExpFlags_;
    }

    const TDeviceState& DeviceState() const override {
        return DeviceState_;
    }

private:
    explicit TFromAppHostClientComponent(const NMegamindAppHost::TClientItem& item);

private:
    TExpFlags ExpFlags_;
    TClientFeatures ClientFeatures_;
    TMaybe<TString> ClientIp_;
    TMaybe<TString> AuthToken_;
    TDeviceState DeviceState_;
};

using TFromAppHostSpeechKitRequestComposite =
    TRequestComposite<TRequestParts, TFromAppHostEventComponent, TFromAppHostClientComponent>;

class TFromAppHostSpeechKitRequest : public TFromAppHostSpeechKitRequestComposite {
public:
    using TPtr = TSimpleSharedPtr<TFromAppHostSpeechKitRequest>;

public:
    TFromAppHostSpeechKitRequest(TFromAppHostInitContext& initCtx, TStatus& status);
    static TErrorOr<TPtr> Create(IAppHostCtx& ahCtx);
};

} // namespace NAlice::NMegamind
