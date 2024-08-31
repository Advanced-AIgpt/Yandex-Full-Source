#include "entry_point.h"

#include "context.h"
#include "result.h"

#include <alice/hollywood/library/scenarios/reminders/proto/callback_payload.pb.h>

#include <alice/hollywood/library/frame/callback.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/push.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/sound/device_volume.h>

#include <alice/library/device_state/device_state.h>
#include <alice/library/frame/directive_builder.h>
#include <alice/library/proto/protobuf.h>
#include <alice/library/scenarios/reminders/api.h>
#include <alice/library/scenarios/reminders/experiments.h>
#include <alice/library/scenarios/reminders/memento.h>
#include <alice/megamind/protos/common/app_type.pb.h>
#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/speechkit/directives.pb.h>
#include <alice/protos/data/scenario/reminders/device_state.pb.h>

#include <library/cpp/timezone_conversion/civil.h>

#include <util/generic/guid.h>
#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>

#include <algorithm>

namespace NAlice::NHollywood::NReminders {

using namespace NAlice::NScenarios;

namespace {

using namespace NAlice::NRemindersApi;
using namespace NAlice::NData::NReminders;

const TString ERROR_INPUT_DATA = "input_data";
const TString ERROR_TSF = "typed_semantic_frame";
const TString ERROR_CALLBACK = "callback";

// TODO (petrk) String for translation.
const TString PUSH_TITLE = "Алиса: напоминание";
// Type from https://a.yandex-team.ru/arc/trunk/arcadia/sup/push/src/main/resources/config/notificator.json
const TString PUSH_TAG = "alice_reminder";

constexpr float ONSHOOT_CHECK_MIN_SOUND_LEVEL = 0.6;
constexpr float ONSHOOT_SET_SOUND_LEVEL = 0.7;

constexpr bool DONT_REMOVE_OLD = false;

TRemindersApi CreateRemindersApi(const TScenarioRunRequestWrapper& request, bool removeOld) {
    TRemindersState state = RemindersFromMemento(request.BaseRequestProto().GetMemento())
                                    .GetOrElse(TRemindersState{});
    TRemindersApi api{TInstant::MilliSeconds(request.ServerTimeMs()), std::move(state)};
    if (removeOld) {
        api.RemoveOld();
    }
    return api;
}

const TString& GetEpochValue(const TStringSlot& slot) {
    if (const TString& value = slot.GetEpochValue(); !value.empty()) {
        return value;
    }
    return slot.GetStringValue();
}

class TRemindersStorage {
public:
    class TReminderHolder : TNonCopyable {
    public:
        TReminderHolder() = default;
        explicit TReminderHolder(TReminderProto&& proto)
            : Own_{std::move(proto)}
            , Proto_{Own_.Get()}
        {
        }

        explicit TReminderHolder(const TReminderProto* proto)
            : Proto_{proto}
        {
        }

        TReminderHolder(TReminderHolder&& proto)
            : Own_{std::move(proto.Own_)}
            , Proto_{proto.Proto_}
        {
            if (Own_) {
                Proto_ = Own_.Get();
            }
        }

        const TReminderProto* operator->() const {
            return Proto_;
        }

        const TReminderProto& operator*() const {
            return *Proto_;
        }

        operator bool () const {
            return !!Proto_;
        }

    private:
        TMaybe<TReminderProto> Own_;
        const TReminderProto* Proto_;
    };

public:
    TRemindersStorage(const TScenarioRunRequestWrapper& request, bool removeOld)
        : Api_{CreateRemindersApi(request, removeOld)}
        , State_{request.BaseRequestProto().GetDeviceState().GetDeviceReminders()}
    {
    }

    TReminderHolder FindReminderById(TStringBuf id, const TDeviceRemindersState::TItem* defaultValue = nullptr) const {
        { // From mememto.
            if (const auto* reminder = Api_.FindReminderById(id); reminder != nullptr) {
                return TReminderHolder{reminder};
            }
        }

        auto createReminderProto = [](const auto& item) {
            TReminderProto reminderProto;
            reminderProto.SetId(item.GetId());
            reminderProto.SetText(item.GetText());
            reminderProto.SetShootAt(FromString<ui64>(item.GetEpoch()));
            reminderProto.SetTimeZone(item.GetTimeZone());
            return TReminderHolder{std::move(reminderProto)};
        };

        // From deviceState.
        const auto& dsList = State_.GetList();
        const auto* reminder = FindIfPtr(dsList.begin(), dsList.end(), [id](const auto& r) { return id == r.GetId(); });
        if (!reminder) {
            if (!defaultValue) {
                return {};
            }
            return createReminderProto(*defaultValue);
        }
        return createReminderProto(*reminder);
    }

