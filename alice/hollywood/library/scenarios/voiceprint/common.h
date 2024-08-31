#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice::NHollywood::NVoiceprint {

const TString SCENARIO_NAME = "voiceprint";

inline constexpr TStringBuf NLG_ENROLL = "voiceprint_enroll";
inline constexpr TStringBuf NLG_ENROLL_COLLECT = "voiceprint_enroll__collect_voice";
inline constexpr TStringBuf NLG_ENROLL_FINISH = "voiceprint_enroll__finish";
inline constexpr TStringBuf NLG_ENROLL_MULTIACC_FINISH_SUCCESS = "voiceprint_enroll_multiacc__finish_success";
inline constexpr TStringBuf NLG_IRRELEVANT = "voiceprint_irrelevant";
inline constexpr TStringBuf NLG_REMOVE_CONFIRM = "voiceprint_remove__confirm";
inline constexpr TStringBuf NLG_REMOVE_FINISH = "voiceprint_remove__finish";
inline constexpr TStringBuf NLG_REMOVE_UNKNOWN_USER = "voiceprint_remove__unknown_user";
inline constexpr TStringBuf NLG_SET_MY_NAME = "voiceprint_set_my_name";
inline constexpr TStringBuf NLG_WHAT_IS_MY_NAME = "voiceprint_what_is_my_name";

inline constexpr TStringBuf ENROLL_NEW_GUEST_FRAME = "alice.guest.enrollment.start";
inline constexpr TStringBuf ENROLL_GUEST_FINISH_FRAME = "alice.guest.enrollment.finish";
inline constexpr TStringBuf ENROLL_FRAME = "personal_assistant.scenarios.voiceprint_enroll";
inline constexpr TStringBuf ENROLL_CANCEL_FRAME = "personal_assistant.scenarios.voiceprint_enroll__cancel";
inline constexpr TStringBuf ENROLL_READY_FRAME = "personal_assistant.scenarios.voiceprint_enroll__ready";
inline constexpr TStringBuf ENROLL_START_FRAME = "personal_assistant.scenarios.voiceprint_enroll__start";
inline constexpr TStringBuf ENROLL_COLLECT_FRAME = "personal_assistant.scenarios.voiceprint_enroll__collect_voice";
inline constexpr TStringBuf ENROLL_COLLECT_FRAME_EMULATED = "alice.voiceprint.enrollment.collect__emulated";
inline constexpr TStringBuf ENROLL_FINISH_FRAME = "personal_assistant.scenarios.voiceprint_enroll__finish";

inline constexpr TStringBuf REMOVE_FRAME = "personal_assistant.scenarios.voiceprint_remove";
inline constexpr TStringBuf REMOVE_FINISH_FRAME = "personal_assistant.scenarios.voiceprint_remove__finish";
inline constexpr TStringBuf REMOVE_CANCEL_FRAME_EMULATED = "alice.voiceprint.remove.cancel__emulated";

inline constexpr TStringBuf SET_MY_NAME_FRAME = "personal_assistant.scenarios.set_my_name";

inline constexpr TStringBuf WHAT_IS_MY_NAME_FRAME = "personal_assistant.scenarios.what_is_my_name";

inline constexpr TStringBuf CONFIRM_FRAME = "alice.proactivity.confirm";
inline constexpr TStringBuf REPEAT_FRAME = "personal_assistant.scenarios.repeat";

inline constexpr uint32_t MAX_PHARSES = 5;

// inline constexpr TStringBuf ATTENTION_ENROLL_REQUESTED = "what_is_my_name__enroll_requested"; // TODO(klim-roma): figure out in which case it is used in BASS
inline constexpr TStringBuf ATTENTION_INVALID_REGION = "invalid_region";
inline constexpr TStringBuf ATTENTION_KNOWN_USER = "known_user";
inline constexpr TStringBuf ATTENTION_SERVER_ERROR = "server_error";
inline constexpr TStringBuf ATTENTION_SILENT_ENROLL_MODE = "what_is_my_name__silent_enroll_mode";

inline constexpr TStringBuf NLU_SLOT_IS_SERVER_ERROR = "is_server_error";
inline constexpr TStringBuf NLU_SLOT_IS_TOO_MANY_ENROLLED_USERS = "is_too_many_enrolled_users";
inline constexpr TStringBuf NLU_SLOT_USER_NAME = "user_name";

inline constexpr TStringBuf SLOT_DISTRACTOR = "distractor";
inline constexpr TStringBuf SLOT_IS_ENROLLMENT_SUGGESTED = "is_enrollment_suggested";
inline constexpr TStringBuf SLOT_IS_KNOWN = "is_known";
inline constexpr TStringBuf SLOT_IS_MULTIACCOUNT_ENABLED = "is_multiaccount_enabled";
inline constexpr TStringBuf SLOT_IS_TOO_MANY_ENROLLED_USERS = "is_too_many_enrolled_users";
inline constexpr TStringBuf SLOT_OLD_USER_NAME = "old_user_name";
inline constexpr TStringBuf SLOT_SWEAR_USER_NAME = "is_user_name_swear";
inline constexpr TStringBuf SLOT_USER_NAME = "user_name";

inline constexpr TStringBuf SWEAR_SLOT_TYPE = "sys.swear";

} // namespace NAlice::NHollywood::NVoiceprint
