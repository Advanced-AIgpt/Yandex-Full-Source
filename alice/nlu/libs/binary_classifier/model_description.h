#pragma once

#include <alice/nlu/libs/binary_classifier/proto/model_description.pb.h>

#include <util/generic/string.h>
#include <util/stream/input.h>

namespace NAlice {

TBinaryClassifierModelDescription LoadBinaryClassifierModelDescription(TStringBuf input);
TBinaryClassifierModelDescription LoadBinaryClassifierModelDescription(IInputStream* input);

} // namespace NAlice
