#include <alice/cuttlefish/library/convert/converter.h>
#include <alice/cuttlefish/library/convert/ut/proto/my.pb.h>
#include <alice/cuttlefish/library/convert/ut/proto/my.traits.pb.h>
#include <alice/cuttlefish/library/convert/ut/test_utils.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/json/json_reader.h>


using namespace NAlice::NCuttlefish::NConvert;

Y_UNIT_TEST_SUITE(JsonToProto) {

struct THobbyHandler {
    static inline void Parse(TString key, const NJson::TJsonValue& val, TEmploee& dst) {
        dst.AddHobbies(TString(key) + "-" + val.GetStringRobust());
    }
    static inline void Parse(TStringBuf key, TRapidJsonNode& val, TEmploee& dst) {
        dst.AddHobbies(TString(key) + "-" + val.GetValue());
    }

    template <typename WriterT>
    static inline void Serialize(WriterT&& writer, const TEmploee& msg) {
        TStringBuf k, v;
        for (const TProtoStringType& it : msg.GetHobbies()) {
            TStringBuf(it).Split('-', k, v);
            writer.Key(k);
            writer.Value(FromString<unsigned>(v));
        }
    }
};

TConverter<TEmploee> CreateEmploeeConverter()
{
    static const TConverter<TPet> petConv = [](){
        TConverter<TPet> conv;
        conv.Build().SetValue<NProtoTraits::TPet::Nickname>("name");
        conv.Build().SetValue<NProtoTraits::TPet::Species>("type");
        return conv;
    }();

    TConverter<TEmploee> conv;

    auto b = conv.Build();
    b.Sub<NProtoTraits::TEmploee::Features>()
        .Sub<NProtoTraits::TFeatures::Mother>()
        .SetValue<NProtoTraits::TParent::Name>("mother/name");
    b.ForEachInArray()
        .AddNew<NProtoTraits::TEmploee::Pets>()
        .Parse<petConv>("pets/list");
    b.ForEachInMap()
        .AddNew<NProtoTraits::TEmploee::Pets>()
        .SetKey<NProtoTraits::TPet::Species>()
        .SetValue<NProtoTraits::TPet::Nickname>("best-pets-ever");
    b.ForEachInMap()
        .Custom<THobbyHandler>("pets/hobbies");

    return conv;
}

Y_UNIT_TEST(Basic) {
    const TStringBuf json = R"__({
        "mother": {
            "name": "Janine"
        },
        "pets": {
            "list": [
                {"name": "Tom", "type": "cat"},
                {"name": "Jerry", "type": "mouse"},
                {"name": "Lucy", "type": "fish"}
            ],
            "hobbies": {
                "one": 1,
                "two": 2,
                "three": 3,
                "four": 4,
                "five": 5
            }
        },
        "best-pets-ever": {
            "dog": "Robert",
            "bird": "Anette"
        }
    })__";

    const TConverter<TEmploee> conv = CreateEmploeeConverter();

    TEmploee empl1;
    conv.Parse(empl1, ReadJsonValue(json));

    TEmploee empl2;
    conv.Parse(empl2, TRapidJsonRootNode(json));

    for (const TEmploee& empl : {empl1, empl2}) {
        const NJson::TJsonValue emplAsJson = AsJsonValue(empl);
        Cerr << emplAsJson << Endl;

        UNIT_ASSERT_EQUAL(emplAsJson["Features"]["Mother"]["Name"], "Janine");
        UNIT_ASSERT_CONTAINS(emplAsJson["Pets"].GetArraySafe(), ReadJsonValue(R"__({"Nickname": "Tom", "Species": "cat"})__"));
        UNIT_ASSERT_CONTAINS(emplAsJson["Pets"].GetArraySafe(), ReadJsonValue(R"__({"Nickname": "Jerry", "Species": "mouse"})__"));
        UNIT_ASSERT_CONTAINS(emplAsJson["Pets"].GetArraySafe(), ReadJsonValue(R"__({"Nickname": "Lucy", "Species": "fish"})__"));
        UNIT_ASSERT_CONTAINS(emplAsJson["Pets"].GetArraySafe(), ReadJsonValue(R"__({"Nickname": "Robert", "Species": "dog"})__"));
        UNIT_ASSERT_CONTAINS(emplAsJson["Pets"].GetArraySafe(), ReadJsonValue(R"__({"Nickname": "Anette", "Species": "bird"})__"));
        UNIT_ASSERT_CONTAINS(emplAsJson["Hobbies"].GetArraySafe(), ReadJsonValue(R"__("one-1")__"));
        UNIT_ASSERT_CONTAINS(emplAsJson["Hobbies"].GetArraySafe(), ReadJsonValue(R"__("two-2")__"));
        UNIT_ASSERT_CONTAINS(emplAsJson["Hobbies"].GetArraySafe(), ReadJsonValue(R"__("three-3")__"));
        UNIT_ASSERT_CONTAINS(emplAsJson["Hobbies"].GetArraySafe(), ReadJsonValue(R"__("four-4")__"));
        UNIT_ASSERT_CONTAINS(emplAsJson["Hobbies"].GetArraySafe(), ReadJsonValue(R"__("five-5")__"));
    }
}


