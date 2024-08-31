#pragma once

#include <alice/cuttlefish/library/yabio/base/protobuf.h>

#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

#include <voicetech/library/messages/message.h>

namespace NAlice::NCuttlefish::NAppHostServices {
    TString GetGroupId(const NVoicetech::NUniproxy2::TMessage&, const TString& groupId);
    void MessageToYabioInitRequestClassify(
        const NVoicetech::NUniproxy2::TMessage&,
        NAlice::NYabio::NProtobuf::TInitRequest&,
        const NAliceProtocol::TSessionContext&,
        const NAliceProtocol::TRequestContext&,
        TLogContext* logContext = nullptr
    );
    void MessageToYabioInitRequestScore(
        const NVoicetech::NUniproxy2::TMessage&,
        NAlice::NYabio::NProtobuf::TInitRequest&,
        const NAliceProtocol::TSessionContext&,
        const NAliceProtocol::TRequestContext&,
        TLogContext* logContext = nullptr
    );
    void BiometryAddDataResponseToJson(
        const NAlice::NYabio::NProtobuf::TAddDataResponse&,
        NJson::TJsonValue& payload,
        NAlice::NYabio::NProtobuf::EMethod
    );
}
