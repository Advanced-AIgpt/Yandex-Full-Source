#pragma once

#include "asr1_client.h"

#include <alice/cuttlefish/library/asr/base/interface.h>
#include <voicetech/library/proto_api/yaldi.pb.h>

namespace NAlice::NAsrAdapter {
    class TProtocolConvertor {
    public:
        static void Convert(const NAsr::NProtobuf::TInitRequest&, YaldiProtobuf::InitRequest&);
        static void Convert(const NAsr::NProtobuf::TAddData&, YaldiProtobuf::AddData&);
        static void Convert(const NAsr::NProtobuf::TEndOfStream&, YaldiProtobuf::AddData&);
        static void Convert(const NAsr::NProtobuf::TCloseConnection&, YaldiProtobuf::AddData&);
        static void Convert(const YaldiProtobuf::InitResponse&, NAsr::NProtobuf::TResponse&);
        static void Convert(const YaldiProtobuf::AddDataResponse&, NAsr::NProtobuf::TResponse&);
    };
}
