#include <alice/nlg/library/runtime_api/text.h>
#include <alice/nlg/library/runtime_api/text_stream.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/vector.h>

using namespace NAlice::NNlg;

namespace {

constexpr auto TEXT = TText::TFlags{} | TText::EFlag::Text;
constexpr auto VOICE = TText::TFlags{} | TText::EFlag::Voice;

using TFlaggedSpans = TVector<std::pair<TStringBuf, TText::TFlags>>;

void CheckText(const TText& text, const TFlaggedSpans& expected) {
    TFlaggedSpans actual(text.begin(), text.end());
    UNIT_ASSERT_VALUES_EQUAL(expected, actual);

    TStringBuilder total;
    for (const auto& [span, flags] : expected) {
        total << span;
    }

    UNIT_ASSERT_VALUES_EQUAL(text.GetStr(), total);
    UNIT_ASSERT_VALUES_EQUAL(text.GetBounds(), total);
}

[[maybe_unused]] void CheckView(const TText::TView& view, const TFlaggedSpans& expected) {
    TFlaggedSpans actual(view.begin(), view.end());
    UNIT_ASSERT_VALUES_EQUAL(expected, actual);

    TStringBuilder total;
    for (const auto& [span, flags] : expected) {
        total << span;
    }

    UNIT_ASSERT_VALUES_EQUAL(view.GetBounds(), total);
}

}  // namespace

