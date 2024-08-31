#include "consts.h"

namespace NAlice::NHollywood::NGeneralConversation {

namespace {

TVector<TStringBuf> Concat(const TVector<TStringBuf>& first, const TVector<TStringBuf>& second) {
    TVector<TStringBuf> result;
    result.reserve(first.size() + second.size());
    result.insert(result.end(), first.begin(), first.end());
    result.insert(result.end(), second.begin(), second.end());

    return result;
}

} // namespace

const TVector<TStringBuf> FRAMES_PROACTIVITY =  {FRAME_PROACTIVITY_ALICE_DO, FRAME_PROACTIVITY_BORED};
const TVector<TStringBuf> FRAMES_MOVIE_QUESTION = {FRAME_MOVIE_DISCUSS, FRAME_YES_I_WATCH_IT, FRAME_LETS_DISCUSS_SPECIFIC_MOVIE};
const TVector<TStringBuf> FRAMES_WITH_ENTITY_SEARCH = Concat(FRAMES_PROACTIVITY, {FRAME_GC});
const TVector<TStringBuf> FRAMES_WITH_SEARCH_RESPONSE = {FRAME_GC};
const TVector<TStringBuf> FRAMES_WITH_SEARCH_SUGGESTS = Concat(FRAMES_PROACTIVITY, FRAMES_WITH_SEARCH_RESPONSE);
const TVector<TStringBuf> FRAMES_WITH_SPECIAL_RESPONSE = Concat(FRAMES_PROACTIVITY, Concat(FRAMES_MOVIE_QUESTION, {FRAME_LETS_DISCUSS_SOME_MOVIE}));
const THashSet<TStringBuf> KNOWN_MOVIE_CONTENT_TYPES = {TStringBuf("movie"), TStringBuf("cartoon"), TStringBuf("tv_show"), TStringBuf("tv_show_cartoon")};
const THashSet<TStringBuf> CHILD_CONTENT_TYPES = {TStringBuf("cartoon"), TStringBuf("tv_show_cartoon")};
const TVector<TStringBuf> SOURCES_ENTITY = {SOURCE_ENTITY_GAME, SOURCE_ENTITY_MOVIE, SOURCE_ENTITY_MUSIC};
const TVector<TStringBuf> DEFAULT_GC_FRAMES = {
    FRAME_LETS_DISCUSS_SPECIFIC_MOVIE,
    FRAME_LETS_DISCUSS_SOME_MOVIE,
    FRAME_PROACTIVITY_BORED,
    FRAME_PROACTIVITY_ALICE_DO,
    FRAME_PURE_GC_DEACTIVATE,
    FRAME_MICROINTENTS,
    FRAME_BANLIST,
    FRAME_FACTS_CROSSPROMO,
    FRAME_MOVIE_AKINATOR,
    FRAME_GENERATIVE_TOAST,
    FRAME_WIZ_DETECTION_POLITICS,
    FRAME_WIZ_DETECTION_UNETHICAL,
    FRAME_WIZ_DETECTION_PORN
};

const THashSet<TStringBuf> GC_DUMMY_FRAMES = {
    FRAME_BANLIST,
    FRAME_WIZ_DETECTION_POLITICS,
    FRAME_WIZ_DETECTION_UNETHICAL,
    FRAME_WIZ_DETECTION_PORN
};

const TVector<TStringBuf> WIZ_DETECTION_FRAMES = {
        FRAME_WIZ_DETECTION_POLITICS,
        FRAME_WIZ_DETECTION_UNETHICAL,
        FRAME_WIZ_DETECTION_PORN,
    };

const TStringBuf TALE_BAN_PREFIX = "Сказка\n";
const TStringBuf TALE_INIT_PREFIX = "Сказка про ";
const TVector<TString> CHARACTERS = {"Алладин", "Волшебник", "Дед Мороз",      "Дядя Фёдор",     "Котёнок Боря",
                                     "Медведь", "Рыцарь",    "Симба",          "Слонёнок",       "Чебурашка",
                                     "Эльза",   "Ящерица",   "добрый Лисёнок", "смелый Пожарный"};

const TVector<TString> TALE_AVATARS_IDS = {
    "/get-dialogs/1530877/sharing_alice_generative_tails",
    "/get-dialogs/1676983/sharing_alice_generative_tails_1",
    "/get-dialogs/758954/sharing_alice_generative_tails_2"
};
const THashMap<TUtf16String, TUtf16String> ACCUSATIVE_FIXLIST = {
    {u"Бэтмен", u"Бэтмена"},
    {u"Бэтмена", u"Бэтмена"},
    {u"Змей", u"Змея"},
    {u"Котики", u"Котиков"},
    {u"Лосяш", u"Лосяша"},
    {u"Лосяша", u"Лосяша"},
    {u"Нолик", u"Нолика"},
    {u"Нолика", u"Нолика"},
    {u"Симба", u"Симбу"},
    {u"Симбу", u"Симбу"},
    {u"Смелого Пожарного", u"Смелого Пожарного"},
    {u"Смелый Пожарный", u"Смелого Пожарного"},
    {u"Том Сойер", u"Тома Сойера"},
    {u"Тома Сойера", u"Тома Сойера"},
    {u"Человек", u"Человека"},
    {u"Колобок", u"Колобка"},
    {u"Колобка", u"Колобка"},
};

const TVector<TString> POSSESSIVE_PREPOSITIONS = {"о", "про", "на", "в", "при", "об", "где", "как"};
const TVector<TUtf16String> PREPOSITIONS_AND_CONJUNCTIONS =
    {u"и", u"с", u"в", u"к", u"о", u"во", u"на", u"об", u"от", u"по",
     u"из", u"за", u"над", u"про", u"под", u"при", u"где", u"как"};
const TVector<TUtf16String> QUESTIONS_WORDS = {u"кто", u"что", u"кого", u"чего", u"кому", u"чему", u"кем", u"чем",
                                               u"ком", u"каким", u"какой", u"какими", u"каком", u"каких", u"как",
                                               u"где", u"сколько", u"когда", u"куда", u"откуда", u"зачем", u"почему"};
const TVector<TGenerativeTaleState::EStage> GENERATIVE_TALE_TERMINAL_STAGES = {
    TGenerativeTaleState::Error,
    TGenerativeTaleState::SendMeMyTale,
    TGenerativeTaleState::SharingDone,
    TGenerativeTaleState::Stop,
};

} // namespace NAlice::NHollywood::NGeneralConversation
