#pragma once

#include "common.h"

#include <alice/cuttlefish/library/cuttlefish/config/config.h>

#include <alice/cuttlefish/library/cuttlefish/megamind/speaker/service.h>

#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/rtlog/rtlog.h>

#include <alice/cuttlefish/library/protos/context_load.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

#include <alice/megamind/protos/speechkit/request.pb.h>

#include <alice/protos/api/meta/backend.pb.h>

#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/neh/neh.h>

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>


namespace NAlice::NCuttlefish::NAppHostServices {

// Megamind HTTP request builder
class IMegamindRequestBuilder {
public:
    virtual ~IMegamindRequestBuilder() = default;
    virtual NNeh::TMessage Build(const NAlice::TSpeechKitRequestProto& skrProto, NAlice::NCuttlefish::TRTLogActivation&, NJson::TJsonValue& sessionLog, bool isFinal) const = 0;
};

using TMegamindRequestBuilderPtr = std::unique_ptr<IMegamindRequestBuilder>;

class TMegamindRequestBuilder : public IMegamindRequestBuilder {
public:
    TMegamindRequestBuilder(
        const ERequestPhase requestPhase,
        const NAliceCuttlefishConfig::TConfig& config,
        TMaybe<NJson::TJsonValue> appHostParams,
        const NAliceProtocol::TSessionContext& sessionCtx,
        const NAliceProtocol::TRequestContext& requestCtx,
        const NAliceProtocol::TContextLoadResponse& contextLoadResponse,
        const IActiveSpeakerService& speakerService,
        const NAlice::TLoggerOptions& aliceLoggerOptions,
        TLogContext logContext
    );
    NNeh::TMessage Build(const NAlice::TSpeechKitRequestProto& skrProto, NAlice::NCuttlefish::TRTLogActivation&, NJson::TJsonValue& sessionLog, bool isFinal) const override;

protected: /* methods */
    bool ShouldSendProtobufContent() const;
    TString BuildVinsUrl() const;
    TString BuildContent(const NAlice::TSpeechKitRequestProto& skrProto, NJson::TJsonValue& jRequestBody, bool legacyJson=false) const;
    TString BuildHeaders(const NAlice::TSpeechKitRequestProto& skrProto, NAlice::NCuttlefish::TRTLogActivation&, TStringBuf targetUrl, bool isFinal, bool legacyJson=false) const;

private:
    const ERequestPhase RequestPhase;
    const NAliceCuttlefishConfig::TConfig& Config;
    const TMaybe<NJson::TJsonValue> AppHostParams;
    const NAliceProtocol::TSessionContext& SessionCtx;
    const NAliceProtocol::TRequestContext& RequestCtx;
    const NAliceProtocol::TContextLoadResponse& ContextLoadResponse;
    const IActiveSpeakerService& SpeakerService;
    const NAlice::TLoggerOptions& AliceLoggerOptions;

    TLogContext LogContext;
};

void ParseContactsResponseJson(const NAppHostHttp::THttpResponse&, NAlice::TSpeechKitRequestProto::TContacts&, const TLogContext&);
}  // namespace NAlice::NCuttlefish::NAppHostServices
