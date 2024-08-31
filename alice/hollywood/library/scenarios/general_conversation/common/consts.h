#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <util/generic/hash_set.h>
#include <util/generic/vector.h>
#include <util/generic/strbuf.h>

namespace NAlice::NHollywood::NGeneralConversation {

constexpr TStringBuf INTENT_GC_PURE_GC_SESSION_TIMEOUT = "pure_gc_session_timeout";
constexpr TStringBuf INTENT_GC_PURE_GC_SESSION_DISABLED = "pure_gc_session_disabled";
constexpr TStringBuf INTENT_IRRELEVANT = "alice.general_conversation.irrelevant";
constexpr TStringBuf INTENT_IRRELEVANT_LANG = "alice.general_conversation.irrelevant_lang";
constexpr TStringBuf INTENT_MOVIE_QUESTION = "movie_question";
constexpr TStringBuf INTENT_DUMMY = "general_conversation_dummy";
constexpr TStringBuf INTENT_MOVIE_AKINATOR = "movie_akinator";
constexpr TStringBuf INTENT_EASTER_EGG = "easter_egg";
constexpr TStringBuf INTENT_GENERATIVE_TALE = "generative_tale";
constexpr TStringBuf INTENT_GENERATIVE_TALE_BANLIST = "generative_tale.banlist";
constexpr TStringBuf INTENT_GENERATIVE_TALE_BANLIST_CONTINUE = "generative_tale.banlist.continue_tale";
constexpr TStringBuf INTENT_GENERATIVE_TALE_BANLIST_ACTIVATION = "generative_tale.banlist.activation";
constexpr TStringBuf INTENT_GENERATIVE_TALE_CHARACTER = "generative_tale_ask_character";
constexpr TStringBuf INTENT_GENERATIVE_TALE_FORCE_EXIT = "generative_tale.force_exit";
constexpr TStringBuf INTENT_GENERATIVE_TALE_TALE_NAME = "generative_tale.tale_name";
constexpr TStringBuf INTENT_GENERATIVE_TALE_CONFIRM_SHARING = "generative_tale.confirm_sharing";
constexpr TStringBuf INTENT_GENERATIVE_TALE_SEND_ME_MY_TALE = "generative_tale.send_me_my_tale";
constexpr TStringBuf INTENT_GENERATIVE_TALE_STOP = "generative_tale.stop";
constexpr TStringBuf INTENT_GENERATIVE_TOAST = "generative_toast";
constexpr TStringBuf INTENT_GENERATIVE_TOAST_TOPIC = "generative_toast.topic";
constexpr TStringBuf INTENT_GENERATIVE_TOAST_EDITORIAL = "generative_toast.editorial";

constexpr TStringBuf FRAME_BANLIST = "alice.fixlist.gc_request_banlist"; // AcceptedFrames
constexpr TStringBuf FRAME_GAME_SUGGEST = "alice.game_suggest";
constexpr TStringBuf FRAME_GC = "alice.general_conversation.general_conversation";
constexpr TStringBuf FRAME_GC_FEEDBACK = "alice.gc_feedback";
constexpr TStringBuf FRAME_GC_FORCE_EXIT = "alice.general_conversation.force_exit";
constexpr TStringBuf FRAME_PROACTIVITY_AGREE = "alice.general_conversation.proactivity_agree";
constexpr TStringBuf FRAME_PROACTIVITY_ALICE_DO = "alice.general_conversation.proactivity.alice_do"; // AcceptedFrames
constexpr TStringBuf FRAME_PROACTIVITY_BORED = "alice.general_conversation.proactivity.bored"; // AcceptedFrames
constexpr TStringBuf FRAME_PROACTIVITY_DECLINE = "alice.proactivity.decline";
constexpr TStringBuf FRAME_PURE_GC_ACTIVATE = "alice.general_conversation.pure_gc_activate";; // AcceptedFrames
constexpr TStringBuf FRAME_PURE_GC_DEACTIVATE = "alice.general_conversation.pure_gc_deactivate"; // AcceptedFrames
constexpr TStringBuf FRAME_MICROINTENTS = "alice.microintents"; // AcceptedFrames
constexpr TStringBuf FRAME_MOVIE_AKINATOR = "alice.movie_akinator.recommend"; // AcceptedFrames
constexpr TStringBuf FRAME_MOVIE_DISCUSS = "alice.movie_discuss";
constexpr TStringBuf FRAME_GAME_DISCUSS = "alice.game_discuss";
constexpr TStringBuf FRAME_MUSIC_DISCUSS = "alice.music_discuss";
constexpr TStringBuf FRAME_AKINATOR_MOVIE_DISCUSS = "alice.general_conversation.akinator_movie_discuss";
constexpr TStringBuf FRAME_AKINATOR_MOVIE_DISCUSS_WEAK = "alice.general_conversation.akinator_movie_discuss_weak";
constexpr TStringBuf FRAME_EASTER_EGG_SUGGESTS_CLICKER = "alice.easter_egg_suggests_clicker";
constexpr TStringBuf FRAME_GENERATIVE_TALE = "alice.generative_tale.activate";
constexpr TStringBuf FRAME_GENERATIVE_TALE_CHARACTER = "alice.generative_tale.character";
constexpr TStringBuf FRAME_GENERATIVE_TALE_CONFIRM_SHARING = "alice.generative_tale.confirm_sharing";
constexpr TStringBuf FRAME_GENERATIVE_TALE_CONTINUE_GENERATING_TALE = "alice.generative_tale.continue_generating_tale";
constexpr TStringBuf FRAME_GENERATIVE_TALE_SEND_ME_MY_TALE = "alice.generative_tale.send_me_my_tale";
constexpr TStringBuf FRAME_GENERATIVE_TALE_STOP = "alice.generative_tale.stop";
constexpr TStringBuf FRAME_GENERATIVE_TALE_TALE_NAME = "alice.generative_tale.tale_name";
constexpr TStringBuf FRAME_GENERATIVE_TOAST = "alice.generative_toast";

constexpr TStringBuf FRAME_GC_SUGGEST = "alice.general_conversation.suggest";
constexpr TStringBuf SLOT_SUGGEST_TEXT = "suggest_text";

constexpr TStringBuf FRAME_MOVIE_DISCUSS_SPECIFIC = "alice.movie_discuss_specific";
constexpr TStringBuf FRAME_MOVIE_SUGGEST = "alice.movie_suggest";
constexpr TStringBuf FRAME_FACTS_CROSSPROMO = "alice.crosspromo_discuss";
constexpr TStringBuf SLOT_FACTS_CROSSPROMO = "entity_crosspromo";
constexpr TStringBuf FRAME_LETS_DISCUSS_SPECIFIC_MOVIE = "alice.general_conversation.lets_discuss_specific_movie"; // AcceptedFrames
constexpr TStringBuf FRAME_LETS_DISCUSS_SPECIFIC_MOVIE_ELLIPSIS = "alice.general_conversation.lets_discuss_specific_movie_ellipsis";
constexpr TStringBuf FRAME_LETS_DISCUSS_SOME_MOVIE = "alice.general_conversation.lets_discuss_some_movie"; // AcceptedFrames
constexpr TStringBuf FRAME_WHAT_IS_YOUR_FAVORITE_MOVIE = "alice.general_conversation.what_is_your_favorite_movie";
constexpr TStringBuf FRAME_WHAT_IS_YOUR_FAVORITE_MOVIE_WEAK = "alice.general_conversation.what_is_your_favorite_movie_weak";
constexpr TStringBuf FRAME_LETS_DISCUSS_UNRECOGNIZED_MOVIE = "alice.general_conversation.lets_discuss_unrecognized_movie";
constexpr TStringBuf FRAME_YES_I_WATCH_IT = "alice.general_conversation.yes_i_watched_it";
constexpr TStringBuf FRAME_NO_I_DID_NOT_WATCH_IT = "alice.general_conversation.no_i_did_not_watch_it";
constexpr TStringBuf FRAME_I_DONT_KNOW = "alice.general_conversation.i_dont_know";
constexpr TStringBuf FRAME_MOVIE_OPEN = "alice.general_conversation.movie_open";

constexpr TStringBuf FRAME_WIZ_DETECTION_POLITICS = "alice.wiz_detection.shinyserp_politota";
constexpr TStringBuf FRAME_WIZ_DETECTION_UNETHICAL = "alice.wiz_detection.shinyserp_unethical";
constexpr TStringBuf FRAME_WIZ_DETECTION_PORN = "alice.wiz_detection.shinyserp_porno";

const extern TVector<TStringBuf> FRAMES_PROACTIVITY;
const extern TVector<TStringBuf> FRAMES_MOVIE_QUESTION;
const extern TVector<TStringBuf> FRAMES_WITH_ENTITY_SEARCH;
const extern TVector<TStringBuf> FRAMES_WITH_SEARCH_RESPONSE;
const extern TVector<TStringBuf> FRAMES_WITH_SPECIAL_RESPONSE;
const extern TVector<TStringBuf> FRAMES_WITH_SEARCH_SUGGESTS;
const extern TVector<TStringBuf> DEFAULT_GC_FRAMES;
const extern THashSet<TStringBuf> GC_DUMMY_FRAMES;
const extern TVector<TStringBuf> WIZ_DETECTION_FRAMES;

constexpr TStringBuf GENERAL_CONVERSATION_SCENARIO_NAME = "general_conversation";
constexpr TStringBuf GENERAL_CONVERSATION_PROACTIVITY_SCENARIO_NAME = "general_conversation_proactivity";

constexpr TStringBuf NLG_RENDER_RESULT = "render_result";
constexpr TStringBuf NLG_RENDER_GENERIC_STATIC_REPLY = "render_generic_static_reply";
constexpr TStringBuf NLG_RENDER_ERROR = "render_error";
constexpr TStringBuf NLG_RENDER_SUGGEST = "render_suggest";

constexpr TStringBuf STATE_AGGREGATED_REPLIES = "aggregated_replies_state";
constexpr TStringBuf STATE_AGGREGATED_REQUEST = "aggregated_request_state";
constexpr TStringBuf STATE_CLASSIFICATION_RESULT = "classification_result";
constexpr TStringBuf STATE_REPLY_CANDIDATES = "reply_candidates_state";
constexpr TStringBuf STATE_REPLY = "reply_state";
constexpr TStringBuf STATE_SESSION = "session_state";
constexpr TStringBuf STATE_SUGGEST_CANDIDATES = "suggest_candidates_state";
constexpr TStringBuf SOCIAL_SHARING_LINK_CREATE_REQUEST_ITEM = "social_sharing_link_create_candidate_request";
constexpr TStringBuf SOCIAL_SHARING_LINK_CREATE_RESPONSE_ITEM = "social_sharing_link_create_candidate_response";
constexpr TStringBuf SOCIAL_SHARING_LINK_COMMIT_REQUEST_ITEM = "social_sharing_link_commit_candidate_request";
constexpr TStringBuf SOCIAL_SHARING_LINK_COMMIT_RESPONSE_ITEM = "social_sharing_link_commit_candidate_response";

constexpr TStringBuf SOURCE_DUMMY = "dummy";
constexpr TStringBuf SOURCE_ENTITY_GAME = "game_specific";
constexpr TStringBuf SOURCE_ENTITY_MOVIE = "movie_specific";
constexpr TStringBuf SOURCE_ENTITY_MUSIC = "music_specific";
constexpr TStringBuf SOURCE_PROACTIVITY = "proactivity";
constexpr TStringBuf SOURCE_PROACTIVITY_FRAME = "proactivity_frame";
constexpr TStringBuf SOURCE_SEQ2SEQ = "seq2seq";
constexpr TStringBuf SOURCE_TIMEOUT = "timeout";
const extern TVector<TStringBuf> SOURCES_ENTITY;
constexpr TStringBuf DEFAULT_SEQ2SEQ_URL = "/generative_bart";
constexpr TStringBuf GENERATIVE_TALE_URL = "/tales/generative";
constexpr TStringBuf DEFAULT_BERT_URL = "/bert_factor";
constexpr TStringBuf DEFAULT_ARABOBA_URL = "/araboba";
constexpr TStringBuf DEFAULT_ARABOBA_BAN_CLF_URL = "/araboba_ban_clf";

constexpr TStringBuf SUGGEST_TYPE = "gc_suggest";
constexpr TStringBuf SUGGEST_PROACTIVITY_TYPE = "gc_proactivity_suggest";

constexpr TStringBuf FRONTAL_LED_IMAGE_DIRECTIVE_NAME = "draw_led_screen";
constexpr TStringBuf FORCE_DISPLAY_CARDS_DIRECTIVE_NAME = "force_display_cards";

constexpr size_t CONTEXT_LENGTH = 9;
constexpr size_t MAX_CANDIDATES_SIZE = 10;
constexpr size_t MAX_SUGGESTS_SIZE = 10;
constexpr size_t MAX_REPLY_HISTORY_SIZE = 15;
constexpr size_t MAX_FACTS_HISTORY_SIZE = 15;
constexpr ui64 FACTS_CROSSPROMO_TIMEOUT_MS = 60*1000;
constexpr ui64 CROSSPROMO_FORBIDDEN_PREV_SCENARIO_TIMEOUT = 5*60*1000;
constexpr float DEFAULT_GC_CLASSIFIER_SCORE_THRESHOLD = 99999;
constexpr float DEFAULT_GC_CLASSIFIER_SCORE_THRESHOLD_SPEAKER = 0.7;
constexpr int TALE_NUM_HYPOS = 5;
constexpr int MAX_BAD_GEN_REQUESTS = 3;
constexpr ui64 GENERATIVE_TALES_ONBOARDING_COUNT = 2;

const extern THashSet<TStringBuf> KNOWN_MOVIE_CONTENT_TYPES;
const extern THashSet<TStringBuf> CHILD_CONTENT_TYPES;

const extern TStringBuf TALE_BAN_PREFIX;
const extern TStringBuf TALE_INIT_PREFIX;
const extern TVector<TString> CHARACTERS;
const extern TVector<TString> TALE_AVATARS_IDS;
const extern THashMap<TUtf16String, TUtf16String> ACCUSATIVE_FIXLIST;

const TString TALE_ASK_CHARACTER = "Про кого будет сказка?";
const TString TALE_QUESTION_CONTINUE = "Продолжим сочинять";
const TString TALE_QUESTION_POSTFIX = "Что случилось дальше?";
const TString TALE_QUESTION_EXIT = "Хватит";

const TString SILENCE_CALLBACK = "silence_callback";

const size_t TALE_MIN_SIZE = 64;
const TString EOS_CHAR = ".!;";
const extern TVector<TString> POSSESSIVE_PREPOSITIONS;
const extern TVector<TUtf16String> PREPOSITIONS_AND_CONJUNCTIONS;
const extern TVector<TUtf16String> QUESTIONS_WORDS;
const extern TVector<TGenerativeTaleState::EStage> GENERATIVE_TALE_TERMINAL_STAGES;

constexpr TStringBuf DEFAULT_ARABOBA_BAN_CLF_PHEAD_PATH = "";
constexpr float DEFAULT_ARABOBA_BAN_CLF_THRESHOLD = 0.053;
constexpr float ARABOBA_BAN_CLF_MAX_SCORE = 1.;

} // namespace NAlice::NHollywood::NGeneralConversation
