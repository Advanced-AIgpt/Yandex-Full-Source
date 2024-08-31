#include "protobuf.h"

#include <util/system/hostname.h>

using namespace NAlice::NYabio;
using namespace NAlice::NYabio::NProtobuf;

void NProtobuf::FillRequiredDefaults(TInitRequest& initRequest) {
    {
        TString hostname;
        try {
            hostname = FQDNHostName();
        } catch (...) {
            hostname = "unknown";
        }
        initRequest.SethostName(hostname);
    }
    initRequest.SetsessionId("");
    initRequest.Setuuid("");
}

void NProtobuf::FillRequiredDefaults(TInitResponse& initResponse) {
    {
        TString hostname;
        try {
            hostname = FQDNHostName();
        } catch (...) {
            hostname = "unknown";
        }
        initResponse.set_hostname(hostname);
    }
}

void NProtobuf::FillRequiredDefaults(TAddDataResponse& addDataResponse) {
    addDataResponse.SetresponseCode(RESPONSE_CODE_OK);
}
