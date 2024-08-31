#include "model_description.h"

#include <contrib/libs/protobuf/src/google/protobuf/util/json_util.h>

#include <util/generic/yexception.h>

namespace NAlice {

namespace {
    template <typename TProtoMessage>
    TProtoMessage JsonStringToProto(TStringBuf input, bool ignoreUnknownFields) {
        TProtoMessage message;
        google::protobuf::util::JsonParseOptions options;
        options.ignore_unknown_fields = ignoreUnknownFields;
        const auto status = google::protobuf::util::JsonStringToMessage(input, &message, options);
        Y_ENSURE(status.ok(), "Failed to convert provided Json to Proto: " << status.ToString());
        return message;
    }
}

TBinaryClassifierModelDescription LoadBinaryClassifierModelDescription(TStringBuf input) {
    return JsonStringToProto<TBinaryClassifierModelDescription>(input, true);
}

TBinaryClassifierModelDescription LoadBinaryClassifierModelDescription(IInputStream* input) {
    Y_ENSURE(input);
    return LoadBinaryClassifierModelDescription(input->ReadAll());
}

} // namespace NAlice
