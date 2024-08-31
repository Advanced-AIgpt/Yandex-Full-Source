#include "jinja2_compiler.h"
#include "jinja2_protoc.h"
#include "jinja2_render.h"
#include "string_convertions.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/common/env.h>

namespace NAlice {

Y_UNIT_TEST_SUITE(Jinja2Compiler) {
    Y_UNIT_TEST(TestProtoc) {
        // Use jinja2_compiler/proto/config.proto as a template
        const TVector<TString> includeFolders;
        TVector<TString> protoRoot;
        TProtoCompiler compiler(includeFolders);

        protoRoot.push_back(ArcadiaFromCurrentLocation(__SOURCE_FILE__, "ut/test.proto"));
        const google::protobuf::DescriptorPool* descr = compiler.Compile(protoRoot);
        UNIT_ASSERT(descr);
    }

    Y_UNIT_TEST(TestRender1) {
        TJinja2Render render;

        UNIT_ASSERT(render.LoadTemplateString("Hello world!"));
        UNIT_ASSERT_STRINGS_EQUAL(render.Render(), "Hello world!");
    }

    Y_UNIT_TEST(TestRender2) {
        TJinja2Render render;

        render.GetTemplateEnv().AddGlobal("Var1", jinja2::Value{"Hello"});
        render.GetTemplateEnv().AddGlobal("Var2", jinja2::Value{"world"});
        UNIT_ASSERT(render.LoadTemplateString("{{Var1}} {{Var2}}!"));
        UNIT_ASSERT_STRINGS_EQUAL(render.Render(), "Hello world!");
    }

    Y_UNIT_TEST(TestsStrings) {
        UNIT_ASSERT(DetectString("varone") == EStringDetector::LOWER_CASE); // Note we can not detect this style completely correct (LOWER_CASE or LOWER_CAMEL_CASE)
        UNIT_ASSERT(DetectString("var_one") == EStringDetector::LOWER_CASE);
        UNIT_ASSERT(DetectString("varOne") == EStringDetector::LOWER_CAMEL_CASE);
        UNIT_ASSERT(DetectString("VarOne") == EStringDetector::UPPER_CAMEL_CASE);
        UNIT_ASSERT(DetectString("VARONE") == EStringDetector::UPPER_CASE);
        UNIT_ASSERT(DetectString("VAR_ONE") == EStringDetector::UPPER_CASE);
        UNIT_ASSERT(DetectString("") == EStringDetector::UNDEFINED);

        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("varone", EStringDetector::LOWER_CASE), "varone");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("varone", EStringDetector::LOWER_CAMEL_CASE), "varone");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("varone", EStringDetector::UPPER_CAMEL_CASE), "Varone");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("varone", EStringDetector::UPPER_CASE), "VARONE");

        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("var_one", EStringDetector::LOWER_CASE), "var_one");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("var_one", EStringDetector::LOWER_CAMEL_CASE), "varOne");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("var_one", EStringDetector::UPPER_CAMEL_CASE), "VarOne");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("var_one", EStringDetector::UPPER_CASE), "VAR_ONE");

        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("varOne", EStringDetector::LOWER_CASE), "var_one");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("varOne", EStringDetector::LOWER_CAMEL_CASE), "varOne");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("varOne", EStringDetector::UPPER_CAMEL_CASE), "VarOne");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("varOne", EStringDetector::UPPER_CASE), "VAR_ONE");

        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("VarOne", EStringDetector::LOWER_CASE), "var_one");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("VarOne", EStringDetector::LOWER_CAMEL_CASE), "varOne");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("VarOne", EStringDetector::UPPER_CAMEL_CASE), "VarOne");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("VarOne", EStringDetector::UPPER_CASE), "VAR_ONE");

        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("VARONE", EStringDetector::LOWER_CASE), "varone");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("VARONE", EStringDetector::LOWER_CAMEL_CASE), "varone");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("VARONE", EStringDetector::UPPER_CAMEL_CASE), "Varone");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("VARONE", EStringDetector::UPPER_CASE), "VARONE");

        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("VAR_ONE", EStringDetector::LOWER_CASE), "var_one");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("VAR_ONE", EStringDetector::LOWER_CAMEL_CASE), "varOne");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("VAR_ONE", EStringDetector::UPPER_CAMEL_CASE), "VarOne");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("VAR_ONE", EStringDetector::UPPER_CASE), "VAR_ONE");

        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("", EStringDetector::LOWER_CASE), "");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("", EStringDetector::LOWER_CAMEL_CASE), "");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("", EStringDetector::UPPER_CAMEL_CASE), "");
        UNIT_ASSERT_STRINGS_EQUAL(ConvertString("", EStringDetector::UPPER_CASE), "");
    }
}

} // namespace NAlice
