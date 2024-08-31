#include <alice/boltalka/generative/inference/core/tokenizer.h>
#include <alice/boltalka/generative/inference/core/proto/tokenizer_type.pb.h>

#include <library/cpp/testing/unittest/registar.h>


using namespace NGenerativeBoltalka;

Y_UNIT_TEST_SUITE(ZelibobaTokenizer) {
    Y_UNIT_TEST(NamesSepTokenizer) {
        TTokenizer::TParams params;
        params.Prefix = "Алиса\nЯ — Алиса, самый умный в мире искусственный интеллект. Я весь день разговариваю с людьми, помогаю им, всегда отвечаю на вопросы и рассказываю что-нибудь интересное. Сейчас с удовольствием говорю с вами.\n---\n";
        params.TurnSeparator = "\n---\n";
        params.NameSeparator = "\n";
        params.AliceName = "Алиса";
        params.UserName = "Человек";
        const TString checkpointPath = "./zeliboba_1_0_boltalka/v19_better";
        params.BpeVocPath = checkpointPath + "/bpe.voc";
        params.TokenToIdVocPath = checkpointPath + "/token_to_id.voc";
        TZelibobaSEPNamesTokenizer tokenizer(params);
        TVector<TString> context = {"привет", "как дела?", "хорошо"};

        UNIT_ASSERT_EQUAL(ToString(tokenizer.GetContextStringBuilder(context)), R"(Алиса
Я — Алиса, самый умный в мире искусственный интеллект. Я весь день разговариваю с людьми, помогаю им, всегда отвечаю на вопросы и рассказываю что-нибудь интересное. Сейчас с удовольствием говорю с вами.
---
Человек
привет
---
Алиса
как дела?
---
Человек
хорошо
---
Алиса
)");

        TVector<TString> expected = {
                "Привет!", "Привет! Как дела?", "Привет привет!",  "Привет! Как настроение?",
                "И вам привет!", "Как настроение?", "Привет! Как поживаете?", "Добрый день!",
                "Доброе утро! Как дела?", "Я так рада вас слышать!"
        };
    }
}