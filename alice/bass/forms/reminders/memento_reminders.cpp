#include "memento_reminders.h"
#include "helpers.h"
#include "reminder.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/library/scenarios/reminders/api.h>
#include <alice/library/scenarios/reminders/memento.h>
#include <alice/library/scenarios/reminders/schedule.h>

#include <alice/bass/libs/client/experimental_flags.h>

#include <alice/library/scenarios/alarm/helpers.h>

// Protobufs.
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>

#include <util/generic/guid.h>

using namespace NAlice;
using namespace NAlice::NRemindersApi;

namespace NBASS::NReminders {
namespace {

// We have hard restrction to 10 same directives, so can schedule only no more than 10 reminders;
// In case of questions ask matrix scheduler responibles for this restriction.
constexpr ssize_t MAX_SCHEDULE_SLOTS_AVAILABLE = 10;

const NMonitoring::TLabels SENSOR_SCHEDULED{{"sensor", "reminder.scheduled"}, {"type", "memento"}};
const NMonitoring::TLabels SENSOR_FAILED{{"sensor", "reminder.failed"}, {"type", "memento"}};


TRemindersApi CreateRemindersApi(const TRequest& req) {
    TRemindersState state = RemindersFromMemento(
                                req.Ctx.Meta().Memento().GetRawValue()->GetString())
                                    .GetOrElse(TRemindersState{});
    TRemindersApi api{req.CurrentTimestamp(), std::move(state)};
    api.RemoveOld();
    return api;
}

class TRemindersBackendMemento : public IRemindersBackend {
public:
    using IRemindersBackend::IRemindersBackend;

protected:
    TMaybe<TRemindersResult> GetRemindersList(bool /* soonestFirst */,
                                              const TDateBounds* dateBounds,
                                              TRemindersListScroll* scroll) override
    {
        auto remindersApi = CreateRemindersApi(Req_);

        const auto status = remindersApi.ProcessList(dateBounds);
        if (!status.IsSuccess()) {
            Req_.SetDefaultError();
            return Nothing();
        }

        const auto& rs = status.Success().State.GetReminders();

        NSc::TValue remindersJson;
        for (const auto& r : rs) {
            NSc::TValue& dst = remindersJson.Push();
            dst["id"] = r.GetId();
            dst["what"] = r.GetText();
            dst["origin"].SetString(Ctx_.MetaClientInfo().DeviceId);
            if (!CreateDateTime(TInstant::Seconds(r.GetShootAt()), Req_.CurrentTime(), Req_.UserTZ, dst)) {
                Req_.SetDefaultError();
                return Nothing();
            }
        }

        if (scroll) {
            scroll->PerPage = rs.size();
        }

        return TRemindersResult{std::move(remindersJson), static_cast<i64>(rs.size())};
    }

    bool TryCancel(const TRemindersListWrapper& listWrapper) override {
        auto& ctx = Req_.Ctx;

        auto remindersApi = CreateRemindersApi(Req_);

        TMaybe<TRemindersApi::TRemoveStatus> status;
        THashSet<TStringBuf> ids;

        if (listWrapper.IsEverything()) {
            status.ConstructInPlace(remindersApi.Clear());
        } else {
            auto onEachId = [&ids](TStringBuf id) {
                ids.emplace(id);
                return true;
            };
            listWrapper.ForEach(onEachId);
            status.ConstructInPlace(remindersApi.Remove(ids));
        }

        if (!status) {
            return false;
        }

        if (!status->IsSuccess()) {
            return false;
        }

        TSchedulerReminderBuilder scheduler{Req_.Uid};
        for (const auto& id : ids) {
            // TODO (petrk) Make batch deletion the same as in creation (same in HW).
            ctx.AddRawServerDirective(scheduler.BuildCancelReminderDirective(TString{id}, ctx.MetaClientInfo().DeviceId));
        }

        TMementoReminderDirectiveBuilder memento;
        ctx.AddRawServerDirective(memento.BuildSaveServerDirective(status->Success().State));

        return true;
    }

    bool IsCancellationSupported() const override {
        return true;
    }

    void SuggestForListing() override {
        // Empty body.
    }
};

} // namespace

bool MementoRemindersCheckIfEnabled(TContext& ctx) {
    if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_MEMENTO_REMINDERS)) {
        return false;
    }

    return ctx.ClientFeatures().SupportsNotifications(); // Prevent working on old devices.
}

