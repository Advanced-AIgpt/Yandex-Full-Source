#include "constants.h"

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

const TVector<TString> ACTIVATION_PREFIXES = {
    "открой навык", "открой диалог", "запусти навык", "запусти диалог",
    "активируй навык", "включи навык", "вызови навык", "открой чат", "запусти чат с"
};
const TVector<TString> ACTIVATION_PREFIXES_GAMES = {"давай поиграем в", "давай сыграем в"};
const TVector<TString> GAMES_CATEGORIES = {"games_trivia_accessories", "kids"};

const TStringBuf URL = "url";

const TStringBuf Z_TITLE = "title";
const TStringBuf Z_BODY = "body";
const TStringBuf Z_CATEGORY = "z_category";
const TStringBuf Z_DEVELOPER = "z_developer";
const TStringBuf Z_ACTIVATIONS = "z_activations";
const TStringBuf Z_ACTIVATION_PREFIXES = "z_activation_prefixes";
const TStringBuf Z_MANUAL_PHRASES = "z_manual_phrases";
const TStringBuf Z_TOLOKA_POSITIVE = "z_activations_toloka";
const TStringBuf Z_TOLOKA_NEGATIVE = "z_activations_toloka_negative";
const TStringBuf Z_FIRST_MESSAGES = "z_first_messages";
const TStringBuf Z_HELP_MESSAGES = "z_help_messages";
const TStringBuf Z_LOGS_QUERIES = "z_logs_queries";
const TStringBuf Z_LOGS_REPLIES = "z_logs_replies";
const TStringBuf Z_CLICKS_QUERIES = "z_clicks_queries";
const TStringBuf Z_PRE_CLICKS_QUERIES = "z_pre_clicks_queries";
const TStringBuf Z_PRE_ACTIVATION_QUERIES = "z_pre_activation_queries";

const TStringBuf F_IS_RECOMMENDED = "f_is_recommended";
const TStringBuf F_WEEK_NEW_USERS = "f_week_new_users";
const TStringBuf F_MAU = "f_mau";
const TStringBuf F_WAU = "f_wau";
const TStringBuf F_RETURNED = "f_returned";
const TStringBuf F_RATING_AVG = "f_rating_avg";
const TStringBuf F_RATING_COUNT = "f_rating_count";
const TStringBuf F_RETENTION = "f_retention";
const TStringBuf F_RATING_S1 = "f_rating_s1";
const TStringBuf F_RATING_S2 = "f_rating_s2";
const TStringBuf F_RATING_S3 = "f_rating_s3";
const TStringBuf F_RATING_S4 = "f_rating_s4";
const TStringBuf F_RATING_S5 = "f_rating_s5";

const TStringBuf I_IS_RECOMMENDED = "i_is_recommended";
const TStringBuf I_WEEK_NEW_USERS = "i_week_new_users";
const TStringBuf I_MAU = "i_mau";
const TStringBuf I_WAU = "i_wau";
const TStringBuf I_RETURNED = "i_returned";
const TStringBuf I_RATING_S1 = "i_rating_s1";
const TStringBuf I_RATING_S2 = "i_rating_s2";
const TStringBuf I_RATING_S3 = "i_rating_s3";
const TStringBuf I_RATING_S4 = "i_rating_s4";
const TStringBuf I_RATING_S5 = "i_rating_s5";

const TStringBuf P_RATING_AVG = "p_rating_avg";
const TStringBuf P_RATING_COUNT = "p_rating_count";
const TStringBuf P_RETENTION = "p_retention";

