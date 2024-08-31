#include "proto_fields_visitor.h"

#include <alice/megamind/library/config/protos/extensions.pb.h>
#include <google/protobuf/descriptor.pb.h>

#include <library/cpp/getoptpb/getoptpb.h>

#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/system/env.h>

#include <cstdlib>

namespace NAlice::NConfig {

namespace {

struct TScopedAppendPath {
    TScopedAppendPath(TString& path, const TString& append)
          : Path(path),
            OldSize(Path.size())
    {
        Path += append;
    }

    ~TScopedAppendPath() {
        Y_ASSERT(Path.size() >= OldSize);
        Path.erase(Path.begin() + OldSize, Path.end());
    }

    TString& Path;
    size_t OldSize;
};

} // namespace

void TCheckRequiredFieldsArePresent::operator()(NProtoBuf::Message& message,
                                                const NProtoBuf::FieldDescriptor& field,
                                                const TString& path) {
    if (field.options().GetExtension(NAlice::Required) &&
        !message.GetReflection()->HasField(message, &field)) {
        Cerr << "Required field " << path.data() << " is missing" << Endl;
        Defect = true;
    }
}

bool TCheckRequiredFieldsArePresent::GetDefect() const {
    return Defect;
}

TDfs::TDfs(const NProtoBuf::Message& message) {
    PrepareForVisit(message);
}

void TDfs::VisitProtoFields(NProtoBuf::Message& message, TProtoFieldsVisitor& visitor, TString& path) {
    VisitImpl(message, visitor, path, 0);
}

size_t TDfs::PrepareForVisit(const NProtoBuf::Message& message) {
    const size_t index = Index++;
    EnvExt.push_back(false);
    SubtreeSize.push_back(1); // init current subtree size

    const auto& descriptor = *message.GetDescriptor();
    const auto& reflection = *message.GetReflection();
    const int fieldCount = descriptor.field_count();
    for (int i = 0; i < fieldCount; ++i) {
        const auto& field = *descriptor.field(i);
        bool envExt = !field.options().GetExtension(NAlice::Env).Empty();
        EnvExt[index] |= envExt;
        if (field.type() != NProtoBuf::FieldDescriptor::TYPE_MESSAGE) {
            continue;
        }
        if (field.is_repeated()) {
            const int repeatedFieldSize = reflection.FieldSize(message, &field);
            for (int j = 0; j < repeatedFieldSize; ++j) {
                const auto& submessage = reflection.GetRepeatedMessage(message, &field, j);
                const size_t indexSubmessage = PrepareForVisit(submessage);
                SubtreeSize[index] += SubtreeSize[indexSubmessage];
                EnvExt[index] |= EnvExt[indexSubmessage];
            }
        } else {
            const auto& submessage = reflection.GetMessage(message, &field);
            // avoid infinite recursion for curcular nested messages.
            if (&submessage != &message) {
                const size_t indexSubmessage = PrepareForVisit(submessage);
                EnvExt[index] |= EnvExt[indexSubmessage];
            }
        }
    }
    return index;
}

void TDfs::VisitImpl(NProtoBuf::Message& message, TProtoFieldsVisitor& visitor, TString& path, size_t index) {
    const auto& descriptor = *message.GetDescriptor();
    const auto& reflection = *message.GetReflection();

    const int fieldCount = descriptor.field_count();
    size_t delta = 1;
    for (int i = 0; i < fieldCount; ++i) {
        auto& field = *descriptor.field(i);
        const TScopedAppendPath withField(path, "." + field.name());
        visitor(message, field, path);
        if (field.type() != NProtoBuf::FieldDescriptor::TYPE_MESSAGE) {
            continue;
        }
        if (field.is_repeated()) {
            const int repeatedFieldSize = reflection.FieldSize(message, &field);
            for (int j = 0; j < repeatedFieldSize; ++j) {
                const TScopedAppendPath withIndex(path, "[" + ToString(j) + "]");
                auto& submessage = *reflection.MutableRepeatedMessage(&message, &field, j);
                VisitImpl(submessage, visitor, path, index + delta);
                delta += SubtreeSize[index + delta];
            }
        } else {
            if (reflection.HasField(message, &field) || EnvExt[index + delta]) {
                auto& submessage = *reflection.MutableMessage(&message, &field);
                VisitImpl(submessage, visitor, path, index + delta);
            }
            delta += SubtreeSize[index + delta];
        }
    }
}

} // namespace NAlice::NConfig
