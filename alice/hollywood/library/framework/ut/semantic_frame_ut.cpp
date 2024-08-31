#include "framework_ut.h"

#include <alice/library/json/json.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw {

namespace {

const TStringBuf JSON_TEST = "{\"name\":\"testframe\",\"slots\":["
                                 "{\"name\":\"int32value\",\"type\":\"int32\",\"value\":\"7582\",\"accepted_types\":[\"int32\",\"int64\"]},"
                                 "{\"name\":\"uint32value\",\"type\":\"uint32\",\"value\":\"65524\",\"accepted_types\":[\"uint32\"]},"
                                 "{\"name\":\"int64value\",\"type\":\"int64\",\"value\":\"1282812722\",\"accepted_types\":[]},"
                                 "{\"name\":\"uint64value\",\"type\":\"uint64\",\"value\":\"4282812722\",\"accepted_types\":[]},"
                                 "{\"name\":\"floatvalue\",\"type\":\"float\",\"value\":\"0.256273\",\"accepted_types\":[\"float\"]},"
                                 "{\"name\":\"stringvalue\",\"type\":\"string\",\"value\":\"thisisastring\",\"accepted_types\":[\"sys.string\"]}"
                             "]}";

const TStringBuf JSON_TESTARR = "{\"name\":\"testframe\",\"slots\":["
                                    "{\"name\":\"int32value\",\"type\":\"int32\",\"value\":\"-7582\",\"accepted_types\":[\"int32\",\"int64\"]},"
                                    "{\"name\":\"int32value\",\"type\":\"int32\",\"value\":\"7790\",\"accepted_types\":[\"int32\",\"int64\"]},"
                                    "{\"name\":\"int32value\",\"type\":\"int32\",\"value\":\"0\",\"accepted_types\":[\"int32\",\"int64\"]},"
                                    "{\"name\":\"uint32value\",\"type\":\"uint32\",\"value\":\"12\",\"accepted_types\":[\"uint32\"]},"
                                    "{\"name\":\"uint32value\",\"type\":\"uint32\",\"value\":\"256\",\"accepted_types\":[\"uint32\"]},"
                                    "{\"name\":\"uint32value\",\"type\":\"uint32\",\"value\":\"abc\",\"accepted_types\":[\"uint32\"]}," // Illegal for uint32
                                    "{\"name\":\"stringvalue\",\"type\":\"string\",\"value\":\"thisisastring1\",\"accepted_types\":[\"sys.string\"]},"
                                    "{\"name\":\"stringvalue\",\"type\":\"string\",\"value\":\"thisisastring2\",\"accepted_types\":[\"sys.string\"]}"
                                "]}";

const TStringBuf JSON_TESTENUM = "{\"name\":\"testframe\",\"slots\":["
                                     "{\"name\":\"enumvalue\",\"type\":\"custom\",\"value\":\"value3\",\"accepted_types\":[\"custom\"]}"
                                 "]}";


struct TFrameTest1 : public TFrame {
    TFrameTest1(const NAlice::TSemanticFrame* proto)
        : TFrame(proto)
        , Int32Value(this, "int32value", 0)
        , UInt32Value(this, "uint32value", 666U)
        , Int64Value(this, "int64value", 2L)
        , UInt64Value(this, "uint64value", 6348254LU)
        , FloatValue(this, "floatvalue", -1.f)
        , StringValue(this, "stringvalue", "strx")
        , DefInt32(this, "notexist1", 123)
        , DefString(this, "notexist2", "xxx")
    {
    }
    TSlot<i32> Int32Value;
    TSlot<ui32> UInt32Value;
    TSlot<i64> Int64Value;
    TSlot<ui64> UInt64Value;
    TSlot<float> FloatValue;
    TSlot<TString> StringValue;
    TSlot<i32> DefInt32;
    TSlot<TString> DefString;
};

struct TFrameTest2 : public TFrame {
    TFrameTest2(const NAlice::TSemanticFrame* proto)
        : TFrame(proto)
        , Int32Value(this, "int32value")
        , UInt32Value(this, "uint32value")
        , Int64Value(this, "int64value")
        , UInt64Value(this, "uint64value")
        , FloatValue(this, "floatvalue")
        , StringValue(this, "stringvalue")
        , DefInt32(this, "notexist1")
        , DefString(this, "notexist2")
    {
    }
    TOptionalSlot<i32> Int32Value;
    TOptionalSlot<ui32> UInt32Value;
    TOptionalSlot<i64> Int64Value;
    TOptionalSlot<ui64> UInt64Value;
    TOptionalSlot<float> FloatValue;
    TOptionalSlot<TString> StringValue;
    TOptionalSlot<i32> DefInt32;
    TOptionalSlot<TString> DefString;
};

struct TFrameTest3 : public TFrame {
    TFrameTest3(const NAlice::TSemanticFrame* proto)
        : TFrame(proto)
        , Int32Value(this, "int32value")
        , UInt32Value(this, "uint32value")
        , Int64Value(this, "int64value")
        , UInt64Value(this, "uint64value")
        , FloatValue(this, "floatvalue")
        , StringValue(this, "stringvalue")
        , DefInt32(this, "notexist1")
        , DefString(this, "notexist2")
    {
    }
    TArraySlot<i32> Int32Value;
    TArraySlot<ui32> UInt32Value;
    TArraySlot<i64> Int64Value;
    TArraySlot<ui64> UInt64Value;
    TArraySlot<float> FloatValue;
    TArraySlot<TString> StringValue;
    TArraySlot<i32> DefInt32;
    TArraySlot<TString> DefString;
};

struct TFrameTest4 : public TFrame {
    enum class ETestEnum {
        Value1,
        Value2,
        Value3,
    };
    TFrameTest4(const NAlice::TSemanticFrame* proto, const TMap<TStringBuf, ETestEnum>& mapper)
        : TFrame(proto)
        , EnumValue(this, "enumvalue", mapper)
    {
    }
    TEnumSlot<ETestEnum> EnumValue;
};

}

