#include "typed_frames.h"

#include <alice/protos/extensions/extensions.pb.h>

#include <alice/library/json/json.h>

#include <util/generic/hash.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>

namespace NAlice {
namespace {

TVector<const google::protobuf::Message*>
CollectTypedSlots(const google::protobuf::Message& typedFrame, const google::protobuf::FieldDescriptor& field) {
    const auto& reflection = *typedFrame.GetReflection();
    TVector<const google::protobuf::Message*> typedSlots;
    if (field.is_repeated()) {
        for (int i = 0; i < reflection.FieldSize(typedFrame, &field); ++i) {
            typedSlots.push_back(&reflection.GetRepeatedMessage(typedFrame, &field, i));
        }
    } else {
        if (reflection.HasField(typedFrame, &field)) {
            typedSlots.push_back(&reflection.GetMessage(typedFrame, &field));
        }
    }
    return typedSlots;
}

void InflateTypedSlot(const google::protobuf::Message& typedSlot, TStringBuf slotName, TSemanticFrame& frame) {
    const auto& descriptor = *typedSlot.GetDescriptor();
    const auto& reflection = *typedSlot.GetReflection();
    const auto* value =
        reflection.GetOneofFieldDescriptor(typedSlot, descriptor.FindOneofByName("Value"));
    if (!value) {
        return;
    }
    auto& slot = *frame.AddSlots();
    slot.SetName(slotName.data(), slotName.size());
    slot.SetType(value->options().GetExtension(SlotType));
    slot.AddAcceptedTypes(slot.GetType());

    switch (value->cpp_type()) {
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
            slot.SetValue(reflection.GetString(typedSlot, value));
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
            slot.SetValue(ToString(reflection.GetUInt32(typedSlot, value)));
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
            slot.SetValue(ToString(reflection.GetInt32(typedSlot, value)));
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
            slot.SetValue(ToString(reflection.GetFloat(typedSlot, value)));
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
            if (const auto* enumVal = reflection.GetEnum(typedSlot, value); enumVal) {
                slot.SetValue(enumVal->name());
            }
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
            slot.SetValue(ToString(reflection.GetBool(typedSlot, value)));
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
            slot.SetValue(JsonStringFromProto(reflection.GetMessage(typedSlot, value)));
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
            slot.SetValue(ToString(reflection.GetDouble(typedSlot, value)));
            break;
        default:
            ythrow yexception() << "field of type " << value->cpp_type_name() << " is not supported";
    }
}

void InflateSemanticFrame(const google::protobuf::Message& typedFrame, TSemanticFrame& frame) {
    const auto& descriptor = *typedFrame.GetDescriptor();
    frame.SetName(descriptor.options().GetExtension(SemanticFrameName));
    for (int i = 0; i < descriptor.field_count(); ++i) {
        const auto& field = *descriptor.field(i);
        if (field.cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
            continue;
        }
        const TStringBuf slotName = field.options().GetExtension(SlotName);
        for (const auto* typedSlot : CollectTypedSlots(typedFrame, field)) {
            InflateTypedSlot(*typedSlot, slotName, frame);
        }
    }
}

} // namespace

namespace NImpl {

THashMap<TString, TTypedSemanticFrameInfo> GetTypedSemanticFramesMapping() {
    THashMap<TString, TTypedSemanticFrameInfo> mapping{};
    const auto& descriptor = *TTypedSemanticFrame::descriptor();
    for (int fieldId = 0; fieldId < descriptor.field_count(); ++fieldId) {
        const auto& frameMessageDescriptor = *descriptor.field(fieldId)->message_type();
        const auto& options = frameMessageDescriptor.options();
        if (!options.HasExtension(SemanticFrameName)) {
            continue;
        }
        TTypedSemanticFrameInfo info{descriptor.field(fieldId)};
        for (int slotFieldId = 0; slotFieldId < frameMessageDescriptor.field_count(); ++slotFieldId) {
            const auto* slotFieldDescriptor = frameMessageDescriptor.field(slotFieldId);
            const auto* slotMessageDescriptor = frameMessageDescriptor.field(slotFieldId)->message_type();
            const auto& slotOptions = slotFieldDescriptor->options();
            if (!slotOptions.HasExtension(SlotName)) {
                continue;
            }
            TTypedSemanticFrameInfo::TTypedSemanticFrameSlotReflectionInfo slotInfo{slotFieldDescriptor};
            for (int valueFieldId = 0; valueFieldId < slotMessageDescriptor->field_count(); ++valueFieldId) {
                const auto* valueFieldDescriptor = slotMessageDescriptor->field(valueFieldId);
                const auto& valueOptions = valueFieldDescriptor->options();
                if (!valueOptions.HasExtension(SlotType)) {
                    continue;
                }
                slotInfo.Values[valueOptions.GetExtension(SlotType)] = valueFieldDescriptor;
            }
            info.Slots[slotOptions.GetExtension(SlotName)] = std::move(slotInfo);
        }

        mapping[options.GetExtension(SemanticFrameName)] = std::move(info);
    }
    return mapping;
}

} // namespace NImpl

TSemanticFrame MakeSemanticFrameFromTypedSemanticFrame(const TTypedSemanticFrame& typedSemanticFrame) {
    TSemanticFrame frame{};
    frame.MutableTypedSemanticFrame()->CopyFrom(typedSemanticFrame);
    if (const auto typeCase = typedSemanticFrame.GetTypeCase(); typeCase != TTypedSemanticFrame::TYPE_NOT_SET) {
        InflateSemanticFrame(
            typedSemanticFrame.GetReflection()->GetMessage(
                typedSemanticFrame, typedSemanticFrame.GetDescriptor()->FindFieldByNumber(static_cast<int>(typeCase))),
            frame);
    }
    return frame;
}

TMaybe<TTypedSemanticFrame> TryMakeTypedSemanticFrameFromSemanticFrame(const TSemanticFrame& semanticFrame) {
    static const auto framesMapping = NImpl::GetTypedSemanticFramesMapping();
    const auto* frameFieldInfo = framesMapping.FindPtr(semanticFrame.GetName());
    if (!frameFieldInfo) {
        return Nothing();
    }
    TTypedSemanticFrame typedSemanticFrame{};
    auto* frame =
        typedSemanticFrame.GetReflection()->MutableMessage(&typedSemanticFrame, frameFieldInfo->FieldDescriptor);
    for (const auto& slot : semanticFrame.GetSlots()) {
        const auto* slotFieldInfo = frameFieldInfo->Slots.FindPtr(slot.GetName());
        if (!slotFieldInfo) {
            continue;
        }
        const auto* valueFieldDescriptor = slotFieldInfo->Values.FindPtr(slot.GetType());
        if (!valueFieldDescriptor) {
            continue;
        }

        google::protobuf::Message* frameSlot = nullptr;
        if (slotFieldInfo->FieldDescriptor->is_repeated()) {
            frameSlot = frame->GetReflection()->AddMessage(frame, slotFieldInfo->FieldDescriptor);
        } else {
            frameSlot = frame->GetReflection()->MutableMessage(frame, slotFieldInfo->FieldDescriptor);
        }
        const auto& frameSlotReflection = frameSlot->GetReflection();
        if (frameSlotReflection->HasField(*frameSlot, *valueFieldDescriptor)) {
            continue; // don't fill slot again if already filled
        }
        switch ((*valueFieldDescriptor)->cpp_type()) {
            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                frameSlotReflection->SetString(frameSlot, *valueFieldDescriptor, slot.GetValue());
                break;
            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32: {
                if (ui32 value = 0; TryFromString<ui32>(slot.GetValue(), value)) {
                    frameSlotReflection->SetUInt32(frameSlot, *valueFieldDescriptor, value);
                }
                break;
            }
            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
                if (const auto* enumValue = (*valueFieldDescriptor)->enum_type()->FindValueByName(slot.GetValue())) {
                    frameSlotReflection->SetEnum(frameSlot, *valueFieldDescriptor, enumValue);
                }
                break;
            }
            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                frameSlotReflection->SetBool(frameSlot, *valueFieldDescriptor, to_lower(slot.GetValue()) == "true");
                break;
            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: {
                if (double doubleValue = 0; TryFromString<double>(slot.GetValue(), doubleValue)) {
                    frameSlotReflection->SetUInt32(frameSlot, *valueFieldDescriptor, doubleValue);
                }
                break;
            }
            default:
                break;
        }
    }
    return std::move(typedSemanticFrame);
}

