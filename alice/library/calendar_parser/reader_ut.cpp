#include "reader.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>

using namespace NCalendarParser;

namespace {

const TString RFC_SAMPLE = R"EOF(DESCRIPTION:This is a lo
 ng description
  that exists on a long line.
)EOF";

TVector<TString> ReadLines(const TString& input) {
    TStringInput stream(input);
    TVector<TString> lines;
    TReader::ReadLines(stream, lines);
    return lines;
}

Y_UNIT_TEST_SUITE(TReaderUnitTest) {
    Y_UNIT_TEST(Empty) {
        const TVector<TString> actual = ReadLines("");
        const TVector<TString> expected;
        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(OneLine) {
        const TVector<TString> actual = ReadLines("Hello, World!");
        const TVector<TString> expected = {"Hello, World!"};
        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(TwoLines) {
        const TVector<TString> actual = ReadLines("first\r\nsecond");
        const TVector<TString> expected = {"first", "second"};
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(ThreeLines) {
        const TVector<TString> actual = ReadLines("first\r\nsecond\r\nthird\r\n");
        const TVector<TString> expected = {"first", "second", "third"};
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(SimpleContinuations) {
        const TVector<TString> actual = ReadLines("This is the \r\n first line!\r\nThis is the second!\r\n\r\n");
        const TVector<TString> expected = {"This is the first line!", "This is the second!", ""};
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(MultilineContinuations) {
        const TVector<TString> actual = ReadLines("A\r\n B\r\n\tC\r\nD\r\n E\r\n F");
        const TVector<TString> expected = {"ABC", "DEF"};
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(ExampleFromRFC) {
        const TVector<TString> actual = ReadLines(RFC_SAMPLE);
        const TVector<TString> expected = {"DESCRIPTION:This is a long description that exists on a long line."};
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
}

} // namespace