Y_UNIT_TEST_SUITE(SemanticFrameTest) {

    Y_UNIT_TEST(RequiredSlots) {
        NAlice::TSemanticFrame sf = JsonToProto<NAlice::TSemanticFrame>(JsonFromString(JSON_TEST));
        TFrameTest1 testframe1(&sf);

        UNIT_ASSERT_EQUAL(testframe1.GetName(), "testframe");
        UNIT_ASSERT_EQUAL(testframe1.Int32Value.Value, 7582);
        UNIT_ASSERT(testframe1.Int32Value.Defined());
        UNIT_ASSERT_EQUAL(testframe1.Int32Value.GetName(), "int32value");
        UNIT_ASSERT_EQUAL(testframe1.Int32Value.GetType(), "int32");

        UNIT_ASSERT_EQUAL(testframe1.UInt32Value.Value, 65524);
        UNIT_ASSERT_EQUAL(testframe1.Int64Value.Value, 1282812722);
        UNIT_ASSERT_EQUAL(testframe1.UInt64Value.Value, 4282812722);
        UNIT_ASSERT_EQUAL(testframe1.FloatValue.Value, 0.256273f);
        UNIT_ASSERT_EQUAL(testframe1.StringValue.Value, "thisisastring");
        UNIT_ASSERT_EQUAL(testframe1.DefInt32.Value, 123);
        UNIT_ASSERT(!testframe1.DefInt32.Defined());
        UNIT_ASSERT_EQUAL(testframe1.DefString.Value, "xxx");
        UNIT_ASSERT(!testframe1.DefString.Defined());
        UNIT_ASSERT_EQUAL(testframe1.DefString.GetType(), "");
    }

    Y_UNIT_TEST(OptionalsSlots) {
        NAlice::TSemanticFrame sf = JsonToProto<NAlice::TSemanticFrame>(JsonFromString(JSON_TEST));
        TFrameTest2 testframe2(&sf);

        UNIT_ASSERT(testframe2.Int32Value.Value.Defined());
        UNIT_ASSERT_EQUAL(*testframe2.Int32Value.Value, 7582);
        UNIT_ASSERT(testframe2.Int32Value.Defined());
        UNIT_ASSERT_EQUAL(testframe2.Int32Value.GetName(), "int32value");
        UNIT_ASSERT_EQUAL(testframe2.Int32Value.GetType(), "int32");

        UNIT_ASSERT_EQUAL(*testframe2.UInt32Value.Value, 65524);
        UNIT_ASSERT_EQUAL(*testframe2.Int64Value.Value, 1282812722);
        UNIT_ASSERT_EQUAL(*testframe2.UInt64Value.Value, 4282812722);
        UNIT_ASSERT_EQUAL(*testframe2.FloatValue.Value, 0.256273f);
        UNIT_ASSERT_EQUAL(*testframe2.StringValue.Value, "thisisastring");
        UNIT_ASSERT(!testframe2.DefInt32.Value.Defined());
        UNIT_ASSERT(!testframe2.DefInt32.Defined());
        UNIT_ASSERT(!testframe2.DefString.Value.Defined());
        UNIT_ASSERT(!testframe2.DefString.Defined());
    }

    Y_UNIT_TEST(ArraySlots) {
        NAlice::TSemanticFrame sf = JsonToProto<NAlice::TSemanticFrame>(JsonFromString(JSON_TESTARR));
        TFrameTest3 testframe3(&sf);

        UNIT_ASSERT_EQUAL(testframe3.Int32Value.Value.size(), 3);
        UNIT_ASSERT_EQUAL(testframe3.Int32Value.Value[0], -7582);
        UNIT_ASSERT_EQUAL(testframe3.Int32Value.Value[1], 7790);
        UNIT_ASSERT_EQUAL(testframe3.Int32Value.Value[2], 0);

        UNIT_ASSERT_EQUAL(testframe3.UInt32Value.Value.size(), 2); // 'abc' can not be translated as uint32
        UNIT_ASSERT_EQUAL(testframe3.UInt32Value.Value[0], 12);
        UNIT_ASSERT_EQUAL(testframe3.UInt32Value.Value[1], 256);

        UNIT_ASSERT_EQUAL(testframe3.Int64Value.Value.size(), 0);
        UNIT_ASSERT_EQUAL(testframe3.UInt64Value.Value.size(), 0);
        UNIT_ASSERT_EQUAL(testframe3.FloatValue.Value.size(), 0);

        UNIT_ASSERT_EQUAL(testframe3.StringValue.Value.size(), 2);
        UNIT_ASSERT_EQUAL(testframe3.StringValue.Value[0], "thisisastring1");
        UNIT_ASSERT_EQUAL(testframe3.StringValue.Value[1], "thisisastring2");
    }

    Y_UNIT_TEST(EnumSlots1) {
        NAlice::TSemanticFrame sf = JsonToProto<NAlice::TSemanticFrame>(JsonFromString(JSON_TESTENUM));
        {
            // Regilar test
            TMap<TStringBuf, TFrameTest4::ETestEnum> mapper = {
                {"value1", TFrameTest4::ETestEnum::Value1},
                {"value2", TFrameTest4::ETestEnum::Value2},
                {"value3", TFrameTest4::ETestEnum::Value3}
            };
            TFrameTest4 testframe4(&sf, mapper);
            UNIT_ASSERT_EQUAL(testframe4.EnumValue.Value, TFrameTest4::ETestEnum::Value3);
        }
        {
            // Test with default values
            TMap<TStringBuf, TFrameTest4::ETestEnum> mapper = {
                {"value1", TFrameTest4::ETestEnum::Value1},
                {"", TFrameTest4::ETestEnum::Value2} // "" uses as default
            };
            TFrameTest4 testframe4(&sf, mapper);
            UNIT_ASSERT_EQUAL(testframe4.EnumValue.Value, TFrameTest4::ETestEnum::Value2);
        }
    }
    Y_UNIT_TEST(EnumSlots2) {
        NAlice::TSemanticFrame sf = JsonToProto<NAlice::TSemanticFrame>(JsonFromString(JSON_TESTENUM));
        // Test without default values
        TMap<TStringBuf, TFrameTest4::ETestEnum> mapper = {
            {"value1", TFrameTest4::ETestEnum::Value1},
            {"value2", TFrameTest4::ETestEnum::Value2}
            // value3 doesn't exist and empty value absent - exception will be thrown
        };
        bool exceptionFound = false;
        try {
            TFrameTest4 testframe4(&sf, mapper);
        } catch (TFrameworkException err) {
            UNIT_ASSERT(err.GetDiags().Contains("Undefined mapping for enum slot"));
            exceptionFound = true;
        }
        UNIT_ASSERT(exceptionFound);
    }
    Y_UNIT_TEST(SpecialSlots) {
        TTestEnvironment testData("my_scenario", "ru-ru");
        testData.AddSemanticFrame("frame", JSON_TEST);

        TFrame frame(testData.CreateRunRequest(), "frame", TFrame::EFrameConstructorMode::Empty);
        UNIT_ASSERT(frame.Defined());
    }

    Y_UNIT_TEST(TTestTSF) {
        TTestEnvironment testData("my_scenario", "ru-ru");
        TMusicPlaySemanticFrame musicTsf;
        TVideoPaymentConfirmedSemanticFrame videoTsf;
        {
            auto* sf = testData.RunRequest.MutableInput()->AddSemanticFrames();
            sf->SetName("music");
            *sf->MutableTypedSemanticFrame()->MutableMusicPlaySemanticFrame() = std::move(musicTsf);
        }
        {
            auto* sf = testData.RunRequest.MutableInput()->AddSemanticFrames();
            sf->SetName("video");
        }

        const auto& runRequest = testData.CreateRunRequest();
        UNIT_ASSERT(runRequest.Input().FindTSF("music", musicTsf));
        UNIT_ASSERT(!runRequest.Input().FindTSF("video", musicTsf));
        UNIT_ASSERT(!runRequest.Input().FindTSF("music", videoTsf));
        UNIT_ASSERT(!runRequest.Input().FindTSF("video", videoTsf));
    }

    Y_UNIT_TEST(TTestTSFAuto) {
        TTestEnvironment testData("my_scenario", "ru-ru");
        TGetTimeSemanticFrame getTimeTsf;
        {
            auto* sf = testData.RunRequest.MutableInput()->AddSemanticFrames();
            sf->SetName("personal_assistant.scenarios.get_time");
            *sf->MutableTypedSemanticFrame()->MutableGetTimeSemanticFrame() = std::move(getTimeTsf);
        }
        TGetTimeSemanticFrame getTimeTsf2;

        const auto& runRequest = testData.CreateRunRequest();
        UNIT_ASSERT(runRequest.Input().FindTSF(getTimeTsf2));
    }
}

} // namespace NAlice::NHollywoodFw
