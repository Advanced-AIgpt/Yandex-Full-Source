#pragma once

#include <alice/megamind/protos/common/frame.pb.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>

namespace NAlice {

namespace NImpl {

struct TTypedSemanticFrameInfo {
    struct TTypedSemanticFrameSlotReflectionInfo {
        explicit TTypedSemanticFrameSlotReflectionInfo(
            const google::protobuf::FieldDescriptor* fieldDescriptor = nullptr)
            : FieldDescriptor(fieldDescriptor)
        {
        }

        const google::protobuf::FieldDescriptor* FieldDescriptor;
        THashMap<TString, const google::protobuf::FieldDescriptor*> Values{};
    };

    explicit TTypedSemanticFrameInfo(const google::protobuf::FieldDescriptor* fieldDescriptor = nullptr)
        : FieldDescriptor(fieldDescriptor)
    {
    }

    const google::protobuf::FieldDescriptor* FieldDescriptor;
    THashMap<TString, TTypedSemanticFrameSlotReflectionInfo> Slots{};
};

THashMap<TString, TTypedSemanticFrameInfo> GetTypedSemanticFramesMapping();

} // namespace NImpl

TSemanticFrame MakeSemanticFrameFromTypedSemanticFrame(const TTypedSemanticFrame& typedSemanticFrame);
TMaybe<TTypedSemanticFrame> TryMakeTypedSemanticFrameFromSemanticFrame(const TSemanticFrame& semanticFrame);
void ValidateTypedSemanticFrames();

} // namespace NAlice
