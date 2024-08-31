#include "contacts_granet.h"

#include <alice/library/proto/proto.h>
#include <alice/nlu/granet/lib/compiler/compiler.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/subst.h>

using namespace NGranet;
using namespace NAlice::NContacts;

TString SortLines(TStringBuf text) {
    auto lines = StringSplitter(text).Split('\n').ToList<TString>();
    for (auto& line : lines) {
        SubstGlobal(line, "w0", "w?");
        SubstGlobal(line, "w1", "w?");
        SubstGlobal(line, "w2", "w?");
    }
    Sort(lines);
    return JoinSeq("\n", lines);
}

Y_UNIT_TEST_SUITE(NUserEntity_ParseContacts) {

    Y_UNIT_TEST(ParseContacts_Empty) {
        {
            const TStringBuf contactsProtoText = "";
            NAlice::TClientEntity contactsProto = NAlice::ParseProtoText<NAlice::TClientEntity>(contactsProtoText);
            const TMaybe<NGranet::NCompiler::TSourceTextCollection> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            UNIT_ASSERT(!contacts.Defined());
        }
        {
            const TStringBuf contactsProtoText = R"(
                Items {
                    key: "key",
                    value: {
                        Instances: [
                            {Phrase: ":,?!&$%^-=+"}
                        ]
                    }
                }
            )";
            NAlice::TClientEntity contactsProto = NAlice::ParseProtoText<NAlice::TClientEntity>(contactsProtoText);
            const TMaybe<NGranet::NCompiler::TSourceTextCollection> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            UNIT_ASSERT(!contacts.Defined());
        }

    }

    Y_UNIT_TEST(ParseContacts_SomeContacts) {
        {
            const TStringBuf contactsProtoText = R"(
                Items {
                    key: "vitaly",
                    value: {
                        Instances: [
                            {Phrase: "Витя"}
                        ]
                    }
                }
            )";

            NAlice::TClientEntity contactsProto = NAlice::ParseProtoText<NAlice::TClientEntity>(contactsProtoText);
            const TMaybe<NGranet::NCompiler::TSourceTextCollection> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            UNIT_ASSERT(contacts.Defined());
            UNIT_ASSERT_VALUES_EQUAL(1, contacts.GetRef().Texts.size());
            UNIT_ASSERT_VALUES_EQUAL("Main", contacts.GetRef().MainTextPath);
            const auto textPtr = contacts.GetRef().Texts.FindPtr(contacts.GetRef().MainTextPath);
            UNIT_ASSERT(textPtr);
            const TStringBuf expectedText = R"(
entity device.address_book.item_name:
 keep_overlapped: true
 enable_synonyms: true
 keep_variants: true
 values:
  "vitaly": $w0
$w0: витя; %lemma_as_is; витя
$p1: 11111111111111; %weight 0.01; $sys.void
$p2: 11111111111111; %weight 0.0001; $sys.void
$p3: 11111111111111; %weight 0.000001; $sys.void
)";
            UNIT_ASSERT_VALUES_EQUAL(SortLines(expectedText), SortLines(*textPtr));
            NGranet::NCompiler::TCompiler compiler({.IsCompatibilityMode = true});
            compiler.CompileFromSourceTextCollection(contacts.GetRef());
        }
        {
            const TStringBuf contactsProtoText = R"(
                Items {
                    key: "vitaly",
                    value: {
                        Instances: [
                            {Phrase: "Витя"}
                        ]
                    }
                }
                Items {
                    key: "petya",
                    value: {
                        Instances: [
                            {Phrase: "Petya"}
                        ]
                    }
                }
            )";

            NAlice::TClientEntity contactsProto = NAlice::ParseProtoText<NAlice::TClientEntity>(contactsProtoText);
            const TMaybe<NGranet::NCompiler::TSourceTextCollection> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            UNIT_ASSERT(contacts.Defined());
            UNIT_ASSERT_VALUES_EQUAL(1, contacts.GetRef().Texts.size());
            UNIT_ASSERT_VALUES_EQUAL("Main", contacts.GetRef().MainTextPath);
            const auto textPtr = contacts.GetRef().Texts.FindPtr(contacts.GetRef().MainTextPath);
            UNIT_ASSERT(textPtr);
            const TStringBuf expectedText = R"(
entity device.address_book.item_name:
 keep_overlapped: true
 enable_synonyms: true
 keep_variants: true
 values:
  "vitaly": $w0
  "petya": $w1