// ------------------------------------------------------------------------------------------------
TConverter<TEmploee> CreateAnotherEmployeeConverter()
{
    static const TConverter<TPet> petConv = [](){
        TConverter<TPet> conv;
        conv.Build().SetValue<NProtoTraits::TPet::Nickname>("name");
        conv.Build().SetValue<NProtoTraits::TPet::Species>("type");
        return conv;
    }();

    TConverter<TEmploee> conv;
    auto b = conv.Build();
    b.SetValue<NProtoTraits::TEmploee::Name>("employee/name");
    b.Field<NProtoTraits::TEmploee::Salary>()
        .From("employee/salary")
        .SpareFrom("employee/pre_salary");
    {
        auto f = b.Sub<NProtoTraits::TEmploee::Features>();
        f.SetValue<NProtoTraits::TFeatures::Personality>("employee/character");
        {
            auto s = f.Sub<NProtoTraits::TFeatures::Mother>();
            s.SetValue<NProtoTraits::TParent::Name>("relatives/mother_name");
            s.SetValue<NProtoTraits::TParent::Salary>("relatives/mother_salary");
        } {
            auto s = f.Sub<NProtoTraits::TFeatures::Father>();
            s.SetValue<NProtoTraits::TParent::Name>("relatives/father_name");
            s.SetValue<NProtoTraits::TParent::Salary>("relatives/father_salary");
        }
    }
    b.ForEachInArray()
        .Append<NProtoTraits::TEmploee::Hobbies>("employee/interests");
    b.ForEachInArray()
        .AddNew<NProtoTraits::TEmploee::Pets>()
        .Parse<petConv>("employee/assets/animals");

    return conv;
}

Y_UNIT_TEST(AnotherConversion) {
    const TConverter<TEmploee> converter = CreateAnotherEmployeeConverter();
    const TStringBuf json = R"__({
        "employee": {
            "name": "Gregor",
            "character": "saint",
            "pre_salary": 145,
            "interests": [
                "books",
                "theatre",
                "murders"
            ],
            "assets": {
                "animals": [
                    {"name": "Tom", "type": "cat"},
                    {"name": "Jerry", "type": "mouse"}
                ]
            }
        },
        "relatives": {
            "mother_name": "Jane",
            "mother_salary": 2000,
            "mother_gender": "Female",
            "father_name": "Bob",
            "father_salary": 2001,
            "father_gender": "Male"
        }
    })__";


    TEmploee empl1;
    converter.Parse(empl1, ReadJsonValue(json));

    TEmploee empl2;
    converter.Parse(empl2, TRapidJsonRootNode(json));

    for (const TEmploee& empl : {empl1, empl2}) {
        TRapidJsonRootWriter w;
        converter.Serialize(empl,  w);
        Cerr << "------------------------------------------------------------------------\n";
        Cerr << w.GetString() << Endl;
        Cerr << "========================================================================\n";

        UNIT_ASSERT_EQUAL(empl.GetName(), "Gregor");
        UNIT_ASSERT_EQUAL(empl.GetSalary(), 145);
        UNIT_ASSERT_EQUAL(empl.GetFeatures().GetPersonality(), "saint");
        UNIT_ASSERT_EQUAL(empl.GetFeatures().GetMother().GetName(), "Jane");
        UNIT_ASSERT_EQUAL(empl.GetFeatures().GetMother().GetSalary(), 2000);
        UNIT_ASSERT_EQUAL(empl.GetFeatures().GetFather().GetName(), "Bob");
        UNIT_ASSERT_EQUAL(empl.GetFeatures().GetFather().GetSalary(), 2001);
        UNIT_ASSERT_CONTAINS(empl.GetHobbies(), "books");
        UNIT_ASSERT_CONTAINS(empl.GetHobbies(), "theatre");
        UNIT_ASSERT_CONTAINS(empl.GetHobbies(), "murders");
        UNIT_ASSERT_EQUAL(empl.GetPets(0).GetNickname(), "Tom");
        UNIT_ASSERT_EQUAL(empl.GetPets(0).GetSpecies(), "cat");
        UNIT_ASSERT_EQUAL(empl.GetPets(1).GetNickname(), "Jerry");
        UNIT_ASSERT_EQUAL(empl.GetPets(1).GetSpecies(), "mouse");
    }
}

}
