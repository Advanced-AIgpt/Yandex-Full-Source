#pragma once

#include <alice/library/censor/protos/extension.pb.h>

#include <google/protobuf/message.h>

#include <util/generic/flags.h>
#include <util/generic/hash_set.h>
#include <util/generic/iterator_range.h>

#include <numeric>

namespace NAlice {

using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::Message;

class TCensor {
public:
    Y_DECLARE_FLAGS(TFlags, EAccess);

    void ProcessMessage(TFlags mode, google::protobuf::Message& message);
    template <typename TContainer>
    static TFlags GenerateFlags(const TContainer& container) {
        return std::accumulate(container.begin(), container.end(), TFlags{}, std::bit_or{});
    }

private:
    bool ScanDescriptor(TFlags mode, const Descriptor& descriptor);
    void InitializeRequiredFields(Message& message);

    struct TFieldValue {
        const FieldDescriptor* FieldDescriptor = nullptr;
        const bool MayContainPrivateFields = false;
    };

    THashMap<std::pair<TFlags, const Descriptor*>, TVector<TFieldValue>> TraverseFields;
    THashMap<const Descriptor*, TVector<const FieldDescriptor*>> RequiredFields;
    THashSet<const Descriptor*> CensorMessages;
};

} // namespace NAlice