$w0: витя; %lemma_as_is; витя
$w1: petya; %lemma_as_is; petya
$p1: 11111111111111; %weight 0.01; $sys.void
$p2: 11111111111111; %weight 0.0001; $sys.void
$p3: 11111111111111; %weight 0.000001; $sys.void
)";
            UNIT_ASSERT_VALUES_EQUAL(SortLines(expectedText), SortLines(*textPtr));
            NGranet::NCompiler::TCompiler compiler({.IsCompatibilityMode = true});
            compiler.CompileFromSourceTextCollection(contacts.GetRef());
        }
        {
            const TStringBuf contactsProtoText = R"(
                Items {
                    key: "vitaly",
                    value: {
                        Instances: [
                            {Phrase: "Витя = Пётрушка Васильевич!!!!"}
                        ]
                    }
                }
            )";

            NAlice::TClientEntity contactsProto = NAlice::ParseProtoText<NAlice::TClientEntity>(contactsProtoText);
            const TMaybe<NGranet::NCompiler::TSourceTextCollection> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            UNIT_ASSERT(contacts.Defined());
            UNIT_ASSERT_VALUES_EQUAL(1, contacts.GetRef().Texts.size());
            UNIT_ASSERT_VALUES_EQUAL("Main", contacts.GetRef().MainTextPath);
            const auto textPtr = contacts.GetRef().Texts.FindPtr(contacts.GetRef().MainTextPath);
            UNIT_ASSERT(textPtr);
            const TStringBuf expectedText = R"(
entity device.address_book.item_name:
 keep_overlapped: true
 enable_synonyms: true
 keep_variants: true
 values:
  "vitaly": [$w0 $w1 $w2]; $p1 [$w0 $w1]; $p1 [$w0 $w2]; $p1 [$w1 $w2]; $p2 $w0; $p2 $w1; $p2 $w2
$w0: витя; %lemma_as_is; витя
$w1: петрушка; %lemma_as_is; петрушка
$w2: васильевич; %lemma_as_is; васильевич
$p1: 11111111111111; %weight 0.01; $sys.void
$p2: 11111111111111; %weight 0.0001; $sys.void
$p3: 11111111111111; %weight 0.000001; $sys.void
)";
            UNIT_ASSERT_VALUES_EQUAL(SortLines(expectedText), SortLines(*textPtr));
            NGranet::NCompiler::TCompiler compiler({.IsCompatibilityMode = true});
            compiler.CompileFromSourceTextCollection(contacts.GetRef());
        }
        {
            const TStringBuf contactsProtoText = R"(
                Items {
                    key: "vitaly",
                    value: {
                        Instances: [
                            {Phrase: "Витя = Петрушка Васильевич!!!!2"}
                        ]
                    }
                }
            )";

            NAlice::TClientEntity contactsProto = NAlice::ParseProtoText<NAlice::TClientEntity>(contactsProtoText);
            const TMaybe<NGranet::NCompiler::TSourceTextCollection> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            UNIT_ASSERT(contacts.Defined());
            UNIT_ASSERT_VALUES_EQUAL(1, contacts.GetRef().Texts.size());
            UNIT_ASSERT_VALUES_EQUAL("Main", contacts.GetRef().MainTextPath);
            const auto textPtr = contacts.GetRef().Texts.FindPtr(contacts.GetRef().MainTextPath);
            UNIT_ASSERT(textPtr);
            const TStringBuf expectedText = R"(
entity device.address_book.item_name:
 keep_overlapped: true
 enable_synonyms: true
 keep_variants: true
 values:
  "vitaly": [$w0 $w1 $w2 $w3]; $p1 [$w1 $w2 $w3]; $p1 [$w0 $w2 $w3]; $p1 [$w0 $w1 $w3]; $p1 [$w0 $w1 $w2]; $p2 [$w0 $w1]; $p2 [$w0 $w2]; $p2 [$w0 $w3]; $p2 [$w1 $w2]; $p2 [$w1 $w3]; $p2 [$w2 $w3]; $p3 $w0; $p3 $w1; $p3 $w2; $p3 $w3
$w0: витя; %lemma_as_is; витя
$w1: петрушка; %lemma_as_is; петрушка
$w2: васильевич; %lemma_as_is; васильевич
$w3: 2; %lemma_as_is; 2
$p1: 11111111111111; %weight 0.01; $sys.void
$p2: 11111111111111; %weight 0.0001; $sys.void
$p3: 11111111111111; %weight 0.000001; $sys.void
)";
            UNIT_ASSERT_VALUES_EQUAL(SortLines(expectedText), SortLines(*textPtr));
            NGranet::NCompiler::TCompiler compiler({.IsCompatibilityMode = true});
            compiler.CompileFromSourceTextCollection(contacts.GetRef());
        }
    }

};
