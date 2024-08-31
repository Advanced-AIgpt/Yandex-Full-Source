#include "form_to_frame.h"

#include <alice/library/frame/description.h>

#include <library/cpp/testing/unittest/registar.h>

#include <search/begemot/rules/granet/proto/granet.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/vector.h>

namespace NBg {

namespace {

NProto::TGranetSlotValue MakeData(ui32 begin, ui32 end, const TString& type, const TString& value) {
    NProto::TGranetSlotValue data;
    data.SetBegin(begin);
    data.SetEnd(end);
    data.SetType(type);
    data.SetValue(value);

    return data;
}

NProto::TGranetTag MakeTag(ui32 begin, ui32 end, const TString& name, const TVector<NProto::TGranetSlotValue>& dataList) {
    NProto::TGranetTag tag;
    tag.SetBegin(begin);
    tag.SetEnd(end);
    tag.SetName(name);
    for (const auto& data : dataList) {
        *tag.AddData() = data;
    }

    return tag;
}



} // namespace

Y_UNIT_TEST_SUITE(FormToFrame) {

Y_UNIT_TEST(NoDescription) {
    NProto::TGranetForm form;
    form.SetName("some name");
    *form.AddTags() = MakeTag(2, 3, "tag1", {MakeData(2, 3, "type2", "value2")});
    *form.AddTags() = MakeTag(3, 5, "tag2", {});

    const NAlice::TSemanticFrame frame = ConvertFormToSemanticFrame(form);

    UNIT_ASSERT_VALUES_EQUAL(form.GetName(), frame.GetName());
    UNIT_ASSERT_VALUES_EQUAL(form.GetTags().size(), frame.GetSlots().size());
    UNIT_ASSERT_VALUES_EQUAL(form.GetTags(0).GetName(), frame.GetSlots(0).GetName());
    UNIT_ASSERT_VALUES_EQUAL(form.GetTags(0).GetData(0).GetType(), frame.GetSlots(0).GetType());
    UNIT_ASSERT_VALUES_EQUAL(form.GetTags(0).GetData(0).GetValue(), frame.GetSlots(0).GetValue());
    UNIT_ASSERT_VALUES_EQUAL(form.GetTags(1).GetName(), frame.GetSlots(1).GetName());
    UNIT_ASSERT_VALUES_EQUAL("", frame.GetSlots(1).GetType());
    UNIT_ASSERT_VALUES_EQUAL("", frame.GetSlots(1).GetValue());
}

Y_UNIT_TEST(WithDescription) {
    using TSlot = NAlice::TFrameDescription::TSlot;
    const NAlice::TFrameDescription description{
        TSlot{"tag1", TVector<TString>{"type1", "type2"}},
        TSlot{"tag2", TVector<TString>{"type3"}},
        TSlot{"tag3", TVector<TString>{"type4", "string"}},
        TSlot{"tag5", TVector<TString>{"type1", "type2"}, /* concatenateStrings */ false, /* keepVariants */ true},
    };

    NProto::TGranetForm form;
    form.SetName("some name");
    *form.AddTags() = MakeTag(2, 3, "tag1", {MakeData(2, 3, "type2", "value2")});
    *form.AddTags() = MakeTag(3, 5, "tag2", {});
    *form.AddTags() = MakeTag(5, 7, "tag3", {});
    *form.AddTags() = MakeTag(7, 8, "tag4", {});
    *form.AddTags() = MakeTag(7, 8, "tag5", {MakeData(7, 8, "type1", "value1"), MakeData(7, 8, "type2", "value2")});

    const NAlice::TSemanticFrame frame = ConvertFormToSemanticFrame(form, &description);

    UNIT_ASSERT_VALUES_EQUAL(form.GetName(), frame.GetName());
    UNIT_ASSERT_VALUES_EQUAL(form.GetTags().size(), frame.GetSlots().size());

    UNIT_ASSERT_VALUES_EQUAL(form.GetTags(0).GetName(), frame.GetSlots(0).GetName());
    UNIT_ASSERT_VALUES_EQUAL(form.GetTags(0).GetData(0).GetType(), frame.GetSlots(0).GetType());
    UNIT_ASSERT_VALUES_EQUAL(form.GetTags(0).GetData(0).GetValue(), frame.GetSlots(0).GetValue());

    UNIT_ASSERT_VALUES_EQUAL(form.GetTags(1).GetName(), frame.GetSlots(1).GetName());
    UNIT_ASSERT_VALUES_EQUAL("", frame.GetSlots(1).GetType());
    UNIT_ASSERT_VALUES_EQUAL("", frame.GetSlots(1).GetValue());

    UNIT_ASSERT_VALUES_EQUAL(form.GetTags(2).GetName(), frame.GetSlots(2).GetName());
    UNIT_ASSERT_VALUES_EQUAL("", frame.GetSlots(2).GetType());
    UNIT_ASSERT_VALUES_EQUAL("", frame.GetSlots(2).GetValue());

    UNIT_ASSERT_VALUES_EQUAL(form.GetTags(3).GetName(), frame.GetSlots(3).GetName());
    UNIT_ASSERT_VALUES_EQUAL("", frame.GetSlots(3).GetType());
    UNIT_ASSERT_VALUES_EQUAL("", frame.GetSlots(3).GetValue());

    UNIT_ASSERT_VALUES_EQUAL(form.GetTags(4).GetName(), frame.GetSlots(4).GetName());
    UNIT_ASSERT_VALUES_EQUAL(NAlice::SLOT_VARIANTS_TYPE, frame.GetSlots(4).GetType());
    UNIT_ASSERT_VALUES_EQUAL(R"([{"type1":"value1"},{"type2":"value2"}])", frame.GetSlots(4).GetValue());
}

}

Y_UNIT_TEST_SUITE(GetRecognizedSlot) {

Y_UNIT_TEST(GranetSlotWithoutValues) {
    const NAlice::TRecognizedSlot slot = GetRecognizedSlot(MakeTag(4, 7, "tag1", {}));
    UNIT_ASSERT_VALUES_EQUAL(4, slot.Begin);
    UNIT_ASSERT_VALUES_EQUAL(7, slot.End);
    UNIT_ASSERT_VALUES_EQUAL("tag1", slot.Name);
    UNIT_ASSERT_VALUES_EQUAL(0, slot.Variants.size());
}

Y_UNIT_TEST(GranetSlotWithValues) {
    const NAlice::TRecognizedSlot slot = GetRecognizedSlot(MakeTag(2, 10, "tag1", {
        MakeData(3, 4, "type1", "value1"),
        MakeData(5, 8, "type2", "value2"),
    }));
    UNIT_ASSERT_VALUES_EQUAL(2, slot.Begin);
    UNIT_ASSERT_VALUES_EQUAL(10, slot.End);
    UNIT_ASSERT_VALUES_EQUAL("tag1", slot.Name);
    UNIT_ASSERT_VALUES_EQUAL(2, slot.Variants.size());
    UNIT_ASSERT_VALUES_EQUAL("type1", slot.Variants[0].Type);
    UNIT_ASSERT_VALUES_EQUAL("value1", slot.Variants[0].Value);
    UNIT_ASSERT_VALUES_EQUAL("type2", slot.Variants[1].Type);
    UNIT_ASSERT_VALUES_EQUAL("value2", slot.Variants[1].Value);
}

Y_UNIT_TEST(GranetSlotWithUntypedValue) {
    const NAlice::TRecognizedSlot slot = GetRecognizedSlot(MakeTag(2, 10, "tag1", {
        MakeData(3, 4, "", "value1"),
    }));
    UNIT_ASSERT_VALUES_EQUAL(2, slot.Begin);
    UNIT_ASSERT_VALUES_EQUAL(10, slot.End);
    UNIT_ASSERT_VALUES_EQUAL("tag1", slot.Name);
    UNIT_ASSERT_VALUES_EQUAL(1, slot.Variants.size());
    UNIT_ASSERT_VALUES_EQUAL("", slot.Variants[0].Type);
    UNIT_ASSERT_VALUES_EQUAL("", slot.Variants[0].Value);
}

}

} // namespace NBg
