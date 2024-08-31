#include <alice/cuttlefish/library/convert/rapid_node.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <util/generic/strbuf.h>

using namespace NAlice::NCuttlefish::NConvert;


// -------------------------------------------------------------------------------------------------
Y_UNIT_TEST_SUITE(RapidNode) {

void IterateArray(TRapidJsonNode& root);

void IterateMap(TRapidJsonNode& root) {
    TString key;
    TRapidJsonNode node;
    while (root.NextMapNode(key, node)) {
        Cerr << "\"" << key << "\": ";
        if (node.IsMap()) {
            Cerr << "{";
            IterateMap(node);
            Cerr << "}, ";
        }
        else if (node.IsArray()) {
            Cerr << "[";
            IterateArray(node);
            Cerr << "], ";
        }
        else if (node.IsValue()) {
            Cerr << "\"" << node.GetValue() << "\", ";
        }
    }
}

void IterateArray(TRapidJsonNode& root) {
    TRapidJsonNode node;
    while (root.NextArrayNode(node)) {
        if (node.IsMap()) {
            Cerr << "{";
            IterateMap(node);
            Cerr << "}, ";
        }
        else if (node.IsArray()) {
            Cerr << "[";
            IterateArray(node);
            Cerr << "], ";
        }
        else if (node.IsValue()) {
            Cerr << "\"" << node.GetValue() << "\", ";
        }
    }
}


Y_UNIT_TEST(RapidNodeBasic) {
    UNIT_ASSERT(TRapidJsonRootNode("{}").IsMap());
    UNIT_ASSERT(TRapidJsonRootNode("[]").IsArray());

    TString rawData = R"__({
        "mother": {
            "name": "Janine"
        },
        "some": {
            "path": [
                {"name": "Tom", "type": "cat"},
                {"name": "Jerry", "type": "mouse"},
                {"name": "Lucy", "type": "fish"}
            ],
            "the-map": {
                "one": 1,
                "two": 2,
                "three": 3,
                "four": 4,
                "five": 5
            }
        },
        "the-map-again": {
            "dog": "Robert",
            "bird": "Anette"
        }
    })__";

    TRapidJsonRootNode root(rawData);
    UNIT_ASSERT(root.IsMap());
    IterateMap(root);
}

}
