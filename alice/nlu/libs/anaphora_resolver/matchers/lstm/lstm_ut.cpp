#include <alice/begemot/lib/session_conversion/session_conversion.h>
#include <alice/nlu/libs/anaphora_resolver/matchers/lstm/lstm.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/folder/path.h>
#include <util/stream/str.h>
#include <util/string/join.h>
#include <util/string/split.h>

namespace {
    constexpr TStringBuf MODEL_DIRECTORY = "search/wizard/data/wizard/AliceAnaphoraMatcher";

    constexpr TStringBuf EMBEDDER_EMBEDDINGS_PATH = "embeddings";
    constexpr TStringBuf EMBEDDER_DICTIONARY_PATH = "embeddings_dictionary.trie";

    constexpr TStringBuf MODEL_PROTOBUF_FILE = "lstm_model/model.pb";
    constexpr TStringBuf MODEL_DESCRIPTION_FILE = "lstm_model/model_description.json";
    constexpr TStringBuf SPECIAL_EMBEDDINGS_FILE = "lstm_model/special_embeddings.json";

    NAlice::TTokenEmbedder GetTokenEmbedder() { // This embedder is a cut version of default one.
        return NAlice::TTokenEmbedder(TBlob::FromString(NResource::Find(EMBEDDER_EMBEDDINGS_PATH)),
                                      TBlob::FromString(NResource::Find(EMBEDDER_DICTIONARY_PATH)));
    }

    NAlice::TLstmAnaphoraMatcherModel GetLstmModel() {
        const TFsPath modelDirectory = BinaryPath(MODEL_DIRECTORY);

        TFileInput modelProtobuf(modelDirectory / MODEL_PROTOBUF_FILE);
        TFileInput modelDescription(modelDirectory / MODEL_DESCRIPTION_FILE);
        TFileInput specialEmbeddings(modelDirectory / SPECIAL_EMBEDDINGS_FILE);

        return NAlice::TLstmAnaphoraMatcherModel(&modelProtobuf,
                                                 NAlice::TLstmAnaphoraMatcherModel::ReadJsonModelDescription(&modelDescription),
                                                 NAlice::TLstmAnaphoraMatcherModel::ReadJsonSpecialTokenEmbeddings(&specialEmbeddings),
                                                 GetTokenEmbedder());
    }

    NBg::NProto::TAliceSessionResult PrepareSession(const TVector<TString>& history) {
        NBg::NProto::TAliceSessionResult session;

        for (size_t i = 0; i < history.size() - 1; ++i) {
            auto phraseProto = session.AddDialoguePhrases();
            for (const TStringBuf token : StringSplitter(history[i]).Split(' ').SkipEmpty()) {
                phraseProto->AddTokens(TString{token});
            }
        }

        for (const TStringBuf token : StringSplitter(history.back()).Split(' ').SkipEmpty()) {
             session.MutableNormalizedRequest()->AddTokens(TString{token});
        }

        NBg::AddGrammarAnalysis(&session);

        return session;
    }

    TString GetMatch(const NAlice::TLstmAnaphoraMatcherModel& matcher, const TVector<TString>& history) {
        const auto session = PrepareSession(history);

        const NBg::TMentionExtractionOptions options;
        const auto mentions = NBg::ExtractMentions(options, session);
        UNIT_ASSERT(mentions.Pronouns.size() == 1);

        const auto match = matcher.Predict(mentions.DialoguePhrases, mentions.Entities, mentions.Pronouns[0]);

        UNIT_ASSERT(match.Defined());

        TVector<TString> matchTokens;
        const auto matchSpan = match->Antecedent.MentionInPhrase.TokenSegment;
        for (size_t i = matchSpan.Begin; i < matchSpan.End; ++i) {
            matchTokens.push_back(mentions.DialoguePhrases[match->Antecedent.PhrasePos].Tokens[i]);
        }

        return JoinSeq(" ", matchTokens);
    }
} // namespace anonymous

