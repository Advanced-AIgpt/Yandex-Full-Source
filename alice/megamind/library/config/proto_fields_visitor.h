#pragma once

#include <alice/megamind/library/config/protos/config.pb.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection.h>

#include <util/generic/vector.h>

namespace NAlice::NConfig {

struct TProtoFieldsVisitor {
    virtual ~TProtoFieldsVisitor() = default;

    virtual void operator()(NProtoBuf::Message& message,
                            const NProtoBuf::FieldDescriptor& field,
                            const TString& path) = 0;
};

class TCheckRequiredFieldsArePresent : public TProtoFieldsVisitor {
public:
    void operator()(NProtoBuf::Message& message,
                    const NProtoBuf::FieldDescriptor& field,
                    const TString& path) override;

    bool GetDefect() const;

private:
    bool Defect = false;
};

class TDfs {
public:
    TDfs() = default;

    explicit TDfs(const NProtoBuf::Message& message);

    void VisitProtoFields(NProtoBuf::Message& message, TProtoFieldsVisitor& visitor, TString& path);

private:
    size_t PrepareForVisit(const NProtoBuf::Message& message); // returns subtree's index

    void VisitImpl(NProtoBuf::Message& message, TProtoFieldsVisitor& visitor, TString& path, size_t index);

    TVector<bool> EnvExt;
    TVector<size_t> SubtreeSize;
    size_t Index = 0;
};

} // namespace NAlice::NConfig
