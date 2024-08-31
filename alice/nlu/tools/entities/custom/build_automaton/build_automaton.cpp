#include <alice/nlu/libs/occurrence_searcher/automaton_builder_app.h>
#include <alice/nlu/proto/entities/custom.pb.h>
#include <util/string/split.h>

namespace NAlice {
namespace NNlu {

template<>
void Update(TCustomEntityValues* value, const TCustomEntityValues& newValue) {
    Y_ASSERT(value);
    for (const auto& newEntityValue : newValue.GetCustomEntityValues()) {
        auto* entityValue = (*value).AddCustomEntityValues();
        (*entityValue) = newEntityValue;
    }
}

namespace {

class TApp : public TAutomatonBuilderApp<TCustomEntityValues> {
public:
    using TAutomatonBuilderApp::TAutomatonBuilderApp;

private:
    TCustomEntityValues DeserializeValue(const TStringBuf& s) const override {
        TCustomEntityValues values;
        TString type;
        TString value;
        Split(s, "\t", type, value);
        auto* pbCustomEntityValues = values.MutableCustomEntityValues();
        auto* pbCustomEntityValue = pbCustomEntityValues->Add();
        pbCustomEntityValue->SetType(type);
        pbCustomEntityValue->SetValue(value);
        return values;
    }
};

} // namespace
} // namespace NNlu
} // namespace NAlice


//
// NOTE(the0): The tool builds data file for the CustomEntities rule and prints it to stdout.
// Data file basically stores (<STRING>, <VALUE>) pairs where Value is a list of (<CUSTOM_ENTITY_TYPE>, <CUSTOM_ENTITY_VALUE>)
// pairs. The data is generated from input lines where each line meets the following format:
//
// <STRING> <TAB> <CUSTOM_ENTITY_TYPE> <TAB> <CUSTOM_ENTITY_VALUE>
//
// Lines with the same <STRING> are combined together to produce a list of
// all the (<CUSTOM_ENTITY_TYPE>, <CUSTOM_ENTITY_VALUE>) pairs which itself serves as a <VALUE> for the (<STRING>, <VALUE>)
// pair stored in the resulting data file.
//
// The input file could be generated from the well-known JSON-based VINS custom entity description format:
//
// ==== custom_entity_name.json ====
// {
//     "custom_entity_value_1": {
//         "string_1_1",
//         ...
//         "string_1_k1"
//     },
//     ...
//     "custom_entity_value_n": {
//         "string_n_1",
//         ...
//         "string_n_kn"
//     },
// }
// =================================
//
// The tool that collects all the project-specific ('scenarios', 'navi', ...) custom entities known to VINS,
// takes a required subset (which is a full set by default) of custom entity types
// and generates file meeting the requirements of the described input format is located at
// $VINS_DM/tools/nlu/print_custom_entity_strings.py
// where $VINS_DM is a path to the root of a VINS repository.
//

int main(int argc, const char* argv[]) {
    return NAlice::NNlu::TApp(argc, argv).Run();
}