void ValidateTypedSemanticFrames() {
    const auto& descriptor = *TTypedSemanticFrame::descriptor();

    for (int fieldId = 0; fieldId < descriptor.field_count(); ++fieldId) {
        const auto& frameMessageDescriptor = *descriptor.field(fieldId)->message_type();
        const auto& options = frameMessageDescriptor.options();
        if (!options.HasExtension(SemanticFrameName)) {
            ythrow yexception() << "No option SemanticFrameName found in " << frameMessageDescriptor.name();
        }

        for (int slotFieldId = 0; slotFieldId < frameMessageDescriptor.field_count(); ++slotFieldId) {
            const auto* slotFieldDescriptor = frameMessageDescriptor.field(slotFieldId);
            const auto* slotMessageDescriptor = frameMessageDescriptor.field(slotFieldId)->message_type();
            const auto& slotOptions = slotFieldDescriptor->options();
            if (!slotOptions.HasExtension(SlotName)) {
                ythrow yexception() << "No option SlotName found in " << frameMessageDescriptor.name()
                                    << " -> " << slotFieldDescriptor->name();
            }

            const auto* oneOfSlotValueDescriptor = slotMessageDescriptor->FindOneofByName("Value");
            if (!oneOfSlotValueDescriptor) {
                ythrow yexception() << "No oneof Value field found in " << frameMessageDescriptor.name()
                                    << " -> " << slotMessageDescriptor->name();
            }

            for (int slotValueFieldId = 0; slotValueFieldId < oneOfSlotValueDescriptor->field_count(); ++slotValueFieldId) {
                const auto& slotValueDescriptorField = *oneOfSlotValueDescriptor->field(slotValueFieldId);
                if (!slotValueDescriptorField.options().HasExtension(SlotType)) {
                    ythrow yexception() << "No option SlotType found in " << frameMessageDescriptor.name() << " -> "
                                        << slotMessageDescriptor->name() << " -> " << slotValueDescriptorField.name();
                }
            }
        }
    }
}

} // namespace NAlice
