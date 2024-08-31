#pragma once
#include <alice/cuttlefish/library/protos/megamind.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <util/datetime/base.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/system/mutex.h>
#include <voicetech/asr/engine/proto_api/response.pb.h>

namespace NAlice::NCuttlefish::NAppHostServices {
    struct TPartialNumbers {
        TMaybe<int> AsrPartialNumber = Nothing();
        TMaybe<int> BioScoringPartialNumber = Nothing();
        TMaybe<int> BioClassificationPartialNumber = Nothing();
    };

    struct TSubrequest : public TThrRefBase {
        TSubrequest()
            : SetraceLabel{"megamind/http"}
            , Start{TInstant::Now()}
        {}

        NAlice::TSpeechKitRequestProto Request;
        TString SetraceLabel;
        TInstant ReceiveAsrResult;
        TMaybe<double> MaxBiometryScore;
        TInstant Start;
        TMutex Mutex;
        bool Postponed{false}; // can not use request right after receiving asr_result
        bool Final{false}; // can be updated on the fly (if got EOU same as partial)
        TMaybe<TString> Error;
        TString ErrorCode;
        THolder<NAliceProtocol::TMegamindResponse> Response;  // updated from another(not apphost) thread (http_client)
        TInstant Finish;
        TMaybe<AsrEngineResponseProtobuf::TAddDataResponse> SpareAddDataResponse;  // for case when MM return trash result we need re-run request
        TPartialNumbers PartialNumbers;  // numbers of partials from streaming backends (asr, bio) used with concrete VinsRequest
    };
    typedef TIntrusivePtr<TSubrequest> TSubrequestPtr;
}
