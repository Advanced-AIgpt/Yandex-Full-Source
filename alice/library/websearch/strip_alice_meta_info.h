#pragma once

#include <alice/megamind/library/search/protos/alice_meta_info.pb.h>
#include <alice/megamind/protos/common/events.pb.h>

namespace NAlice {

void FillCompressedAsr(TAliceMetaInfo::TCompressedAsr& asr, const google::protobuf::RepeatedPtrField<TAsrResult>& asrResults);
TString DecodeCompressedAsr(const TAliceMetaInfo::TCompressedAsr& asr, size_t index);

} // namespace NAlice