Y_UNIT_TEST_SUITE(TLstmAnaphoraResolverMatcherSuite) {

Y_UNIT_TEST(MinTest) {
    auto model = GetLstmModel();
    model.EstablishSessionIfNotYet();

    NVins::TSample request;
    request.Tokens = {"pronoun"};
    const auto pronoun = NAlice::TMentionInDialogue(/*phraseId*/0, /*Start*/0, /*End*/1);
    const auto result = model.Predict({request}, {}, pronoun);
    UNIT_ASSERT(result.Empty());
}

Y_UNIT_TEST(Apply) {
    auto model = GetLstmModel();
    model.EstablishSessionIfNotYet();

    TVector<NVins::TSample> history(3);
    history[0].Tokens = {"какая", "сейчас", "погода"};
    history[1].Tokens = {"идет", "дождь"};
    history[2].Tokens = {"когда", "он", "закончится"};

    const TVector<NAlice::TMentionInDialogue> entityPositions = {NAlice::TMentionInDialogue(0, 1, 2),
                                                                 NAlice::TMentionInDialogue(0, 2, 3),
                                                                 NAlice::TMentionInDialogue(1, 1, 2)};
    const NAlice::TMentionInDialogue pronounPosition = NAlice::TMentionInDialogue(2, 1, 2);

    const auto result = model.Predict(history, entityPositions, pronounPosition);
    UNIT_ASSERT(!result.Empty());
    UNIT_ASSERT(result->Anaphora == pronounPosition);
    UNIT_ASSERT(result->Antecedent == entityPositions[2]); // correct match is "погода"
}

static const TVector<std::pair<TVector<TString>, TString>> TEST_DATA = {
    {
        {
            "кто такой путин",
            "владимир владимирович путин - российский государственный и политический деятель действующий президент российской федерации верховный главнокомандующий вооруженными силами российской федерации",
            "какой у него рост"
        },
        "путин"
    },
    {
        {
            "кто такой путин",
            "владимир владимирович путин - российский государственный и политический деятель действующий президент российской федерации верховный главнокомандующий вооруженными силами российской федерации",
            "какой у него рост",
            "170 см",
            "а сколько ему лет"
        },
        "путин"
    },
    {
        {
            "кто такой нолан",
            "кристофер нолан британский и американский кинорежиссер сценарист и продюсер",
            "когда он родился"
        },
        "нолан"
    },
    {
        {
            "кто такой нолан",
            "кристофер нолан британский и американский кинорежиссер сценарист и продюсер",
            "когда он родился",
            "30 июля 1970 г 49 лет вестминстер лондон англия",
            "какой у него рост"
        },
        "нолан"
    },
    {
        {
            "кто президент россии",
            "владимир владимирович путин",
            "какой у него рост"
        },
        "владимир владимирович путин"
    },
    {
        {
            "кто президент россии",
            "владимир владимирович путин",
            "какой у него рост",
            "170 см",
            "а сколько ему лет",
        },
        "владимир владимирович путин"
    },
    {
        {
            "самая высокая точка европы",
            "эльбрус",
            "в какой стране он находится"
        },
        "эльбрус"
    },
    {
        {
            "кто создал компьютер",
            "кондрад цузе",
            "когда он родился"
        },
        "кондрад цузе"
    },
    {
        {
            "кто создал компьютер",
            "кондрад цузе",
            "когда он родился",
            "22 июня 1910 г",
            "где он родился"
        },
        "кондрад цузе"
    },
    {
        {
            "кто режиссер фильма интерстеллар",
            "кристофер нолан",
            "когда он родился"
        },
        "кристофер нолан"
    },
    {
        {
            "кто режиссер фильма интерстеллар",
            "кристофер нолан",
            "когда он родился",
            "30 июля 1970 г 49 лет вестминстер лондон англия",
            "какой у него рост"
        },
        "кристофер нолан"
    },
    {
        {
            "расскажи про глубокое обучение",
            "глубокое обучение — совокупность методов машинного обучения основанных на обучении представлениям а не специализированным алгоритмам под конкретные задачи",
            "оно сильно развито"
        },
        "глубокое обучение"
    },
    {
        {
            "что такое глубокое обучение",
            "глубокое обучение — совокупность методов машинного обучения основанных на обучении представлениям а не специализированным алгоритмам под конкретные задачи",
            "оно сильно развито"
        },
        "глубокое обучение"
    },
    {
        {
            "расскажи про борю",
            "ищу ответ",
            "а сколько ему лет"
        },
        "борю"
    },
    {
        {
            "кто такой навальный",
            "алексей анатольевич навальный род 4 июня 1976 военный городок бутынь одинцовского района московская область ссср — российский политик и общественный деятель юрист инвест-активист",
            "сколько ему лет",
        },
        "навальный"
    },
    {
        {
            "расскажи про билла гейтса",
            "билл гейтс — американский предприниматель и общественный деятель филантроп один из создателей и бывший крупнейший акционер компании microsoft",
            "сколько у него денег"
        },
        "билла гейтса"
    },
    {
        {
            "кто такой сократ",
            "сократ — древнегреческий философ учение которого знаменует поворот в философии — от рассмотрения природы и мира к рассмотрению человека",
            "когда он родился"
        },
        "сократ"
    },
    {
        {
            "кто такой витгенштейн",
            "людвиг витгенштейн австрийский философ австрийский философ и логик представитель аналитической философии один из крупнейших философов xx века",
            "когда он умер"
        },
        "витгенштейн"
    },
    {
        {
            "а что такое япония",
            "япония — островное государство в восточной азии",
            "а какая там завтра погода"
        },
        "япония"
    },
    {
        {
            "рязань",
            "рязань — город в россии",
            "погода там"
        },
        "рязань"
    },
    {
        {
            "привет",
            "ку",
            "как дела",
            "хорошо а у вас",
            "кто такой путин",
            "путин владимир владимирович — российский государственный деятель действующий президент российской федерации с 7 мая 2012 года",
            "сколько ему лет",
        },
        "путин"
    },
    {
        {
            "кто такая кэти перри",
            "кэти перри — американская певица композитор автор песен актриса посол доброй воли оон",
            "сколько ей лет",
        },
        "кэти перри"
    },
    {
        {
            "кто такой брэд питт",
            "уильям брэдли питт — американский актер и продюсер",
            "как зовут его первую жену",
        },
        "брэд питт"
    },
    {
        {
            "а кто такой сергей дружко",
            "сергей евгеньевич дружко — российский актер телеведущий певец музыкант режиссер телевидения",
            "его фото с женой",
        },
        "сергей дружко"
    },
    {
        {
            "аркадий волож",
            "аркадий юрьевич волож — сооснователь и генеральный директор группы компаний яндекс",
            "когда он основал яндекс"
        },
        "аркадий юрьевич волож"
    },
    {
        {
            "аркадий волож",
            "аркадий юрьевич волож — сооснователь и генеральный директор группы компаний яндекс",
            "когда он основал яндекс",
            "23 сентября 1997 года",
            "сколько у него денег"
        },
        "аркадий юрьевич волож"
    }
};

Y_UNIT_TEST(Quality) {
    auto model = GetLstmModel();
    model.EstablishSessionIfNotYet();

    for (auto&& [history, expectedMatch] : TEST_DATA) {
        UNIT_ASSERT_VALUES_EQUAL(GetMatch(model, history), expectedMatch);
    }
}

}
