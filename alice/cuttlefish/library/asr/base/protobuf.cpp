#include "protobuf.h"

#include <util/system/hostname.h>

using namespace NAlice::NAsr;
using namespace NAlice::NAsr::NProtobuf;

void NProtobuf::FillRequiredDefaults(TInitResponse& initResponse) {
    initResponse.SetIsOk(true);
    {
        TString hostname;
        try {
            hostname = FQDNHostName();
        } catch (...) {
            hostname = "unknown";
        }
        initResponse.SetHostname(hostname);
    }
    initResponse.SetTopic("");
    initResponse.SetTopicVersion("");
    initResponse.SetServerVersion("");
}

void NProtobuf::FillRequiredDefaults(TAddDataResponse& addDataResponse) {
    addDataResponse.SetIsOk(true);
    addDataResponse.SetResponseStatus(Active);
    addDataResponse.SetValidationInvoked(false);
    addDataResponse.SetCoreDebug("");
    addDataResponse.SetMessagesCount(0);
    addDataResponse.SetDurationProcessedAudio(0);
}

void NProtobuf::FillRequiredDefaults(TAsrHypo& hypo) {
    hypo.SetTotalScore(1.);
    hypo.SetParentModel("");
}
