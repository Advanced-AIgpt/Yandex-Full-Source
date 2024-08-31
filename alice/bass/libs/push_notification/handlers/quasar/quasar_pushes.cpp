#include "mm_semantic_frame_push.h"
#include "quasar_pushes.h"
#include "repeat_phrase_push.h"
#include "text_action_push.h"
#include "video_push.h"

#include <alice/bass/libs/push_notification/handlers/handler.h>
#include <alice/bass/libs/logging_v2/logger.h>

namespace NBASS::NPushNotification {

namespace {

template <typename TScheme>
TResultValue ValidateScheme(const TScheme scheme) {
    TStringBuilder errMsg;
    auto onError = [&errMsg](TStringBuf path, TStringBuf msg) {
        if (errMsg) {
            errMsg << TStringBuf("; ");
        }
        errMsg << path << TStringBuf(": ") << msg;
    };

    if (!scheme->Validate({} /* path */, true /* strict */, onError))
        return TError{TError::EType::BADREQUEST, TStringBuilder{} << "JsonSchemeValidation: " << errMsg};
    return ResultSuccess();
}

void AddPush(THandler &handler, const TString& payload, TStringBuf quasarEvent) {
    const int ttl = 360;

    handler.SetInfo(/* event (not used) */ TStringBuf("text_action"),
            /* quasar_event */ quasarEvent,
            /* title (not used) */ TStringBuf{},
            /* body (not used) */ TStringBuf{},
            /* url (not used) */ TStringBuf{},
            /* tag */ TStringBuf("quasar_event"),
            /* ttl */ ttl,
            /* payload */ payload);

    handler.AddCustom("ru.yandex.quasar.app");
}

}  // namespace

TResultValue TQuasarPushes::Generate(THandler& handler, TApiSchemeHolder scheme) {
    if (scheme->Event() == TStringBuf("text_action")) {
        LOG(DEBUG) << "Got quasar text action" << *scheme->GetRawValue();
        const NSc::TValue& serviceData = *scheme->ServiceData();

        if (TResultValue validationResult = ValidateScheme(TQuasarTextActionSchemeHolder{serviceData})) {
            return validationResult;
        }

        AddPush(handler,
                NQuasarTextActionPush::GenerateTextActionPayload(serviceData["text_actions"]),
                TStringBuf("text_action"));

        return ResultSuccess();
    }

    if (scheme->Event() == TStringBuf("play_video")) {
        LOG(DEBUG) << "Got quasar text action" << *scheme->GetRawValue();
        const NSc::TValue& serviceData = *scheme->ServiceData();

        if (TResultValue validationResult = ValidateScheme(TQuasarPlayVideoActionSchemeHolder{serviceData})) {
            return validationResult;
        }

        const TString payload =
                NQuasarVideoPush::GenerateActionPayload(NQuasarVideoPush::QUASAR_PLAY_VIDEO_BY_DESCRIPTOR,
                                                        NQuasarVideoPush::CreateVideoDescriptor(
                                                                serviceData["play_uri"],
                                                                serviceData["provider_name"],
                                                                serviceData["provider_item_id"]));

        AddPush(handler, payload, TStringBuf("server_action"));

        return ResultSuccess();
    }

    if (scheme->Event() == TStringBuf("phrase_action")) {
        LOG(DEBUG) << "Got quasar repeat phrase action " << *scheme->GetRawValue();
        const NSc::TValue& serviceData = *scheme->ServiceData();

        if (TResultValue validationResult = ValidateScheme(TQuasarRepeatPhraseActionSchemeHolder{serviceData})) {
            return validationResult;
        }

        const TString payload = NQuasarRepeatPhraseActionPush::GenerateRepeatPhraseActionPayload(serviceData);
        AddPush(handler, payload, TStringBuf("server_action"));

        return ResultSuccess();
    }

    if (scheme->Event() == TStringBuf("mm_semantic_frame")) {
        LOG(DEBUG) << "Got quasar mm semantic frame action " << *scheme->GetRawValue();
        const NSc::TValue& serviceData = *scheme->ServiceData();

        if (TResultValue validationResult = ValidateScheme(TQuasarMMSemanticFrameActionSchemeHolder{serviceData})) {
            return validationResult;
        }

        const TString payload = NQuasarMMSemanticFrameActionPush::GenerateMMSemanticFrameActionPayload(serviceData);
        AddPush(handler, payload, TStringBuf("server_action"));

        return ResultSuccess();
    }

    return TError{TError::EType::SYSTEM,
                  TStringBuilder{} << "no handler found for '" << scheme->Service() << "' and event '"
                                   << scheme->Event() << '\''
    };
}

}  // namespace NBASS::NPushNotification
