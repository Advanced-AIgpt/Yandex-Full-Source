#pragma once

#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/common/required_messages/required_messages.pb.h>

#include <alice/library/logger/logger.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/singleton.h>
#include <util/generic/string.h>

namespace NAlice::NMegamindApi {

class TRequestConstructor final {
public:
    class TProtocolStatus final {
    public:
        enum class EStatusCode {
            Ok = 0,
            ParseError = 1,
        };

        explicit TProtocolStatus(EStatusCode code = EStatusCode::Ok, const TString& message = Default<TString>())
            : StatusCode(code)
            , Message(message) {
        }

        [[nodiscard]] const TString& GetMessage() const {
            return Message;
        }

        [[nodiscard]] bool Ok() const {
            return StatusCode == EStatusCode::Ok;
        }

        static const TProtocolStatus& StatusOk() {
            return Default<TProtocolStatus>();
        }

    private:
        EStatusCode StatusCode;
        TString Message{};
    };

public:
    explicit TRequestConstructor(TRTLogger& logger = TRTLogger::NullLogger())
        : Logger(logger) {
    }

    TProtocolStatus PushSpeechKitJson(const NJson::TJsonValue& speechKitJson);

    TSpeechKitRequestProto MakeRequest() &&;

    static void PatchContacts(TSpeechKitRequestProto& skrProto, TRTLogger& log);

private:
    TRTLogger& Logger;
    TSpeechKitRequestProto SpeechKitRequest;

    // all possible google.protobuf.Any values should be linked inside MM while jsonToProto conversion exists
    const NAlice::NMegamind::TRequiredMessages RequiredProtoMessages;
};

} // namespace NAlice::NMegamindApi