    TReminderHolder FindReminderById(const TSemanticFrame& frame, bool useFrameIfNotFound) const {
        const auto& tsf = frame.GetTypedSemanticFrame().GetRemindersOnShootSemanticFrame();
        auto reminder = FindReminderById(tsf.GetId().GetStringValue());
        if (!useFrameIfNotFound || reminder) {
            return reminder;
        }

        TReminderProto reminderProto;
        reminderProto.SetId(tsf.GetId().GetStringValue());
        reminderProto.SetText(tsf.GetText().GetStringValue());
        reminderProto.SetShootAt(FromString<ui64>(GetEpochValue(tsf.GetEpoch())));
        reminderProto.SetTimeZone(tsf.GetTimeZone().GetStringValue());
        return TReminderHolder{std::move(reminderProto)};

    }

    bool HasReminderId(TStringBuf id) const {
        if (Api_.HasReminderId(id)) {
            return true;
        }

        const auto& dsList = State_.GetList();
        const auto it = std::find_if(dsList.begin(), dsList.end(), [&id](const auto& r) { return id == r.GetId(); });
        return it != dsList.end();
    }

private:
    TRemindersApi Api_;
    const TDeviceRemindersState& State_;
};

NJson::TJsonValue CreateNlgDateTime(const TReminderProto& reminder) {
    using namespace NDatetime;

    const auto& tz = GetTimeZone(reminder.GetTimeZone());
    const TCivilSecond time = NDatetime::Convert(TInstant::Seconds(reminder.GetShootAt()), tz);

    NJson::TJsonValue json;

    // For nlg.human_day_rel().
    json["year"] = time.year();
    json["month"] = time.month();
    json["day"] = time.day();
    json["tzinfo"] = reminder.GetTimeZone();

    // For nlg.time_format().
    json["hours"] = time.hour();
    json["minutes"] = time.minute();
    json["seconds"] = time.second();

    return json;
}

THandlerResult::TBackendPtr HandleReminderNotFound(THandlerContext& ctx) {
    return THandlerResult::CreateBackend<TResultBackendNlg>(ctx, "cancelation_result", "not_found", /* rngId */nullptr);
}

THandlerResult::TBackendPtr OnCancelHandler(THandlerContext& ctx, const TSemanticFrame& frame) {
    TDirective directive;
    auto& cancelDirective = *directive.MutableRemindersCancelDirective();

    TRemindersStorage storage{ctx.Request(), DONT_REMOVE_OLD};

    const auto& tsf = frame.GetTypedSemanticFrame();
    if (tsf.HasRemindersOnCancelSemanticFrame()) {
        TStringBuf ids = tsf.GetRemindersOnCancelSemanticFrame().GetIds().GetStringValue();

        TStringBuf id;
        while (ids.NextTok(',', id)) {
            if (!storage.HasReminderId(id)) {
                return HandleReminderNotFound(ctx);
            }
            cancelDirective.AddIds(TString(id));
        }
    }

    if (!cancelDirective.IdsSize()) {
        return THandlerResult::Error(ERROR_INPUT_DATA, TString::Join("unable to find reminder ids in tsf ", ON_CANCEL_FRAME));
    }

    cancelDirective.SetAction("id");

    auto fillCallbackIn = [](TCallbackDirective& callback, bool success) {
        callback.SetName(ON_SUCCESS_CB_NAME);
        TResultPayloadProto proto;
        proto.SetSuccess(success);
        proto.SetType(TResultPayloadProto::Cancelation);
        *callback.MutablePayload() = MessageToStruct(std::move(proto));
    };
    fillCallbackIn(*cancelDirective.MutableOnSuccessCallback(), true);
    fillCallbackIn(*cancelDirective.MutableOnFailCallback(), false);

    auto result = THandlerResult::CreateBackend<TResultBackendAction>();
    result->AddDirective(std::move(directive));
    return result;
}

struct TCallbackDefinition {
    const TString Name;
    const TString Type;
    const bool Success;