void MementoRemindersCreateHandler(TRequest& req, TContext& /*ctx*/) {
    auto& ctx = req.Ctx;
    auto& sensors = ctx.GlobalCtx().Counters().Sensors();

    // TODO (petrk) also can be in separate func but later.
    TakeDayPartIntoAccount(ctx);

    const auto text = req.ObtainRemindText();
    if (!text.Defined()) {
        return;
    }

    const auto shootAt = ProcessShootTimeValue(ctx, req);
    if (!shootAt.Defined()) {
        return;
    }

    const TString id = CreateGuidAsString();
    const TInstant shootAtInstant = NDatetime::Convert(*shootAt, req.UserTZ);

    LOG(INFO) << "Set reminder: '" << id << "', shootAt: " << *shootAt << ", serverTime: " << ctx.ServerTimeMs() << ", clientTime: " << req.CurrentTimestamp() << Endl;

    auto remindersApi = CreateRemindersApi(req);
    if (const auto status = remindersApi.Append(id, *text, shootAtInstant, req.UserTZ.name()); status.IsSuccess()) {
        const TString& myDeviceId = ctx.MetaClientInfo().DeviceId;
        bool myDeviceIsFound = false;
        ssize_t scheduleSlotsAvailable = MAX_SCHEDULE_SLOTS_AVAILABLE;

        TSchedulerReminderBuilder scheduler{req.Uid};
        if (const auto* qdi = req.Ctx.DataSources().FindPtr(static_cast<ui64>(NAlice::EDataSourceType::QUASAR_DEVICES_INFO))) {
            const auto totalDevices = (*qdi)["quasar_devices_info"]["devices"].GetArray().size();

            if (totalDevices > static_cast<size_t>(scheduleSlotsAvailable)) {
                LOG(WARNING) << "User has " << totalDevices << ", but for schedule available " << scheduleSlotsAvailable << Endl;
            }

            for (const auto& device : (*qdi)["quasar_devices_info"]["devices"].GetArray()) {
                const TString deviceId{device["quasar_info"]["device_id"].GetString()};

                LOG(INFO) << "Reminder is scheduled to device: " << deviceId << Endl;
                if (!myDeviceIsFound && ctx.MetaClientInfo().DeviceId == deviceId) {
                    myDeviceIsFound = true;
                }

                ctx.AddRawServerDirective(scheduler.BuildScheduleReminderDirective(status.Success().Reminder, deviceId, myDeviceId));
                --scheduleSlotsAvailable;

                // No more devices available for scheduling.
                if (scheduleSlotsAvailable <= 0) {
                    break;
                }

                // If my device is not found in the list and the only one slot available for schedule.
                // Then break to schedule my device later.
                if (scheduleSlotsAvailable == 1 && !myDeviceIsFound) {
                    break;
                }
            }
        }

        if (!myDeviceIsFound) {
            LOG(WARNING) << "My device is not in the list, reminder is scheduled to this device manually (" << myDeviceId << ")\n";
            ctx.AddRawServerDirective(scheduler.BuildScheduleReminderDirective(status.Success().Reminder, myDeviceId, myDeviceId));
            --scheduleSlotsAvailable;
        }

        sensors.Rate(SENSOR_SCHEDULED)->Add(MAX_SCHEDULE_SLOTS_AVAILABLE - scheduleSlotsAvailable);

        // Update reminders state in memento.
        TMementoReminderDirectiveBuilder memento;
        ctx.AddRawServerDirective(memento.BuildSaveServerDirective(status.Success().State));

        // For normal vins text response.
        // TODO (petrk) Create a separate function.
        auto& responseJson = req.AnswerSlot().Json();
        responseJson["type"].SetString("ok");
        responseJson["id"].SetString(id);

        req.MoveSlotValueToAnswer(SLOT_NAME_TIME, SLOT_TYPE_TIME);
        req.MoveSlotValueToAnswer(SLOT_NAME_DATE, SLOT_TYPE_DATE);
        req.AnswerSlot().Json()[SLOT_NAME_DATE] = NAlice::NScenarios::NAlarm::DateToValue(req.CurrentTime(), *shootAt);
        req.AnswerSlot().Json()[SLOT_NAME_TIME] = NAlice::NScenarios::NAlarm::TimeToValue(*shootAt);

        req.CopySlotValueFromAnswer(SLOT_NAME_WHAT, SLOT_TYPE_WHAT);
        req.CopySlotValueFromAnswer(SLOT_NAME_DATE, SLOT_TYPE_DATE);
        req.CopySlotValueFromAnswer(SLOT_NAME_TIME, SLOT_TYPE_TIME);
    } else {
        sensors.Rate(SENSOR_FAILED)->Inc();
        LOG(ERR) << "TRemindersApi.Append() error: " << status.Error() << Endl;

        switch (status.Error()) {
            case TRemindersApi::EError::ReminderInPast:
                req.SetError(FAIL_PAST_DATETIME);
                break;
            case TRemindersApi::EError::ReminderNotFound:
                break;
        }
        return;
    }

    ctx.AddStopListeningBlock();
}

void MementoRemindersListHandler(TRequest& req, TContext& ctx) {
    TRemindersBackendMemento{req, ctx}.ProcessList();
}

void MementoRemindersCancelHandler(TRequest& req, TContext& ctx) {
    TRemindersBackendMemento{req, ctx}.ProcessCancel();
}

} // namespace NBASS::NReminders
