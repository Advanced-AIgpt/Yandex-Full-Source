//
// Autogenerated file
// This file was created from directives.proto and directives.jinja2 template
//
// Don't edit it manually!
// Please refer to doc: https://docs.yandex-team.ru/alice-scenarios/hollywood/main/codegen
// for more information about custom codegeneration
//

#include "gen_server_directives.pb.h"

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/protos/endpoint/capability.pb.h>
#include <alice/protos/endpoint/common.pb.h>
#include <alice/protos/endpoint/endpoint.pb.h>
#include <alice/protos/endpoint/capabilities/battery/capability.pb.h>
#include <alice/protos/endpoint/capabilities/div_view/capability.pb.h>
#include <alice/protos/endpoint/capabilities/iot_scenarios/capability.pb.h>
#include <alice/protos/endpoint/capabilities/opening_sensor/capability.pb.h>
#include <alice/protos/endpoint/capabilities/route_manager/route_manager.pb.h>
#include <alice/protos/endpoint/capabilities/vibration_sensor/capability.pb.h>
#include <alice/protos/endpoint/capabilities/water_leak_sensor/capability.pb.h>

namespace NAlice::NHollywoodFw {
namespace {

template <class T>
class TCustomServerDirectiveWrapper final : public TServerDirectiveWrapper {
public:
    TCustomServerDirectiveWrapper(T&& directive, T*(NScenarios::TDirective::*fn)(), const TString& name)
        : TServerDirectiveWrapper(name)
        , Directive_(std::move(directive))
        , Fn_(fn)
    {
    }
    void Attach(NScenarios::TScenarioResponseBody& response) override {
        NScenarios::TServerDirective directive;
        T& directive2 = *((directive.*Fn_)());
        directive2.CopyFrom(Directive_);
        *(response.MutableLayout()->AddDirectives()) = std::move(directive);
    };
private:
    T Directive_;
    T*(NScenarios::TDirective::*Fn_)();
};

} // anonymous namespace

/*
    Add all compiled directives to response
*/
void TServerDirectivesWrapper::BuildAnswer(NScenarios::TScenarioResponseBody& response) {
    for (const auto& it : Directives_) {
        it->Attach(response);
    }
}
void TDirectivesWrapper::AddUpdateDatasyncDirective(NAlice::NScenarios::TUpdateDatasyncDirective&& directive) {
    auto p = std::make_shared<TCustomDirectiveWrapper<NAlice::NScenarios::TUpdateDatasyncDirective>>(
        std::move(directive), &NScenarios::TDirective::MutableUpdateDatasyncDirective, "update_datasync_directive");
    Directives_.push_back(p);
}
void TDirectivesWrapper::AddPushMessageDirective(NAlice::NScenarios::TPushMessageDirective&& directive) {
    auto p = std::make_shared<TCustomDirectiveWrapper<NAlice::NScenarios::TPushMessageDirective>>(
        std::move(directive), &NScenarios::TDirective::MutablePushMessageDirective, "push_message_directive");
    Directives_.push_back(p);
}
void TDirectivesWrapper::AddPersonalCardsDirective(NAlice::NScenarios::TPersonalCardsDirective&& directive) {
    auto p = std::make_shared<TCustomDirectiveWrapper<NAlice::NScenarios::TPersonalCardsDirective>>(
        std::move(directive), &NScenarios::TDirective::MutablePersonalCardsDirective, "personal_cards_directive");
    Directives_.push_back(p);
}
void TDirectivesWrapper::AddMementoChangeUserObjectsDirective(NAlice::NScenarios::TMementoChangeUserObjectsDirective&& directive) {
    auto p = std::make_shared<TCustomDirectiveWrapper<NAlice::NScenarios::TMementoChangeUserObjectsDirective>>(
        std::move(directive), &NScenarios::TDirective::MutableMementoChangeUserObjectsDirective, "memento_change_user_objects_directive");
    Directives_.push_back(p);
}
void TDirectivesWrapper::AddSendPushDirective(NAlice::NScenarios::TSendPushDirective&& directive) {
    auto p = std::make_shared<TCustomDirectiveWrapper<NAlice::NScenarios::TSendPushDirective>>(
        std::move(directive), &NScenarios::TDirective::MutableSendPushDirective, "send_push_directive");
    Directives_.push_back(p);
}
void TDirectivesWrapper::AddDeletePushesDirective(NAlice::NScenarios::TDeletePushesDirective&& directive) {
    auto p = std::make_shared<TCustomDirectiveWrapper<NAlice::NScenarios::TDeletePushesDirective>>(
        std::move(directive), &NScenarios::TDirective::MutableDeletePushesDirective, "delete_pushes_directive");
    Directives_.push_back(p);
}
void TDirectivesWrapper::AddPushTypedSemanticFrameDirective(NAlice::NScenarios::TPushTypedSemanticFrameDirective&& directive) {
    auto p = std::make_shared<TCustomDirectiveWrapper<NAlice::NScenarios::TPushTypedSemanticFrameDirective>>(
        std::move(directive), &NScenarios::TDirective::MutablePushTypedSemanticFrameDirective, "push_typed_semantic_frame_directive");
    Directives_.push_back(p);
}
void TDirectivesWrapper::AddAddScheduleActionDirective(NAlice::NScenarios::TAddScheduleActionDirective&& directive) {
    auto p = std::make_shared<TCustomDirectiveWrapper<NAlice::NScenarios::TAddScheduleActionDirective>>(
        std::move(directive), &NScenarios::TDirective::MutableAddScheduleActionDirective, "add_schedule_action_directive");
    Directives_.push_back(p);
}
void TDirectivesWrapper::AddSaveUserAudioDirective(NAlice::NScenarios::TSaveUserAudioDirective&& directive) {
    auto p = std::make_shared<TCustomDirectiveWrapper<NAlice::NScenarios::TSaveUserAudioDirective>>(
        std::move(directive), &NScenarios::TDirective::MutableSaveUserAudioDirective, "save_user_audio_directive");
    Directives_.push_back(p);
}
void TDirectivesWrapper::AddPatchAsrOptionsForNextRequestDirective(NAlice::NScenarios::TPatchAsrOptionsForNextRequestDirective&& directive) {
    auto p = std::make_shared<TCustomDirectiveWrapper<NAlice::NScenarios::TPatchAsrOptionsForNextRequestDirective>>(
        std::move(directive), &NScenarios::TDirective::MutablePatchAsrOptionsForNextRequestDirective, "patch_asr_options_for_next_request_directive");
    Directives_.push_back(p);
}
void TDirectivesWrapper::AddCancelScheduledActionDirective(NAlice::NScenarios::TCancelScheduledActionDirective&& directive) {
    auto p = std::make_shared<TCustomDirectiveWrapper<NAlice::NScenarios::TCancelScheduledActionDirective>>(
        std::move(directive), &NScenarios::TDirective::MutableCancelScheduledActionDirective, "cancel_scheduled_action_directive");
    Directives_.push_back(p);
}
} // namespace NAlice::NHollywoodFw