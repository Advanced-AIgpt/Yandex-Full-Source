#include "frame.h"

#include <alice/library/proto/proto.h>
#include <alice/library/unittest/message_diff.h>

#include <alice/megamind/protos/common/frame.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood {

namespace {

const auto PROTO = ParseProtoText<TSemanticFrame>(R"(
    Name: "foo"
    Slots: [
        {
            Name: "bar"
            TypedValue: {
                Type: "bar_type"
                String: "bar_value"
            }
            IsFilled: true
        },
        {
            Name: "baz"
            AcceptedTypes: [
                "baz_type",
                "string"
            ]
            IsRequested: true
        }
    ]
)");

// TODO(a-square): remove legacy fields Type & Value
const auto EXPECTED_PROTO = ParseProtoText<TSemanticFrame>(R"(
    Name: "foo"
    Slots: [
        {
            Name: "bar"
            Type: "bar_type"
            Value: "bar_value"
            TypedValue: {
                Type: "bar_type"
                String: "bar_value"
            }
            AcceptedTypes: [
                "bar_type"
            ]
            IsFilled: true
        },
        {
            Name: "baz"
            AcceptedTypes: [
                "baz_type",
                "string"
            ]
            IsRequested: true
        }
    ]
)");

Y_UNIT_TEST_SUITE(Frame) {
    Y_UNIT_TEST(SlotMap) {
        TFrame frame{"foo"};

        TSlot slot{"bar", "bar_type", TSlot::TValue{"bar_value"}};
        frame.AddSlot(slot);
        const auto found = frame.FindSlot("bar");
        UNIT_ASSERT(found);
        UNIT_ASSERT_VALUES_EQUAL(slot.Name, found->Name);

        UNIT_ASSERT(!frame.FindSlot("baz"));

        frame.AddSlot(slot);
        UNIT_ASSERT_EQUAL(found, frame.FindSlot("bar"));
        const auto* barSlots = frame.FindSlots("bar");
        UNIT_ASSERT(barSlots != nullptr);
        UNIT_ASSERT_EQUAL(2, barSlots->size());
        UNIT_ASSERT_EQUAL(found, &barSlots->front());

        UNIT_ASSERT(!frame.FindSlot("baz"));

        frame.RemoveSlots("bar");
        UNIT_ASSERT(!frame.FindSlot("bar"));
        UNIT_ASSERT(!frame.FindSlot("baz"));
    }

    Y_UNIT_TEST(FromProto) {
        const TFrame frame = TFrame::FromProto(PROTO);
        UNIT_ASSERT_VALUES_EQUAL(size_t{2}, frame.Slots().GetSize());

        const auto bar = frame.FindSlot("bar");
        UNIT_ASSERT(bar);
        UNIT_ASSERT_VALUES_EQUAL("bar", bar->Name);
        UNIT_ASSERT_VALUES_EQUAL("bar_type", bar->Type);
        UNIT_ASSERT_VALUES_EQUAL("bar_value", bar->Value.AsString());
        UNIT_ASSERT(bar->IsFilled);
        UNIT_ASSERT(!bar->IsRequested);
        UNIT_ASSERT(bar->AcceptedTypes.empty());

        const auto baz = frame.FindSlot("baz");
        UNIT_ASSERT(baz);
        UNIT_ASSERT_VALUES_EQUAL("baz", baz->Name);
        // TODO(a-square): make sure these don't leak into proto
        UNIT_ASSERT_VALUES_EQUAL("", baz->Type);
        UNIT_ASSERT_VALUES_EQUAL("", baz->Value.AsString());
        UNIT_ASSERT(!baz->IsFilled);
        UNIT_ASSERT(baz->IsRequested);
        UNIT_ASSERT_VALUES_EQUAL((TVector<TString>{"baz_type", "string"}), baz->AcceptedTypes);
    }

    Y_UNIT_TEST(ToProto) {
        TFrame frame{"foo"};

        TSlot bar;
        bar.Name = "bar";
        bar.Type = "bar_type";
        bar.Value = TSlot::TValue{"bar_value"};
        bar.IsFilled = true;
        frame.AddSlot(bar);

        TSlot baz;
        baz.Name = "baz";
        baz.AcceptedTypes = {"baz_type", "string"};
        baz.IsRequested = true;
        frame.AddSlot(baz);

        UNIT_ASSERT_MESSAGES_EQUAL(EXPECTED_PROTO, frame.ToProto());
    }

    Y_UNIT_TEST(RoundTrip) {
        UNIT_ASSERT_MESSAGES_EQUAL(EXPECTED_PROTO, TFrame::FromProto(EXPECTED_PROTO).ToProto());
    }

    Y_UNIT_TEST(RoundTripMultipleSlotValues) {
        const auto proto = ParseProtoText<TSemanticFrame>(R"(
            Name: "foo"
            Slots: [
                {
                    Name: "bar"
                    TypedValue: {
                        Type: "bar_type"
                        String: "bar_value"
                    }
                    IsFilled: true
                },
                {
                    Name: "bar"
                    TypedValue: {
                        Type: "bar_type"
                        String: "bar_value2"
                    }
                    IsFilled: true
                },
                {
                    Name: "baz"
                    AcceptedTypes: [
                        "baz_type",
                        "string"
                    ]
                    IsRequested: true
                },
                {
                    Name: "baz"
                    AcceptedTypes: [
                        "baz_type2",
                        "string"
                    ]
                    IsRequested: true
                },
                {
                    Name: "baz"
                    AcceptedTypes: [
                        "baz_type3",
                        "string"
                    ]
                    IsRequested: true
                }
            ]
        )");

        const auto expectedProto = ParseProtoText<TSemanticFrame>(R"(
            Name: "foo"
            Slots: [
                {
                    Name: "bar"
                    Type: "bar_type"
                    Value: "bar_value"
                    TypedValue: {
                        Type: "bar_type"
                        String: "bar_value"
                    }
                    AcceptedTypes: [
                        "bar_type"
                    ]
                    IsFilled: true
                },
                {
                    Name: "bar"
                    Type: "bar_type"
                    Value: "bar_value2"
                    TypedValue: {
                        Type: "bar_type"
                        String: "bar_value2"
                    }
                    AcceptedTypes: [
                        "bar_type"
                    ]
                    IsFilled: true
                },
                {
                    Name: "baz"
                    AcceptedTypes: [
                        "baz_type",
                        "string"
                    ]
                    IsRequested: true
                },
                {
                    Name: "baz"
                    AcceptedTypes: [
                        "baz_type2",
                        "string"
                    ]
                    IsRequested: true
                },
                {
                    Name: "baz"
                    AcceptedTypes: [
                        "baz_type3",
                        "string"
                    ]
                    IsRequested: true
                }
            ]
        )");

        UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, TFrame::FromProto(proto).ToProto());
    }
}

} // namespace

} // namespace NAlice::NHollywood
