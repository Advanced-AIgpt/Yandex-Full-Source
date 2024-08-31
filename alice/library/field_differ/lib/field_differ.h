#pragma once

#include <alice/library/field_differ/protos/differ_report.pb.h>
#include <alice/library/field_differ/protos/extension.pb.h>

#include <google/protobuf/message.h>

#include <util/generic/hash_set.h>

namespace NAlice {

namespace NTestSuiteFieldDiffer {

struct TTestCaseCheckSizes;

} // namespace NTestSuiteFieldDiffer

using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::Message;

class TFieldDiffer {
public:
    TDifferReport FindDiffs(const Message& lhs, const Message& rhs);

private:
    friend struct NAlice::NTestSuiteFieldDiffer::TTestCaseCheckSizes;

    bool ScanDescriptor(const Descriptor& descriptor);
    void FindDiffsImpl(const Message& lhs, const Message& rhs, TDifferReport& differReport, const TString& path);

    struct TFieldValue {
        const FieldDescriptor* FieldDescriptor = nullptr;
        const bool MayContainImportantFields = false;
        const EImportantFieldCheck ImportantFieldCheck = EImportantFieldCheck::IFC_NOTHING;
    };

    THashMap<const Descriptor*, TVector<TFieldValue>> TraverseFields;
    THashSet<const Descriptor*> MayContainImportantFields;
};

} // namespace NAlice