Y_UNIT_TEST_SUITE(NlgText) {
    Y_UNIT_TEST(TextConstructionAssignment) {
        CheckText(TText{}, {});
        CheckText(TText{"foo"}, {{"foo", TEXT | VOICE}});
        CheckText(TText{"foo", TEXT}, {{"foo", TEXT}});
        CheckText(TText{"foo", VOICE}, {{"foo", VOICE}});

        TText text{"foo", TEXT};

        CheckText(TText{text}, {{"foo", TEXT}});
        CheckText(TText{TText{"foo", TEXT}}, {{"foo", TEXT}});

        {
            TText copy{"bar", VOICE};
            copy = text;
            UNIT_ASSERT_VALUES_EQUAL(text, copy);
            CheckText(copy, {{"foo", TEXT}});
        }

        {
            TText copy{"bar", VOICE};
            copy = std::move(text);
            CheckText(copy, {{"foo", TEXT}});
        }
    }

    Y_UNIT_TEST(MoveStr) {
        TText text{"foo"};
        TString str = std::move(text).MoveStr();
        UNIT_ASSERT_VALUES_EQUAL(str, "foo");
        UNIT_ASSERT_VALUES_EQUAL(text.GetStr(), "");
    }

    Y_UNIT_TEST(StringAppend) {
        for (auto flag : {TEXT, VOICE, TEXT | VOICE}) {
            TText text1{"foo", flag};
            TText text2;
            text2.Append("foo", flag);

            UNIT_ASSERT_VALUES_EQUAL(text1, text2);
            CheckText(text1, {{"foo", flag}});
            CheckText(text2, {{"foo", flag}});
        }

        for (auto flag1 : {TEXT, VOICE, TEXT | VOICE}) {
            for (auto flag2 : {TEXT, VOICE, TEXT | VOICE}) {
                for (auto flag3 : {TEXT, VOICE, TEXT | VOICE}) {
                    // make sure the consecutive flags are different
                    if (flag1 == flag2 || flag2 == flag3) {
                        continue;
                    }

                    TText text{"foo", flag1};
                    text.Append("bar", flag2);
                    text.Append("", flag1);  // should be ignored
                    text.Append("baz", flag3);

                    CheckText(text, {{"foo", flag1}, {"bar", flag2}, {"baz", flag3}});
                }
            }
        }

        for (auto flag1 : {TEXT, VOICE, TEXT | VOICE}) {
            for (auto flag2 : {TEXT, VOICE, TEXT | VOICE}) {
                // make sure the consecutive flags are different
                if (flag1 == flag2) {
                    continue;
                }

                {
                    TText text{"foo", flag1};
                    text.Append("bar", flag2);
                    text.Append("", flag1);  // should be ignored
                    text.Append("baz", flag2);

                    CheckText(text, {{"foo", flag1}, {"barbaz", flag2}});
                }

                {
                    TText text{"foo", flag1};
                    text.Append("", flag1);  // should be ignored
                    text.Append("bar", flag1);
                    text.Append("baz", flag2);

                    CheckText(text, {{"foobar", flag1}, {"baz", flag2}});
                }
            }
        }
    }

    Y_UNIT_TEST(TextAppend) {
        for (auto flag1 : {TEXT, VOICE, TEXT | VOICE}) {
            for (auto flag2 : {TEXT, VOICE, TEXT | VOICE}) {
                {
                    TText text{"foo", flag1};
                    TText other{"", flag2};

                    TText result{text};
                    result.Append(other);

                    UNIT_ASSERT_VALUES_EQUAL(text, result);
                }
                {
                    TText text{"", flag1};
                    TText other{"foo", flag2};

                    TText result{text};
                    result.Append(other.GetView());

                    UNIT_ASSERT_VALUES_EQUAL(other, result);
                }
            }
        }

        for (auto flag1 : {TEXT, VOICE, TEXT | VOICE}) {
            for (auto flag2 : {TEXT, VOICE, TEXT | VOICE}) {
                for (auto flag3 : {TEXT, VOICE, TEXT | VOICE}) {
                    for (auto flag4 : {TEXT, VOICE, TEXT | VOICE}) {
                        TText text{"foo", flag1};

                        TText other;
                        other.Append("bar", flag2);
                        other.Append("baz", flag3);

                        text.Append(other, flag4);

                        TFlaggedSpans result;
                        if (flag1 == (flag2 & flag4) && (flag2 & flag4) == (flag3 & flag4)) {
                            result = {{"foobarbaz", flag1}};
                        } else if (flag1 == (flag2 & flag4)) {
                            result = {{"foobar", flag1}};
                            if (flag3 & flag4) {
                                result.push_back({"baz", flag3 & flag4});
                            }
                        } else if ((flag2 & flag4) == (flag3 & flag4)) {
                            result = {{"foo", flag1}};
                            if (flag2 & flag4) {
                                result.push_back({"barbaz", flag2 & flag4});
                            }
                        } else if (!(flag2 & flag4) && flag1 == (flag3 & flag4)) {
                            result = {{"foobaz", flag1}};
                        } else {
                            result = {{"foo", flag1}};
                            if (flag2 & flag4) {
                                result.push_back({"bar", flag2 & flag4});
                            }
                            if (flag3 & flag4) {
                                result.push_back({"baz", flag3 & flag4});
                            }
                        }

                        CheckText(text, result);
                    }
                }
            }
        }
    }

    Y_UNIT_TEST(TextClip) {
        TText text;
        text.Append("foo", TEXT);
        text.Append("1", VOICE);
        text.Append("bar", TEXT);
        text.Append("2", VOICE);

        const TStringBuf bounds = text.GetBounds();
        const size_t size = bounds.size();
        const TText::TView view{text, bounds};

        auto checkClip = [&text, &view](const TStringBuf bounds, const TFlaggedSpans& expected) {
            CheckText(TText{text, bounds}, expected);
            CheckText(TText{view, bounds}, expected);
            CheckView(TText::TView{text, bounds}, expected);
            CheckView(TText::TView{view, bounds}, expected);
        };

        checkClip(TStringBuf{}, {});
        checkClip(bounds, {{"foo", TEXT}, {"1", VOICE}, {"bar", TEXT}, {"2", VOICE}});
        checkClip(bounds.substr(0, size - 1), {{"foo", TEXT}, {"1", VOICE}, {"bar", TEXT}});
        checkClip(bounds.substr(1, size - 1), {{"oo", TEXT}, {"1", VOICE}, {"bar", TEXT}, {"2", VOICE}});
        checkClip(bounds.substr(2, size - 2), {{"o", TEXT}, {"1", VOICE}, {"bar", TEXT}, {"2", VOICE}});
        checkClip(bounds.substr(2, size - 3), {{"o", TEXT}, {"1", VOICE}, {"bar", TEXT}});
        checkClip(bounds.substr(2, size - 4), {{"o", TEXT}, {"1", VOICE}, {"ba", TEXT}});
        checkClip(bounds.substr(3, size - 5), {{"1", VOICE}, {"ba", TEXT}});
        checkClip(bounds.substr(3, size - 6), {{"1", VOICE}, {"b", TEXT}});
        checkClip(bounds.substr(4, size - 7), {{"b", TEXT}});
        checkClip(bounds.substr(4, size - 8), {});
    }

    Y_UNIT_TEST(ViewConstruction) {
        TText text;
        text.Append("foo", TEXT);
        text.Append("bar", VOICE);
        text.Append("baz", TEXT | VOICE);

        CheckText(text, {{"foo", TEXT}, {"bar", VOICE}, {"baz", TEXT | VOICE}});
        CheckView(text.GetView(), {{"foo", TEXT}, {"bar", VOICE}, {"baz", TEXT | VOICE}});

        TStringBuf bounds = text.GetBounds();
        CheckView(text.GetView(bounds.substr(1)), {{"oo", TEXT}, {"bar", VOICE}, {"baz", TEXT | VOICE}});
        CheckView(text.GetView(bounds.substr(2)), {{"o", TEXT}, {"bar", VOICE}, {"baz", TEXT | VOICE}});
        CheckView(text.GetView(bounds.substr(3)), {{"bar", VOICE}, {"baz", TEXT | VOICE}});
        CheckView(text.GetView(bounds.substr(3, bounds.size() - 4)), {{"bar", VOICE}, {"ba", TEXT | VOICE}});
        CheckView(text.GetView(bounds.substr(3, bounds.size() - 5)), {{"bar", VOICE}, {"b", TEXT | VOICE}});
        CheckView(text.GetView(bounds.substr(3, bounds.size() - 6)), {{"bar", VOICE}});
        CheckView(text.GetView(bounds.substr(3, bounds.size() - 7)), {{"ba", VOICE}});
        CheckView(text.GetView(bounds.substr(3, bounds.size() - 8)), {{"b", VOICE}});
        CheckView(text.GetView(bounds.substr(3, bounds.size() - 9)), {});
    }

    Y_UNIT_TEST(StreamPrimitives) {
        TText text;
        TTextOutput out(text);

        out << 3.14
            << ClearFlag(TText::EFlag::Voice) << 12 << 3
            << ClearFlag(TText::EFlag::Text) << "Hello"
            << SetFlag(TText::EFlag::Voice) << "world";

        CheckText(text, {{"3.14", TEXT | VOICE}, {"123", TEXT}, {"world", VOICE}});
    }

    Y_UNIT_TEST(StreamText) {
        TText text;
        TTextOutput out(text);

        TText other("foo");
        other.Append("bar", TEXT);

        out << ClearFlag(TText::EFlag::Voice) << other;
        CheckText(text, {{"foobar", TEXT}});
    }

    Y_UNIT_TEST(StreamFallback) {
        TString str;
        TStringOutput out(str);

        TText other("foo");
        other.Append("bar", TEXT);

        out << ClearFlag(TText::EFlag::Voice) << other;
        UNIT_ASSERT_VALUES_EQUAL(str, "foobar");
    }

    Y_UNIT_TEST(StreamStack) {
        TText text;
        TTextOutput out(text);

        out << "all"
            << ClearFlag(TText::EFlag::Voice)
            << "text"
            << PushMask
            << "text"
            << SetFlag(TText::EFlag::Voice)
            << "all"
            << PopMask
            << "text";

        CheckText(text, {{"all", TEXT | VOICE}, {"texttext", TEXT}, {"all", TEXT | VOICE}, {"text", TEXT}});
    }

    Y_UNIT_TEST(StreamStackFallback) {
        TString str;
        TStringOutput out(str);

        out << "all"
            << ClearFlag(TText::EFlag::Voice)
            << "text"
            << PushMask
            << "text"
            << SetFlag(TText::EFlag::Voice)
            << "all"
            << PopMask
            << "text";

        UNIT_ASSERT_VALUES_EQUAL(str, "alltexttextalltext");
    }
}
