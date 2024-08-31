#include "asr.h"

namespace NAlice::NAsrAdapter {
    YaldiProtobuf::InitRequest GetCensoredYaldiInitRequest(const YaldiProtobuf::InitRequest& initRequest) {
        auto initRequestCopy = initRequest;
        initRequestCopy.mutable_user_info()->MutableContactBookItems()->Clear();
        return initRequestCopy;
    }
}
