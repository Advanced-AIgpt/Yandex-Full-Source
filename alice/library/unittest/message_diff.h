#pragma once

#include <google/protobuf/util/message_differencer.h>
#include <util/generic/string.h>

namespace NAlice {

struct TMessageDiff {
    bool AreEqual = false;
    TString Diff;
    TString FullDiff;

    TMessageDiff(const google::protobuf::Message& expected, const google::protobuf::Message& actual);
};

} // namespace NAlice

#if defined(UNIT_ASSERT_MESSAGES_EQUAL_C)
#error UNIT_ASSERT_MESSAGES_EQUAL_C is defined twice!
#endif

#define UNIT_ASSERT_MESSAGES_EQUAL_C(lhs, rhs, msg)                                                     \
    do {                                                                                                \
        NAlice::TMessageDiff diff((lhs), (rhs));                                                        \
        TStringBuilder failMsg;                                                                         \
        failMsg << "Message diff:\n" << diff.Diff;                                                      \
        TStringBuilder details;                                                                         \
        details << msg;                                                                                 \
        if (details) {                                                                                  \
            failMsg << "\nDetails: " << details;                                                        \
        }                                                                                               \
        UNIT_ASSERT_C(diff.AreEqual, failMsg);                                                          \
    } while (false)

#if defined(UNIT_ASSERT_MESSAGES_EQUAL)
#error UNIT_ASSERT_MESSAGES_EQUAL is defined twice!
#endif

#define UNIT_ASSERT_MESSAGES_EQUAL(lhs, rhs)                                                            \
    UNIT_ASSERT_MESSAGES_EQUAL_C(lhs, rhs, "")
