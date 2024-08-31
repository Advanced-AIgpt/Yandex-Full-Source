#pragma once

#include <alice/bass/forms/common/blackbox_api.h>
#include <alice/bass/forms/common/data_sync_api.h>
#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/vins.h>

#include <alice/bass/util/error.h>

#include <alice/library/passport_api/passport_api.h>

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>

namespace NVoiceprintTesting {
class TVoiceprintEnrollCollectVoiceWrapper;

template <typename THolder>
class TBlackboxDatasyncDependentHandlerWrapper;
} // namespace NVoiceprintTesting

namespace NBASS {

class TContext;

class TVoiceprintEnrollHandler : public IHandler {
public:
    TVoiceprintEnrollHandler(THolder<TBlackBoxAPI> blackBoxAPI, THolder<TDataSyncAPI> dataSyncAPI);

    TResultValue Do(TRequestHandler& r) override {
        return Do(r.Ctx());
    }

private:
    friend class NVoiceprintTesting::TBlackboxDatasyncDependentHandlerWrapper<TVoiceprintEnrollHandler>;

    TResultValue Do(TContext& ctx);
    TResultValue DoImpl(TContext& ctx);

private:
    THolder<TBlackBoxAPI> BlackBoxAPI;
    THolder<TDataSyncAPI> DataSyncAPI;
};

class TVoiceprintEnrollCollectVoiceHandler : public IHandler {
public:
    static const TStringBuf GenderFemale;
    static const TStringBuf GenderMale;
    static const TStringBuf GenderUndefined;

public:
    TVoiceprintEnrollCollectVoiceHandler(THolder<TBlackBoxAPI> blackBoxAPI, THolder<NAlice::TPassportAPI> passportAPI,
                                         THolder<TDataSyncAPI> dataSyncAPI);

    TResultValue Do(TRequestHandler& r) override;

    static TStringBuf DetectGender(TStringBuf phrase);

private:
    friend class NVoiceprintTesting::TVoiceprintEnrollCollectVoiceWrapper;

    struct TUserInfo {
        void ToKeyValues(TVector<TPersonalDataHelper::TKeyValue>& kvs) const;

        TString UserName;
        TString Gender;
        TMaybe<TString> GuestUID;
    };

private:
    TResultValue Do(TContext& ctx);
    TResultValue DoImpl(TContext& ctx);

    TResultValue FreezeUserName(const TSlot* userNameSlot, TContext& ctx, TSlot*& userNameFrozenSlot) const;
    TResultValue Finish(const NSc::TValue& voiceRequests, TUserInfo& info, TContext& ctx);

    void FillFinishForm(TContext& ctx, TStringBuf userName, TMaybe<TString> uid, TMaybe<TString> code) const;
    TResultValue SaveUserInfo(TContext& ctx, TStringBuf uid, const TUserInfo& info) const;

    TMaybe<NAlice::TPassportAPI::TResult> RegisterKolonkish(TContext& ctx, TContext& finish, TStringBuf userName) const;

private:
    THolder<TBlackBoxAPI> BlackBoxAPI;
    THolder<NAlice::TPassportAPI> PassportAPI;
    THolder<TDataSyncAPI> DataSyncAPI;
};

class TWhatIsMyNameHandler : public IHandler {
public:
    explicit TWhatIsMyNameHandler(THolder<TBlackBoxAPI> blackBoxAPI, THolder<TDataSyncAPI> dataSyncAPI);

    TResultValue Do(TRequestHandler& r) override {
        return Do(r.Ctx());
    }

private:
    friend class NVoiceprintTesting::TBlackboxDatasyncDependentHandlerWrapper<TWhatIsMyNameHandler>;

    TResultValue Do(TContext& ctx);
    TResultValue DoImpl(TContext& ctx);

private:
    THolder<TBlackBoxAPI> BlackBoxAPI;
    THolder<TDataSyncAPI> DataSyncAPI;
};

class TSetMyNameHandler : public IHandler {
public:
    TSetMyNameHandler(THolder<TBlackBoxAPI> blackBoxAPI, THolder<TDataSyncAPI> dataSyncAPI);

    TResultValue Do(TRequestHandler& r) override {
        return Do(r.Ctx());
    }

private:
    friend class NVoiceprintTesting::TBlackboxDatasyncDependentHandlerWrapper<TSetMyNameHandler>;

    TResultValue Do(TContext& ctx);
    TResultValue DoImpl(TContext& ctx);

    TResultValue SaveNewUserName(TContext& ctx, TString uid, TString newUserName);

    void SetOldUsername(TContext& ctx, TString oldUserName);

private:
    THolder<TBlackBoxAPI> BlackBoxAPI;
    THolder<TDataSyncAPI> DataSyncAPI;
};

void RegisterVoiceprintEnrollHandlers(THandlersMap& handlers);
} // namespace NBASS
