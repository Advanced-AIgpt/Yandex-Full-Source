#include <library/cpp/testing/benchmark/bench.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/protobuf/json/json2proto.h>
#include <alice/cuttlefish/library/convert/bench/proto/my.pb.h>
#include <alice/cuttlefish/library/convert/bench/proto/my.traits.pb.h>
#include <alice/cuttlefish/library/convert/converter.h>
#include <alice/cuttlefish/library/convert/rapid_node.h>
#include <google/protobuf/util/json_util.h>

namespace {

NJson::TJsonValue ReadJson(TStringBuf raw)
{
    NJson::TJsonValue val;
    NJson::ReadJsonTree(raw, &val, true);
    return val;
}

using TEmployeeConverter = NAlice::NCuttlefish::NConvert::TConverter<TEmploee>;
using NAlice::NCuttlefish::NConvert::TRapidJsonRootNode;


TEmployeeConverter CreateEmployeeConverter()
{
    using PT_Emploee = NProtoTraits::TMessageTraits<::TEmploee>;
    using PT_Features = NProtoTraits::TMessageTraits<::TFeatures>;
    using PT_Parent = NProtoTraits::TMessageTraits<::TParent>;

    TEmployeeConverter conv;
    auto b = conv.Build();
    b.SetValue<PT_Emploee::Name>("Name");
    b.SetValue<PT_Emploee::Salary>("Salary");
    {
        auto f = b.Sub<PT_Emploee::Features>();
        f.SetValue<PT_Features::Personality>("Features/Personality");
        {
            auto s = f.Sub<PT_Features::Mother>();
            s.SetValue<PT_Parent::Name>("Features/Mother/Name");
            s.SetValue<PT_Parent::Salary>("Features/Mother/Salary");
        } {
            auto s = f.Sub<PT_Features::Father>();
            s.SetValue<PT_Parent::Name>("Features/Father/Name");
            s.SetValue<PT_Parent::Salary>("Features/Father/Salary");
        }
    }
    b.ForEachInArray().Append<PT_Emploee::Interests>().From("Interests");

    return conv;
}

void ManualParse(TEmploee& dst, const NJson::TJsonValue& src)
{
    dst.Clear();

    dst.SetName(src["Name"].GetStringSafe());
    dst.SetSalary(src["Salary"].GetUIntegerSafe());
    for (const auto& item : src["Interests"].GetArray()) {
        dst.AddInterests(item.GetString());
    }

    const NJson::TJsonValue& featuresSrc = src["Features"];
    TFeatures& featuresDst = *dst.MutableFeatures();
    featuresDst.SetPersonality(featuresSrc["Personality"].GetStringSafe());

    const NJson::TJsonValue& motherSrc = featuresSrc["Mother"];
    TParent& motherDst = *featuresDst.MutableMother();
    motherDst.SetName(motherSrc["Name"].GetStringSafe());
    motherDst.SetSalary(motherSrc["Salary"].GetUIntegerSafe());

    const NJson::TJsonValue& fatherSrc = featuresSrc["Father"];
    TParent& fatherDst = *featuresDst.MutableFather();
    fatherDst.SetName(fatherSrc["Name"].GetStringSafe());
    fatherDst.SetSalary(fatherSrc["Salary"].GetUIntegerSafe());
}

struct TContext {
    TContext()
        : JsonConverter(CreateEmployeeConverter())
        , RawJsonSource(NResource::Find("/json_src.json"))
        , JsonSource(ReadJson(RawJsonSource))

    { }

    const TEmployeeConverter JsonConverter;
    const TString RawJsonSource;
    NJson::TJsonValue JsonSource;
    mutable TEmploee ProtobufTarget;
};

const TContext CONTEXT;

} // anonymous namespace


Y_CPU_BENCHMARK(_CHECK, iface) {
    for (size_t i = 0; i < iface.Iterations(); ++i) {
        NProtobufJson::Json2Proto(CONTEXT.JsonSource, CONTEXT.ProtobufTarget);
        Cerr << "--- Json2Proto:         " << CONTEXT.ProtobufTarget << Endl;

        {
            CONTEXT.ProtobufTarget.Clear();
            CONTEXT.JsonConverter.Parse(CONTEXT.ProtobufTarget, TRapidJsonRootNode(CONTEXT.RawJsonSource));
            Cerr << "--- TEmployeeConverter: " << CONTEXT.ProtobufTarget << Endl;
        } {
            CONTEXT.ProtobufTarget.Clear();
            ManualParse(CONTEXT.ProtobufTarget, CONTEXT.JsonSource);
            Cerr << "--- Manual:             " << CONTEXT.ProtobufTarget << Endl;
        }
    }
}

// ------------------------------------------------------------------------------------------------

Y_CPU_BENCHMARK(Json2Proto, iface) {
    for (size_t i = 0; i < iface.Iterations(); ++i) {
        NProtobufJson::Json2Proto(CONTEXT.JsonSource, CONTEXT.ProtobufTarget);
    }
}

Y_CPU_BENCHMARK(TEmployeeConverter, iface) {
    for (size_t i = 0; i < iface.Iterations(); ++i) {
        CONTEXT.ProtobufTarget.Clear();
        CONTEXT.JsonConverter.Parse(CONTEXT.ProtobufTarget, CONTEXT.JsonSource);
    }
}

Y_CPU_BENCHMARK(Manual, iface) {
    for (size_t i = 0; i < iface.Iterations(); ++i) {
        ManualParse(CONTEXT.ProtobufTarget, CONTEXT.JsonSource);
    }
}

// ------------------------------------------------------------------------------------------------

Y_CPU_BENCHMARK(RawJson2Proto, iface) {
    for (size_t i = 0; i < iface.Iterations(); ++i) {
        NProtobufJson::Json2Proto(CONTEXT.RawJsonSource, CONTEXT.ProtobufTarget);
    }
}

Y_CPU_BENCHMARK(RawTEmployeeConverter, iface) {
    for (size_t i = 0; i < iface.Iterations(); ++i) {
        CONTEXT.ProtobufTarget.Clear();
        CONTEXT.JsonConverter.Parse(CONTEXT.ProtobufTarget, TRapidJsonRootNode(CONTEXT.RawJsonSource));
    }
}

Y_CPU_BENCHMARK(RawManual, iface) {
    NJson::TJsonValue val;
    for (size_t i = 0; i < iface.Iterations(); ++i) {
        NJson::ReadJsonTree(CONTEXT.RawJsonSource, &val, true);
        ManualParse(CONTEXT.ProtobufTarget, val);
    }
}
