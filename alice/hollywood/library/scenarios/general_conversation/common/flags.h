#pragma once

#include <util/generic/strbuf.h>

namespace NAlice::NHollywood::NGeneralConversation {

inline constexpr TStringBuf EXP_HW_GC_BERT_RERANKER_COEF_RELEV = "hw_gc_bert_reranker_coef_relev=";
inline constexpr TStringBuf EXP_HW_GC_BERT_RERANKER_COEF_INFORMATIVENESS = "hw_gc_bert_reranker_coef_informativeness=";
inline constexpr TStringBuf EXP_HW_GC_BERT_RERANKER_COEF_SEQ2SEQ = "hw_gc_bert_reranker_coef_seq2seq=";
inline constexpr TStringBuf EXP_HW_GC_BERT_RERANKER_COEF_INTEREST = "hw_gc_bert_reranker_coef_interest=";
inline constexpr TStringBuf EXP_HW_GC_BERT_RERANKER_COEF_NOT_RUDE = "hw_gc_bert_reranker_coef_not_rude=";
inline constexpr TStringBuf EXP_HW_GC_BERT_RERANKER_COEF_NOT_MALE = "hw_gc_bert_reranker_coef_not_male=";
inline constexpr TStringBuf EXP_HW_GC_BERT_RERANKER_COEF_RESPECT = "hw_gc_bert_reranker_coef_respect=";

inline constexpr TStringBuf EXP_HW_GC_OPTIMIZATION_SAMPLE = "hw_gc_optimization_sample=";

inline constexpr TStringBuf EXP_HW_GC_SEQ2SEQ_URL = "hw_gc_seq2seq_url";
inline constexpr TStringBuf EXP_HW_GC_BERT_URL = "hw_gc_bert_url";
inline constexpr TStringBuf EXP_HW_GC_DISABLE_SEQ2SEQ_REPLY = "hw_gc_disable_seq2seq_reply";
inline constexpr TStringBuf EXP_HW_GC_DISABLE_NLGSEARCH_REPLY = "hw_gc_disable_nlgsearch_reply";
inline constexpr TStringBuf EXP_HW_GC_DISABLE_AGGREGATED_REPLY_IN_MODAL_MODE = "hw_gc_disable_aggregated_reply_in_modal_mode";
inline constexpr TStringBuf EXP_HW_GC_DISABLE_EASTER_EGG_HAPPY_BIRTHDAY = "hw_gc_disable_easter_egg";
inline constexpr TStringBuf EXP_HW_GC_EASTER_EGG_SUGGESTS_COUNT_TRIGGER = "hw_gc_easter_egg_suggests_count_trigger=";
inline constexpr TStringBuf EXP_HW_GC_ENABLE_EASTER_EGG_AMANDA = "hw_gc_enable_easter_egg_amanda";
inline constexpr TStringBuf EXP_HW_GC_FIRST_REPLY = "hw_gc_first_reply";
inline constexpr TStringBuf EXP_HW_ENABLE_GC_PROACTIVITY = "hw_gc_proactivity";
inline constexpr TStringBuf EXP_HW_ENABLE_GC_FRAME_PROACTIVITY = "hw_gc_frame_proactivity";
inline constexpr TStringBuf EXP_HW_ENABLE_GC_SOFT_REPEATS_FILTER = "hw_gc_filter_repeats_by_embeddings";
inline constexpr TStringBuf EXP_HW_ENABLE_GC_TEXT_IN_STATE = "hw_gc_save_reply_text";
inline constexpr TStringBuf EXP_HW_ENABLE_GC_RAW_TEXT_UTTERANCE = "hw_gc_raw_text_utterance";
inline constexpr TStringBuf EXP_HW_ENABLE_GC_RAW_VOICE_UTTERANCE = "hw_gc_raw_voice_utterance";
inline constexpr TStringBuf EXP_HW_ENABLE_GC_ENTITY_INDEX = "hw_gc_entity_index";
inline constexpr TStringBuf EXP_HW_DISABLE_GC_FAST_DATA_BANLIST = "hw_gc_fast_data_banlist_disable";
inline constexpr TStringBuf EXP_HW_FORCE_GC_PROACTIVITY_SOFT = "hw_gc_force_proactivity_soft";
inline constexpr TStringBuf EXP_HW_FORCE_GC_ENTITY_SOFT = "hw_gc_force_entity_soft";
inline constexpr TStringBuf EXP_HW_FORCE_GC_ENTITY_SOFT_RNG = "hw_gc_force_entity_soft_rng";
inline constexpr TStringBuf EXP_HW_GC_RANKER_NAME_PREFIX = "hw_gc_ranker_name=";
inline constexpr TStringBuf EXP_HW_GC_RANKER_WEIGHT_PREFIX = "hw_gc_ranker_weight=";
inline constexpr TStringBuf EXP_HW_GC_DEBUG_CANDIDATES_EXCEPTION = "hw_gc_debug_candidates_exception";
inline constexpr TStringBuf EXP_HW_GC_DEBUG_RENDER_TIMEOUT_EXCEPTION = "hw_gc_debug_render_timeout_reply_exception";
inline constexpr TStringBuf EXP_HW_GC_DEBUG_FILTER_ALL_CANDIDATES = "hw_gc_debug_filter_all_candidates";
inline constexpr TStringBuf EXP_HW_GC_DEBUG_PURE_GC_SESSION_TIMEOUT = "hw_gc_debug_pure_gc_session_timeout=";
inline constexpr TStringBuf EXP_HW_GC_DEBUG_SERVER_TIME = "hw_gc_debug_server_time";
inline constexpr TStringBuf EXP_HW_GC_DEBUG_DISABLE_SEARCH_SUGGESTS = "hw_gc_debug_disable_search_suggests";
inline constexpr TStringBuf EXP_HW_GC_MOCKED_REPLY = "hw_gc_mocked_reply";
inline constexpr TStringBuf EXP_HW_GC_ENTITY_DISCUSSION_QUESTION_PROB_PREFIX = "hw_gc_entity_discussion_question_prob=";
inline constexpr TStringBuf EXP_HW_GC_ENTITY_DISCUSSION_QUESTION_SUGGEST = "hw_gc_entity_discussion_question_suggest";
inline constexpr TStringBuf EXP_HW_GC_FORCE_ENTITY_DISCUSSION_SENTIMENT_PREFIX = "hw_gc_force_entity_discussion_sentiment=";
inline constexpr TStringBuf EXP_HW_GC_MOVIE_OPEN_SUGGEST_PROB_PREFIX = "hw_gc_movie_open_suggest_prob=";
inline constexpr TStringBuf EXP_HW_DISABLE_FRAUD_MICROINTENTS = "hw_gc_disable_fraud_microintents";
inline constexpr TStringBuf EXP_HW_ENABLE_MICROINTENTS_OUTSIDE_MODAL = "hw_gc_enable_microintents_outside_modal";
inline constexpr TStringBuf EXP_HW_DISABLE_MICROINTENTS_OUTSIDE_MODAL = "hw_gc_disable_microintents_outside_modal";
inline constexpr TStringBuf EXP_HW_ENABLE_HEAVY_SCENARIO_CLASSIFICATION = "hw_gc_enable_gc_heavy_scanrio_classification";
inline constexpr TStringBuf EXP_HW_DO_NOT_USE_SEARCH_IN_HEAVY = "hw_gc_do_not_use_search_in_heavy";
inline constexpr TStringBuf EXP_HW_ENABLE_BIRTHDAY_MICROINTENTS = "hw_gc_enable_birthday_microintents";
inline constexpr TStringBuf EXP_HW_DISABLE_EMOTIONAL_TTS = "hw_gc_disable_emotional_tts";
inline constexpr TStringBuf EXP_HW_DISABLE_EMOTIONAL_TTS_CLASSIFIER = "hw_gc_disable_emotional_tts_classifier";
inline constexpr TStringBuf EXP_HW_SET_TTS_SPEED = "hw_gc_set_tts_speed";
inline constexpr TStringBuf EXP_HW_SET_CONTEXT_LENGTH = "hw_gc_set_context_length";

inline constexpr TStringBuf EXP_HW_GC_ADD_GIF_TO_ANSWER = "hw_gc_add_gif_to_answer";
inline constexpr TStringBuf EXP_HW_GC_ENABLE_GIF_SHOW = "hw_gc_enable_gif_show";
inline constexpr TStringBuf EXP_HW_GC_ADD_RANDOM_GIF_TO_ANSWER = "hw_gc_add_random_gif_to_answer";
inline constexpr TStringBuf EXP_HW_GC_RANDOM_GIF_PROBABILITY = "hw_gc_random_gif_probability=";
inline constexpr TStringBuf EXP_HW_FORCE_GC_SEARCH_GIF_SOFT = "hw_gc_force_search_gif_soft";
inline constexpr TStringBuf EXP_HW_GC_ADD_SEARCH_GIF_TO_ANSWER = "hw_gc_add_search_gif_to_answer";
inline constexpr TStringBuf EXP_HW_DISABLE_EMOJI_CLASSIFIER = "hw_gc_disable_emoji_classifier";

inline constexpr TStringBuf EXP_HW_GC_SAMPLE_POLICY = "hw_gc_sample_policy";

inline constexpr TStringBuf EXP_HW_DISABLE_GC_PROTOCOL = "hw_gc_protocol_disable";
inline constexpr TStringBuf EXP_HW_DISABLE_GC_MOVIE_DISCUSSIONS = "hw_gc_disable_movie_discussions_by_default";

inline constexpr TStringBuf EXP_HW_GCP_PROACTIVITY_CHECK_YANDEX_PLUS = "hw_gcp_proactivity_check_yandex_plus";
inline constexpr TStringBuf EXP_HW_GCP_PROACTIVITY_SUGGEST_GAME = "hw_gcp_proactivity_suggest_game";
inline constexpr TStringBuf EXP_HW_GCP_PROACTIVITY_SUGGEST_MOVIE = "hw_gcp_proactivity_suggest_movie";
inline constexpr TStringBuf EXP_HW_GCP_PROACTIVITY_MOVIE_AKINATOR = "hw_gcp_proactivity_movie_akinator";
inline constexpr TStringBuf EXP_HW_GCP_PROACTIVITY_MOVIE_DISCUSS = "hw_gcp_proactivity_movie_discuss";
inline constexpr TStringBuf EXP_HW_GCP_PROACTIVITY_MUSIC_DISCUSS = "hw_gcp_proactivity_music_discuss";
inline constexpr TStringBuf EXP_HW_GCP_PROACTIVITY_GAME_DISCUSS = "hw_gcp_proactivity_game_discuss";
inline constexpr TStringBuf EXP_HW_GCP_PROACTIVITY_MOVIE_DISCUSS_SPECIFIC = "hw_gcp_proactivity_movie_discuss_specific";

inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_CONTENT_FOR_CHILD = "hw_gc_proactivity_content_for_child";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS = "hw_gc_proactivity_movie_discuss";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS_SUGGESTS = "hw_gc_proactivity_movie_discuss_suggests";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_GAME_DISCUSS = "hw_gc_proactivity_game_discuss";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_MUSIC_DISCUSS = "hw_gc_proactivity_music_discuss";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS_SPECIFIC = "hw_gc_proactivity_movie_discuss_specific";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_ENTITY_SEARCH = "hw_gc_proactivity_entity_search";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_ENTITY_SEARCH_FIRST = "hw_gc_proactivity_entity_search_first";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_ENTITY_SEARCH_CROP = "hw_gc_proactivity_entity_search_crop=";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_ENTITY_SEARCH_EXP = "hw_gc_proactivity_entity_search_exp=";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_MOVIE_AKINATOR = "hw_gc_proactivity_movie_akinator";

inline constexpr TStringBuf EXP_HW_GC_LETS_DISCUSS_MOVIE_FRAMES = "hw_gc_lets_discuss_movie_frames";

inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_FORCE_SOFT = "hw_gc_proactivity_force_soft=";
inline constexpr TStringBuf EXP_HW_GCP_PROACTIVITY_FORCE_SOFT = "hw_gcp_proactivity_force_soft=";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_TIMEOUT = "hw_gc_proactivity_timeout=";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_FORIDDEN_DIALOG_TURNS_COUNT_LESS = "hw_gc_proactivity_forbidden_dialog_turn_count_less=";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_FORIDDEN_PREV_SCENARIO_TIMEOUT = "hw_gc_proactivity_forbidden_prev_scenario_timeout=";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_FORIDDEN_PREV2_SCENARIO_TIMEOUT = "hw_gc_proactivity_forbidden_prev2_scenario_timeout=";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_FORIDDEN_PREV_SCENARIOS = "hw_gc_proactivity_forbidden_prev_scenarios=";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_FORIDDEN_PREV2_SCENARIOS = "hw_gc_proactivity_forbidden_prev2_scenarios=";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS_PATCH_CONTEXT_INDEX = "hw_gc_proactivity_movie_discuss_patch_context_index=";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS_SPECIFIC_WATCHED_PATCH_CONTEXT_INDEX = "hw_gc_proactivity_movie_discuss_specific_wacthed_patch_context_index=";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS_SPECIFIC_NOT_WATCHED_PATCH_CONTEXT_INDEX = "hw_gc_proactivity_movie_discuss_specific_not_wacthed_patch_context_index=";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_NEXT_REQUEST_PATCH_CONTEXT_INDEX = "hw_gc_proactivity_next_request_patch_context_index=";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS_ENTITY_BOOST = "hw_gc_proactivity_movie_discuss_entity_boost=";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS_SPECIFIC_WATCHED_ENTITY_BOOST = "hw_gc_proactivity_movie_discuss_specific_wacthed_entity_boost=";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS_SPECIFIC_NOT_WATCHED_ENTITY_BOOST = "hw_gc_proactivity_movie_discuss_specific_not_wacthed_entity_boost=";
inline constexpr TStringBuf EXP_HW_GC_PROACTIVITY_NEXT_REQUEST_ENTITY_BOOST = "hw_gc_proactivity_next_request_entity_boost=";
inline constexpr TStringBuf EXP_HW_GC_DISCUSSION_MEMORY = "hw_gc_discussion_memory=";
inline constexpr TStringBuf EXP_HW_GC_ENTITY_SEARCH_CACHE_TIMEOUT = "hw_gc_entity_search_cache_timeout=";

inline constexpr TStringBuf EXP_HW_GC_REPLY_PREFIX = "hw_gc_reply_";
inline constexpr TStringBuf EXP_HW_GC_MODAL_REPLY_PREFIX = "hw_gc_modal_reply_";
inline constexpr TStringBuf EXP_HW_GC_SUGGEST_PREFIX = "hw_gc_suggest_";

inline constexpr TStringBuf EXP_HW_FACTS_CROSSPROMO_TIMEOUT = "hw_facts_crosspromo_timeout=";
inline constexpr TStringBuf EXP_HW_FACTS_CROSSPROMO_DISABLE = "hw_facts_crosspromo_disable";
inline constexpr TStringBuf EXP_HW_FACTS_CROSSPROMO_ENABLE_NON_QUASAR = "hw_facts_crosspromo_non_quasar";
inline constexpr TStringBuf EXP_HW_FACTS_CROSSPROMO_FULL_DICT = "hw_facts_crosspromo_full_dict";
inline constexpr TStringBuf EXP_HW_FACTS_CROSSPROMO_ONLY_CHILDREN = "hw_facts_crosspromo_only_children";
inline constexpr TStringBuf EXP_HW_FACTS_CROSSPROMO_SCENARIO_FILTER_DISABLE = "hw_facts_crosspromo_scenario_filter_disable";
inline constexpr TStringBuf EXP_HW_FACTS_CROSSPROMO_FORBIDDEN_PREV_SCENARIO_TIMEOUT = "hw_crosspromo_forbidden_prev_scenario_timeout=";
inline constexpr TStringBuf EXP_HW_FACTS_CROSSPROMO_CHANGE_QUESTIONS = "hw_facts_crosspromo_change_questions";

inline constexpr TStringBuf EXP_HW_ENABLE_GC_MOVIE_AKINATOR = "hw_gc_movie_akinator";

inline constexpr TStringBuf EXP_HW_GC_DISABLE_CHILD_REPLIES = "hw_gc_disable_child_replies";
inline constexpr TStringBuf EXP_HW_GC_ENABLE_CHILD_SUGGESTS = "hw_gc_enable_child_suggests";

inline constexpr TStringBuf EXP_HW_GC_COMMON_SLOWDOWN_USEC = "hw_gc_common_slowdown_usec=";
inline constexpr TStringBuf EXP_HW_GC_PURE_SLOWDOWN_USEC = "hw_gc_pure_slowdown_usec=";

inline constexpr TStringBuf EXP_HW_GENERATIVE_TALE_USAGE_COUNTER = "hw_generative_tales_memento_usage=";
inline constexpr TStringBuf EXP_HW_GENERATIVE_TALE_HANDLE_SILENCE = "hw_generative_tales_handle_silence";

inline constexpr TStringBuf EXP_HW_GC_ENABLE_GENERATIVE_TOAST = "hw_gc_enable_generative_toast";
inline constexpr TStringBuf EXP_HW_GC_GENERATIVE_TOAST_PROBA = "hw_gc_generative_toast_proba=";

inline constexpr TStringBuf EXP_HW_GC_FORCE_PURE_GC = "hw_gc_force_pure_gc";
inline constexpr TStringBuf EXP_HW_GC_DISABLE_PURE_GC = "hw_gc_disable_pure_gc";
inline constexpr TStringBuf EXP_HW_GC_ENABLE_MODALITY_IN_PURE_GC = "hw_gc_enable_modality_in_pure_gc";

inline constexpr TStringBuf EXP_HW_GC_DISABLE_LET_US_TALK_MICROINTENT = "hw_gc_disable_let_us_talk_microintent";

inline constexpr TStringBuf EXP_HW_GC_DISABLE_BANLIST = "hw_gc_disable_banlist";
inline constexpr TStringBuf EXP_HW_GC_ENABLE_VIOLATION_DETECTION = "hw_gc_enable_violation_detection";

inline constexpr TStringBuf EXP_HW_GC_CLASSIFIER_SCORE_THRESHOLD_PREFIX = "hw_gc_classifier_score_threshold=";
inline constexpr TStringBuf EXP_HW_GC_CLASSIFIER_SCORE_THRESHOLD_SPEAKER_PREFIX = "hw_gc_classifier_score_threshold_speaker=";

inline constexpr TStringBuf EXP_HW_GC_DISABLE_SAMPLE_AND_RANK = "hw_gc_disable_sample_and_rank";

inline constexpr TStringBuf EXP_HW_GC_RENDER_GENERAL_CONVERSATION = "hw_gc_render_general_conversation";

inline constexpr TStringBuf EXP_HW_GC_ARABOBA_BAN_CLF_URL = "hw_gc_araboba_ban_clf_url";
inline constexpr TStringBuf EXP_HW_GC_ARABOBA_BAN_CLF_PHEAD_PATH = "hw_gc_araboba_ban_clf_phead_path";
inline constexpr TStringBuf EXP_HW_GC_ARABOBA_BAN_CLF_THRESHOLD_PREFIX = "hw_gc_araboba_ban_clf_threshold=";
} // namespace NAlice::NHollywood::NGeneralConversation
