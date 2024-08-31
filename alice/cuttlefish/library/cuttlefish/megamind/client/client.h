#pragma once

#include "http_requester.h"

#include <alice/cuttlefish/library/cuttlefish/megamind/request/common.h>

#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/cuttlefish/config/config.h>

#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/rtlog/rtlog.h>
#include <alice/cuttlefish/library/protos/megamind.pb.h>

#include <library/cpp/neh/multiclient.h>
#include <library/cpp/threading/future/core/future.h>

#include <util/generic/list.h>
#include <util/generic/ptr.h>
#include <util/system/mutex.h>

namespace NAlice::NCuttlefish::NAppHostServices {

class TErrorWithCode: public std::exception {
public:
    TErrorWithCode(const TString& errorCode, const TString& error)
        : Error_(error)
        , Code_(errorCode)
    {
        Text_ = TStringBuilder() << "code=" << Code_ << ": " << Error_;
    }
    TErrorWithCode(const TErrorWithCode&) = default;

    const TString& Error() const noexcept {
        return Error_;
    }
    const TString& Code() const noexcept {
        return Code_;
    }
    const TString& Text() const noexcept {
        return Text_;
    }
    const char* what() const noexcept override {
        return Text_.c_str();
    }

private:
    TString Error_;
    TString Code_;
    TString Text_;
};

// Megamind asynchronous HTTP client (and maybe protobuf-client in the future)
class IMegamindClient : public TThrRefBase {
public:
    virtual ~IMegamindClient() = default;
    virtual NThreading::TFuture<NAliceProtocol::TMegamindResponse> SendRequest(
        NNeh::TMessage&& msg,
        int asrPartialNumber,
        TAtomicSharedPtr<NAlice::NCuttlefish::TRTLogActivation> rtLogChild
    ) = 0;
};

using TMegamindClientPtr = TIntrusivePtr<IMegamindClient>;


class TMegamindClient : public IMegamindClient {
public:
    template <typename ...ArgsTs>
    static TMegamindClientPtr Create(ArgsTs&& ...args) {
        return new TMegamindClient(std::forward<ArgsTs>(args)...);
    }

public:
    NThreading::TFuture<NAliceProtocol::TMegamindResponse> SendRequest(
        NNeh::TMessage&& msg,
        int asrPartialNumber,
        TAtomicSharedPtr<NAlice::NCuttlefish::TRTLogActivation> rtLogChild
    ) override;

private:
    using TPromise = ::NThreading::TPromise<NAliceProtocol::TMegamindResponse>;

    TMegamindClient(
        const ERequestPhase phase,
        const NAliceCuttlefishConfig::TConfig& config,
        const NAliceProtocol::TSessionContext& sessionCtx,
        TLogContext logContext
    );
    void ResponsesDispatcher();
    void OnEvent(NNeh::IMultiClient::TEvent& evt, TPromise& promise);

private:
    const ERequestPhase Phase;
    const NAliceCuttlefishConfig::TConfig& Config;
    TSourceMetrics Metrics;
    TLogContext LogContext;
};

}  // namespace NAlice::NCuttlefish::NAppHostServices
