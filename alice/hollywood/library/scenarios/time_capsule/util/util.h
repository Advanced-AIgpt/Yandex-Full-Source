#pragma once

#include <alice/hollywood/library/scenarios/time_capsule/context/context.h>

namespace NAlice::NHollywood::NTimeCapsule {

namespace NNlgTemplateNames {

constexpr TStringBuf TIME_CAPSULE_DELETE = "time_capsule_delete";
constexpr TStringBuf TIME_CAPSULE_HOW_LONG = "time_capsule_how_long";
constexpr TStringBuf TIME_CAPSULE_INFORMATION = "time_capsule_information";
constexpr TStringBuf TIME_CAPSULE_INTERRUPT_APPROVE = "time_capsule_interrupt_approve";
constexpr TStringBuf TIME_CAPSULE_OPEN = "time_capsule_open";
constexpr TStringBuf TIME_CAPSULE_QUESTION = "time_capsule_questions";
constexpr TStringBuf TIME_CAPSULE_RERECORD = "time_capsule_rerecord";
constexpr TStringBuf TIME_CAPSULE_SAVE = "time_capsule_save";
constexpr TStringBuf TIME_CAPSULE_SAVE_APPROVE = "time_capsule_save_approve";
constexpr TStringBuf TIME_CAPSULE_START_APPROVE = "time_capsule_start_approve";
constexpr TStringBuf TIME_CAPSULE_STOP = "time_capsule_stop";

constexpr TStringBuf ERROR = "error";
constexpr TStringBuf SUGGESTS = "suggests";

constexpr TStringBuf TIME_CAPSULE_IMAGE_CARD = "time_capsule_image_card";

constexpr TStringBuf NOT_SUPPORTED = "not_supported";
constexpr TStringBuf NOT_SUPPORTED_DEVICE = "not_supported_device";
constexpr TStringBuf RENDER_RESULT = "render_result";
constexpr TStringBuf MAX_RETRY_COUNT_REACHED = "max_retry_count_reached";
constexpr TStringBuf USER_NOT_AUTHORIZED = "user_not_authorized";
constexpr TStringBuf RECORDING_FINISH = "recording_finish";
constexpr TStringBuf TIME_CAPSULE_RECORDED = "time_capsule_recorded";
constexpr TStringBuf NO_TIME_CAPSULES = "no_time_capsules";

constexpr TStringBuf HOW_TO_DELETE = "how_to_delete";
constexpr TStringBuf HOW_TO_OPEN = "how_to_open";
constexpr TStringBuf HOW_TO_RECORD = "how_to_record";
constexpr TStringBuf HOW_TO_RERECORD = "how_to_rerecord";
constexpr TStringBuf WHAT_IS_IT = "what_is_it";

} // namespace NNlgTemplateNames

namespace NFrameNames {

constexpr TStringBuf TIME_CAPSULE_DELETE = "alice.time_capsule.delete";
constexpr TStringBuf TIME_CAPSULE_HOW_LONG = "alice.time_capsule.how_long";
constexpr TStringBuf TIME_CAPSULE_HOW_TO_DELETE = "alice.time_capsule.how_to_delete";
constexpr TStringBuf TIME_CAPSULE_HOW_TO_OPEN = "alice.time_capsule.how_to_open";
constexpr TStringBuf TIME_CAPSULE_HOW_TO_RECORD = "alice.time_capsule.how_to_record";
constexpr TStringBuf TIME_CAPSULE_HOW_TO_RERECORD = "alice.time_capsule.how_to_rerecord";
constexpr TStringBuf TIME_CAPSULE_FORCE_INTERRUPT = "alice.time_capsule.force_interrupt";
constexpr TStringBuf TIME_CAPSULE_INTERRUPT = "alice.time_capsule.interrupt";
constexpr TStringBuf TIME_CAPSULE_NEXT_STEP = "alice.time_capsule.next_step";
constexpr TStringBuf TIME_CAPSULE_OPEN = "alice.time_capsule.open";
constexpr TStringBuf TIME_CAPSULE_QUESTION = "alice.time_capsule.question";
constexpr TStringBuf TIME_CAPSULE_RESUME = "alice.time_capsule.resume";
constexpr TStringBuf TIME_CAPSULE_RERECORD = "alice.time_capsule.rerecord";
constexpr TStringBuf TIME_CAPSULE_SAVE = "alice.time_capsule.save";
constexpr TStringBuf TIME_CAPSULE_SAVE_APPROVE = "alice.time_capsule.save_approve";
constexpr TStringBuf TIME_CAPSULE_SKIP_QUESTION = "alice.time_capsule.skip_question";
constexpr TStringBuf TIME_CAPSULE_START = "alice.time_capsule.start";
constexpr TStringBuf TIME_CAPSULE_STOP = "alice.time_capsule.stop";
constexpr TStringBuf TIME_CAPSULE_WHAT_IS_IT = "alice.time_capsule.what_is_it";

constexpr TStringBuf TIME_CAPSULE_CONFIRM = "alice.time_capsule.confirm";
constexpr TStringBuf TIME_CAPSULE_DECLINE = "alice.time_capsule.decline";

constexpr TStringBuf TIME_CAPSULE_INTERRUPT_CONFIRM = "alice.time_capsule.interrupt_confirm";
constexpr TStringBuf TIME_CAPSULE_INTERRUPT_DECLINE = "alice.time_capsule.interrupt_decline";

constexpr TStringBuf ALICE_CONFIRM = "alice.proactivity.confirm";
constexpr TStringBuf ALICE_DECLINE = "alice.proactivity.decline";

} // namespace NFrameNames

namespace NScenarioNames {

constexpr TStringBuf TIME_CAPSULE = "time_capsule";

} // namespace NScenarioNames

namespace NExperiments {

constexpr TStringBuf HW_TIME_CAPSULE_ENABLE_RECORD_EXP = "hw_time_capsule_enable_record_exp";
constexpr TStringBuf HW_TIME_CAPSULE_DEMO_MODE_EXP = "hw_time_capsule_demo_mode_exp";
constexpr TStringBuf HW_TIME_CAPSULE_HARDCODE_SESSION_ID_EXP = "hw_time_capsule_hardcode_session_id_exp";

} // NEXperiments

namespace NSuggests {

constexpr TStringBuf TIME_CAPSULE_INTERRUPT = "time_capsule_interrupt";
constexpr TStringBuf TIME_CAPSULE_SKIP_QUESTION = "time_capsule_skip_question";
constexpr TStringBuf TIME_CAPSULE_START = "time_capsule_start";
constexpr TStringBuf TIME_CAPSULE_WHAT_IS_IT = "time_capsule_what_is_it";

}

constexpr TStringBuf ATTENTION_HOW_TO_DELETE = "attention_how_to_delete";
constexpr TStringBuf ATTENTION_TEXT_ANSWER = "attention_text_answer";
constexpr TStringBuf ATTENTION_SKIP_QUESTION = "attention_skip_question";
constexpr TStringBuf ATTENTION_WHAT_IS_IT = "attention_what_is_it";

TString MakeSharedLinkImageUrl(const TString& avatarsId, bool full = false);

} // namespace NAlice::NHollywood::NTimeCapsule