    void PutInto(TCallbackDirective& cbd, const google::protobuf::Struct& aux) const {
        cbd.SetName(Name);

        auto& payload = *cbd.MutablePayload()->mutable_fields();

        payload.insert(aux.fields().begin(), aux.fields().end());

        payload["type"].set_string_value(Type);
        payload["success"].set_bool_value(Success);
    }
};

const TCallbackDefinition CREATION_SUCCESS_CALLBACK = { "reminders_on_success_callback", "Creation", true };
const TCallbackDefinition CREATION_FAIL_CALLBACK = { "reminders_on_fail_callback", "Creation", false };

void FillOnShootFrameIn(TRemindersSetDirective& d) {
    auto& tsf = *d.MutableOnShootFrame();

    auto& atm = *tsf.MutableAnalytics();
    atm.SetProductScenario("reminders");
    atm.SetOrigin(TAnalyticsTrackingModule_EOrigin_Scenario);
    atm.SetOriginInfo("hw scenario");
    atm.SetPurpose("on_set");

    auto& onShootTsf = *tsf.MutableTypedSemanticFrame()->MutableRemindersOnShootSemanticFrame();
    onShootTsf.MutableId()->SetStringValue(d.GetId());
    onShootTsf.MutableText()->SetStringValue(d.GetText());
    onShootTsf.MutableEpoch()->SetStringValue(d.GetEpoch());
    onShootTsf.MutableTimeZone()->SetStringValue(d.GetTimeZone());
}

THandlerResult::TBackendPtr OnCreateHandler(THandlerContext& /* ctx */, const TCallbackDirective& cbd) {
    // TODO (petrk) Add checking for android 12 after this scenario is moved from bass to hw.

    // Right now this handler is used to create new reminder as OnSuccess handler after permission is granted.
    const auto reminder = StructToMessage<TReminderProto>(cbd.GetPayload());

    TDirective directive;
    auto& setDirective = *directive.MutableRemindersSetDirective();
    // FIXME (petrk) Add checks.
    setDirective.SetId(reminder.GetId());
    setDirective.SetText(reminder.GetText());
    setDirective.SetEpoch(ToString(reminder.GetShootAt()));
    setDirective.SetTimeZone(reminder.GetTimeZone());

    FillOnShootFrameIn(setDirective);

    TProtoStructBuilder auxBuilder;
    auxBuilder.Set("action", "id");
    auxBuilder.Set("reminders", TProtoListBuilder{}
                                    .Add(TProtoStructBuilder{}
                                            .Set("id", reminder.GetId())
                                            .Build()).Build());
    const auto aux = auxBuilder.Build();
    CREATION_SUCCESS_CALLBACK.PutInto(*setDirective.MutableOnSuccessCallback(), aux);
    CREATION_FAIL_CALLBACK.PutInto(*setDirective.MutableOnFailCallback(), aux);

    auto result = THandlerResult::CreateBackend<TResultBackendAction>();
    result->AddDirective(std::move(directive));
    return result;
}

THandlerResult::TBackendPtr OnPermissionFailHandler(THandlerContext& ctx, const TCallbackDirective& ) {
    return THandlerResult::CreateBackend<TResultBackendNlg>(ctx, "permission", "fail", /* rngId */nullptr);
}

THandlerResult::TBackendPtr OnResultHandler(THandlerContext& ctx, const TCallbackDirective& directive) {
    const auto payload = StructToMessage<TResultPayloadProto>(directive.GetPayload());
    const TRemindersStorage remindersStorage{ctx.Request(), DONT_REMOVE_OLD};
    const auto templateName = TString::Join(
        to_lower(TResultPayloadProto_EType_Name(payload.GetType())),
        "_result");

    if (!payload.GetSuccess() && payload.GetType() == TResultPayloadProto::Cancelation) {
        TString id;
        if (payload.GetIds().size() > 0) {
            id = payload.GetIds()[0];
        } else if (payload.GetReminders().size() > 0) {
            id = payload.GetReminders()[0].GetId();
        }

        if (!id.empty() && !remindersStorage.HasReminderId(id)) {
            return HandleReminderNotFound(ctx);
        }
    }

    auto result = THandlerResult::CreateBackend<TResultBackendNlg>(ctx, templateName, payload.GetSuccess() ? "success" : "fail", /* rngId */nullptr);

    auto& nlgCtx = result->NlgData().Context;
    if (payload.GetAction() == "all") {
        nlgCtx["everything"] = true;
    }

    if (payload.RemindersSize() > 0) {
        for (const auto& r : payload.GetReminders()) {
            const TString& id = r.GetId();
            if (id.Empty()) {
                LOG_WARN(ctx->Ctx.Logger()) << "OnResult: " << TResultPayloadProto_EType_Name(payload.GetType())
                                            << ", got reminders with empty reminder id. "
                                            << directive.ShortUtf8DebugString();
                continue;
            }

            auto reminder = remindersStorage.FindReminderById(id, &r);
            if (!reminder) {
                LOG_INFO(ctx->Ctx.Logger()) << "OnResult: " << TResultPayloadProto_EType_Name(payload.GetType())
                                            << ", got reminder (" << id << ") but not found in storage.";
                continue;
            }

            NJson::TJsonValue item;
            item["text"] = reminder->GetText();
            item["datetime"] = CreateNlgDateTime(*reminder);
            nlgCtx["reminders"].AppendValue(item);
        }
    }

    // Adding voice button named 'cancel'.
    if (payload.GetType() == TResultPayloadProto::Creation) {
        if (payload.GetSuccess()) {
            TFrameAction action;
            auto& frameNluHint = *action.MutableNluHint();
            frameNluHint.SetFrameName(ON_CANCEL_FRAME);

            auto& parsedUtterance = *action.MutableParsedUtterance();
            auto& frame = *parsedUtterance.MutableTypedSemanticFrame()->MutableRemindersOnCancelSemanticFrame();

            TStringBuilder ids;
            for (const auto& reminder : payload.GetReminders()) {
                if (!ids.empty()) {
                    ids << ',';
                }
                ids << reminder.GetId();
            }
            frame.MutableIds()->SetStringValue(ids);

            result->AddFrameAction(ON_CANCEL_FRAME, std::move(action));
        }
    }

    return result;
}

void RedirectPush(THandlerContext& ctx, const TSemanticFrame& frame, TResultBackendNlg& result) {
    if (!ctx.Request().ClientInfo().IsSmartSpeaker()
        || ctx.Request().HasExpFlag(NRemindersApi::EXPFLAG_REMINDERS_DISABLE_REDIRECT_PUSH))
    {
        return;
    }

    const auto& tsf = frame.GetTypedSemanticFrame();
    const auto& onShootTsf = tsf.GetRemindersOnShootSemanticFrame();

    const TString& originDeviceId = onShootTsf.GetOriginDeviceId().GetStringValue();
    if (!originDeviceId.empty() && originDeviceId != ctx.Request().ClientInfo().DeviceId) {
        return;
    }

    TPushDirectiveBuilder pushDirectiveBuilder{
        PUSH_TITLE,
        tsf.GetRemindersOnShootSemanticFrame().GetText().GetStringValue(),
        TTypedSemanticFrameDirectiveBuilder{}
            .SetScenarioAnalyticsInfo("reminders", "on_shoot", "redirected_push")
            .SetTypedSemanticFrame(tsf)
            .BuildDialogUrl(),
        PUSH_TAG
    };
    pushDirectiveBuilder.SetAnalyticsAction(
        TString::Join("send_push_", PUSH_TAG),
        TString::Join("send push " + PUSH_TAG),
        "Отправляется напоминание пользователю на прилолжение Яндекс");

    result.AddPushDirective(pushDirectiveBuilder);
}

// XXX (petrk) This a temporary fix until https://st.yandex-team.ru/SK-6183 is done.
// We understand the problems with this approach but decided to do it!
void VolumeUpAndRestore(THandlerContext& ctx, TResultBackendNlg& result) {
    // If something is playing skip raising the volume.
    if (GetCurrentlyPlaying(ctx.Request().BaseRequestProto().GetDeviceState()) != ECurrentlyPlaying::Nothing) {
        return;
    }

    const auto deviceVolume = NSound::TDeviceVolume::BuildFromState(ctx.Request().BaseRequestProto().GetDeviceState());
    const auto checkLevel = deviceVolume.AbsoluteLevelPercent(ONSHOOT_CHECK_MIN_SOUND_LEVEL);
    if (checkLevel <= deviceVolume.GetCurrent()) {
        return;
    }

    TDirective dVolUp;
    dVolUp.MutableSoundSetLevelDirective()->SetNewLevel(deviceVolume.AbsoluteLevelPercent(ONSHOOT_SET_SOUND_LEVEL));

    TDirective dTtsPlaceHolder;
    dTtsPlaceHolder.MutableTtsPlayPlaceholderDirective()->SetName("tts_reminder_on_shoot_placeholder");

    TDirective dVolResotre;
    dVolResotre.MutableSoundSetLevelDirective()->SetNewLevel(deviceVolume.GetCurrent());

    result.AddDirective(std::move(dVolUp));
    result.AddDirective(std::move(dTtsPlaceHolder));
    result.AddDirective(std::move(dVolResotre));
}

THandlerResult::TBackendPtr OnShootHandler(THandlerContext& ctx, const TSemanticFrame& frame) {
    if (!frame.GetTypedSemanticFrame().HasRemindersOnShootSemanticFrame()) {
        return THandlerResult::Error(ERROR_TSF, TString::Join("invalid semantic frame type for ", ON_SHOOT_FRAME));
    }

    const auto remindersStorage = TRemindersStorage(ctx.Request(), DONT_REMOVE_OLD);

    // TODO (petrk) remove it when uniproxy rollout new transparent directive.
    const bool useFrameIfNotFound = ctx.Request().Interfaces().GetHasNotifications() == false;
    const auto reminder = remindersStorage.FindReminderById(frame, useFrameIfNotFound);
    if (!reminder) {
        // Nothing to do.
        LOG_INFO(ctx->Ctx.Logger()) << "OnShoot: reminder not found: " << frame.ShortUtf8DebugString();
        return THandlerResult::CreateBackend<TResultBackendAction>();
    }

    auto result = THandlerResult::CreateBackend<TResultBackendNlg>(ctx, "shoot", "render_result", &reminder->GetId());
    auto& nlgCtx = result->NlgData().Context;
    nlgCtx["remind_text"] = reminder->GetText();
    nlgCtx["remind_datetime"] = CreateNlgDateTime(*reminder);

    VolumeUpAndRestore(ctx, *result);

    RedirectPush(ctx, frame, *result);

    return result;
}

using TCallbackHandler = std::function<THandlerResult::TBackendPtr(THandlerContext&, const TCallbackDirective&)>;
const THashMap<TStringBuf, TCallbackHandler> CallbackHandlers{
    { ON_SUCCESS_CB_NAME, OnResultHandler },
    { ON_FAIL_CB_NAME, OnResultHandler },
    { ON_PERMISSION_FAIL_CB_NAME, OnPermissionFailHandler },
    { ON_PERMISSION_SUCCESS_CB_NAME, OnCreateHandler },
};

using TSemanticFrameHandler = std::function<THandlerResult::TBackendPtr(THandlerContext&, const TSemanticFrame&)>;
const THashMap<TStringBuf, TSemanticFrameHandler> FrameHandlers{
    { ON_SHOOT_FRAME, OnShootHandler },
    { ON_CANCEL_FRAME, OnCancelHandler },
};

} // namespace

// TRemindersEntryPointHandler ------------------------------------------------
TString TRemindersEntryPointHandler::Name() const {
    static const TString name{"main"};
    return name;
}

void TRemindersEntryPointHandler::Do(TScenarioHandleContext& ctx) const {
    auto render = [&ctx](THandlerResult::TBackendPtr result) {
        ctx.ServiceCtx.AddProtobufItem(*result->CreateResponse(), RESPONSE_ITEM);
    };

    THandlerContext handlerCtx{ctx};

    const auto& input = handlerCtx.Request().Input();

    if (const auto* callback = input.GetCallback()) {
        if (const auto* handler = CallbackHandlers.FindPtr(callback->GetName())) {
            return render((*handler)(handlerCtx, *callback));
        }

        return render(THandlerResult::Error(
                ERROR_CALLBACK,
                TString::Join("no handler for callback '", callback->GetName(), "' found")));
    }

    for (const auto& frame : input.Proto().GetSemanticFrames()) {
        if (const auto* handler = FrameHandlers.FindPtr(frame.GetName())) {
            return render((*handler)(handlerCtx, frame));
        }
    }

    render(THandlerResult::Irrelevant());
}

const TVector<TStringBuf>& TRemindersEntryPointHandler::GetSupportedFrames() {
    static const auto supportedFrameNames = []() {
        auto result = TVector<TStringBuf>(Reserve(FrameHandlers.size()));
        for (const auto& [key, value] : FrameHandlers) {
            result.emplace_back(key);
        }
        return result;
    }();
    return supportedFrameNames;
}

bool TRemindersEntryPointHandler::IsCallbackSupported(const TStringBuf callbackName) {
    return CallbackHandlers.contains(callbackName);
}

} // namespace NAlice::NHollywood::NReminders
