#include <alice/nlu/granet/lib/compiler/compiler.h>
#include <alice/nlu/granet/lib/compiler/source_text_collection.h>
#include <alice/nlu/granet/lib/test/batch.h>
#include <alice/nlu/granet/lib/ut/granet_tester.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/join.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetPart09_Files) {

    Y_UNIT_TEST(Simple) {
        NCompiler::TResourceDataLoader reader;
        TGranetTester tester(CompileGrammarFromPath("/granet/hello_world.grnt", {}, &reader));
        tester.TestTagger("hello_world", true, "'Здравствуй'(hello:) 'мир'(world:)");
        tester.TestTagger("hello_world", false, "Не здравствуй мир");
        tester.TestTagger("hello_world", true, "Ну блин 'здаров'(hello:) блин 'мир'(world:)");
    }

    Y_UNIT_TEST(Music) {
        NCompiler::TResourceDataLoader reader;
        TGranetTester tester(CompileGrammarFromPath("/granet/music.grnt", {}, &reader));
        tester.TestTagger("music.play_song", true, "'Поставь'(request:) 'спят усталые игрушки'(song:)");
    }

    void TestCollection(const TGrammar::TRef& grammar) {
        TGranetTester tester(grammar);
        tester.TestTagger("hello_world", true, "'Привет'(hello:) 'мир'(world:)");
        tester.TestTagger("hello_world", false, "Поставь песню привет мир");
        tester.TestTagger("music.play_song", true, "'Поставь песню'(request:) 'привет мир'(song:)");
        tester.TestTagger("music.play_song", false, "Привет мир");
    }

    Y_UNIT_TEST(Collection) {
        NCompiler::TResourceDataLoader reader;
        const TFsPath path = "/granet/collection.grnt";

        TestCollection(CompileGrammarFromPath(path, {}, &reader));

        const NCompiler::TSourceTextCollection toWrite =
            NCompiler::TCompiler().CollectSourceTexts(path, {}, &reader);
        TestCollection(NCompiler::TCompiler().CompileFromSourceTextCollection(toWrite));

        NCompiler::TSourceTextCollection toRead;
        toRead.FromCompressedBase64(toWrite.ToCompressedBase64());
        TestCollection(NCompiler::TCompiler().CompileFromSourceTextCollection(toRead));
    }

    Y_UNIT_TEST(TestGrammar) {
        NCompiler::TResourceDataLoader reader;
        TGrammar::TConstRef grammar = CompileGrammarFromPath("/granet/collection.grnt", {}, &reader);
        const TFsPath batchPath = BinaryPath("alice/nlu/granet/lib/ut/data/batch");
        const TString message = NBatch::EasyUnitTest(grammar, {.BatchDir = batchPath});
        if (!message.empty()) {
            UNIT_FAIL(message);
        }
    }
}
