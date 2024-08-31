#include "message_diff.h"

#include <alice/library/proto/proto.h>

#include <library/cpp/libgit2_wrapper/unidiff.h>

#include <util/generic/hash.h>
#include <util/stream/str.h>

namespace NAlice {

TMessageDiff::TMessageDiff(const google::protobuf::Message& expected, const google::protobuf::Message& actual) {
    google::protobuf::util::MessageDifferencer differencer;
    differencer.ReportDifferencesToString(&Diff);
    AreEqual = differencer.Compare(expected, actual);

    if (!AreEqual) {
        const TString expectedText = SerializeProtoText(expected);
        const TString actualText = SerializeProtoText(actual);

        TStringOutput out{FullDiff};
        NLibgit2::UnifiedDiff(expectedText, actualText, /* context= */ UINT_MAX,
                          out, /* colored= */ false);
    }
}

} // namespace NAlice
