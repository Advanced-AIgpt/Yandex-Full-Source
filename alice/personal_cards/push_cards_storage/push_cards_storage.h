#pragma once

#include <alice/bass/libs/logging/logger.h>

#include <alice/personal_cards/config/config.pb.h>
#include <alice/personal_cards/protos/model.pb.h>
#include <alice/personal_cards/sensors.h>

#include <infra/libs/sensors/macros.h>

#include <library/cpp/json/json_value.h>

namespace NPersonalCards {

class IPushCardsStorage {
public:
    void AddPushCard(
        const TString& uid,
        const TPushCard& pushCard,
        const TInstant sentDateTime,
        const NJson::TJsonValue& data
    ) {
        try {
            AddPushCardImpl(uid, pushCard, sentDateTime, data);
        } catch (...) {
            ReportFail("add_push_card");
            throw;
        }
    };

    void DismissPushCard(const TString& uid, const TString& cardId) {
        try {
            if ("*" == cardId) { // Ablility to remove all user's cards
                DismissPushCardImpl(uid);
            } else {
                DismissPushCardImpl(uid, cardId);
            }
        } catch(...) {
            ReportFail("dismiss_push_card");
            throw;
        }
    };

    NJson::TJsonArray GetPushCards(const TString& uid, const TInstant now) {
        try {
            return GetPushCardsImpl(uid, now);
        } catch (...) {
            ReportFail("get_push_card");
            throw;
        }
    }

    virtual void UpdateSensors() = 0;

    virtual ~IPushCardsStorage() = default;

protected:
    virtual void AddPushCardImpl(
        const TString& uid,
        const TPushCard& pushCard,
        const TInstant sentDateTime,
        const NJson::TJsonValue& data
    ) = 0;

    virtual void DismissPushCardImpl(const TString& uid) = 0;

    virtual void DismissPushCardImpl(const TString& uid, const TString& cardId) = 0;

    virtual NJson::TJsonArray GetPushCardsImpl(const TString& uid, const TInstant now) = 0;

private:
    void ReportFail(const TString& name) {
        // Sensors
        NInfra::TRateSensor(SENSOR_GROUP, "push_cards_storage_fails", {{"operation", name}}).Inc();

        // Log
        LOG(ERR) << "Push cards storage (" << name << "): '" << CurrentExceptionMessage() << "'" << Endl;
    }
};

using TPushCardsStoragePtr = TAtomicSharedPtr<IPushCardsStorage>;

TPushCardsStoragePtr CreateYDBPushCardsStorage(const TYDBClientConfig& ydbClientConfig);

} // namespace NPersonalCards
