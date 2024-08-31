#include <alice/cachalot/library/modules/activation/request.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/utils.h>

#include <alice/cachalot/events/cachalot.ev.pb.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/protos/asr.pb.h>

#include <google/protobuf/timestamp.pb.h>

#include <library/cpp/protobuf/interop/cast.h>


namespace {

    constexpr TStringBuf ITEM_TYPE_ANNOUNCEMENT_REQUEST = "activation_announcement_request";
    constexpr TStringBuf ITEM_TYPE_ANNOUNCEMENT_RESPONSE = "activation_announcement_response";

    constexpr TStringBuf ITEM_TYPE_FINAL_REQUEST = "activation_final_request";
    constexpr TStringBuf ITEM_TYPE_FINAL_RESPONSE = "activation_final_response";

    constexpr TStringBuf ITEM_TYPE_ASR_SPOTTER_VALIDATION = "asr_spotter_validation";
    constexpr TStringBuf ITEM_TYPE_MM_RUN_READY = "mm_run_ready";
    constexpr TStringBuf ITEM_TYPE_ACTIVATION_SUCCESSFUL = "activation_successful";


    void SetCompetitorInfo(
        ::NCachalotProtocol::TActivationSubjectInfo* info,
        const NCachalot::TActivationStorageKey& key,
        const NCachalot::TActivationStorageData& data
    ) {
        info->SetUserId(key.UserId);
        info->SetDeviceId(data.DeviceId);
        info->MutableActivationAttemptTime()->CopyFrom(NProtoInterop::CastToProto(data.ActivationAttemptTime));
        info->MutableSpotterFeatures()->SetAvgRMS(data.SpotterFeatures.AvgRMS);
        info->MutableSpotterFeatures()->SetValidated(data.SpotterFeatures.Validated);
    }

    double GetPatchedAvgRms(const ::NCachalotProtocol::TActivationSubjectInfo& info) {
        // VOICESERV-4137

        static constexpr char YandexMicroFirstSymbol = 'L';
        static constexpr size_t YandexStationDeviceIdLength = 20;

        const double baseAvgRms = info.GetSpotterFeatures().GetAvgRMS();

        if (const TString& deviceId = info.GetDeviceId(); 0 < deviceId.size()) {
            if (deviceId[0] == YandexMicroFirstSymbol) {
                // yandexmicro
                return baseAvgRms * 3;
            }
            if (deviceId.size() == YandexStationDeviceIdLength) {
                // yandestation (first)
                return baseAvgRms * 1.2;
            }
        }

        return baseAvgRms;
    }

    auto ExtractSubjectInfo(const ::NCachalotProtocol::TActivationSubjectInfo& info) {
        NCachalot::TActivationStorageKey requestKey;
        requestKey.UserId = info.GetUserId();

        NCachalot::TActivationStorageData requestData;
        requestData.DeviceId = info.GetDeviceId();
        requestData.ActivationAttemptTime = NProtoInterop::CastFromProto(info.GetActivationAttemptTime());
        requestData.SpotterFeatures.AvgRMS = GetPatchedAvgRms(info);
        requestData.SpotterFeatures.Validated = info.GetSpotterFeatures().GetValidated();

        return std::make_tuple(requestKey, requestData);
    }

    template <typename TProtoReq>
    void ParseRequestOptions(const TProtoReq& protoReq, NCachalot::TActivationStorageRequestOptions* options) {
        if (protoReq.HasFreshnessDeltaMilliSeconds()) {
            options->FreshnessManager = {
                .FreshnessDeltaMilliSeconds = protoReq.GetFreshnessDeltaMilliSeconds()
            };
        }

        if constexpr (std::is_same_v<TProtoReq, NCachalotProtocol::TActivationFinalRequest>) {
            if (protoReq.HasIgnoreRms()) {
                options->Flags.IgnoreRms = protoReq.GetIgnoreRms();
            }
        }
    }


}  // anonymous namespace


namespace NCachalot {


TRequestActivationBase::TRequestActivationBase(
    NAppHost::TServiceContextPtr ctx,
    TIntrusivePtr<IActivationStorage> storage,
    TActivationServiceConfig config
)
    : TRequest(std::move(ctx), &TMetrics::GetInstance().ActivationMetrics.RequestMetrics)
    , Storage(std::move(storage))
    , ActivationAlgorithmMetrics(&TMetrics::GetInstance().ActivationMetrics.AlgorithmMetrics)
    , ServiceConfig(std::move(config))
{
}

TRequestActivationBase::TRequestActivationBase(
    const NNeh::IRequestRef& req,
    TIntrusivePtr<IActivationStorage> storage,
    TActivationServiceConfig config
)
    : TRequest(req, &TMetrics::GetInstance().ActivationMetrics.RequestMetrics)
    , Storage(std::move(storage))
    , ActivationAlgorithmMetrics(&TMetrics::GetInstance().ActivationMetrics.AlgorithmMetrics)
    , ServiceConfig(std::move(config))
{
}


TAsyncStatus TRequestActivationAnnouncement::ServeAsync() {
    DLOG("[ActivationAnnouncement] started");

    RequestMetrics->OnServeStarted(ArrivalTime);
    Response.MutableStats()->SetWaitingTime(MillisecondsSince(ArrivalTime));
    ActivationAlgorithmMetrics->OnAnnouncement();

    if (!Request.HasActivationAnnouncement()) {
        SetError(EResponseStatus::BAD_REQUEST, "Invalid proto for handle activation_announcement");
        return NThreading::MakeFuture(TStatus(Status));
    }

    const auto& protoReq = Request.GetActivationAnnouncement();
    const auto& [requestKey, requestData] = ExtractSubjectInfo(protoReq.GetInfo());
    NThreading::TPromise<TStatus> status = NThreading::NewPromise<TStatus>();

    DLOG("[ActivationAnnouncement] makes announcement for " << requestKey.UserId << ' ' << requestData.DeviceId);
    LogFrame->Log().LogEventInfoCombo<NEvClass::ActivationAnnouncementStarted>(requestKey.UserId, requestData.DeviceId,
        requestData.SpotterFeatures.Validated, requestData.SpotterFeatures.AvgRMS);

    if (requestData.SpotterFeatures.Validated) {
        TSpotterValidationDevice svd;
        svd.SetCertain(requestData.DeviceId);
        AnaLog.UpdateSpotterValidatedBy(svd);
    }

    ParseRequestOptions(protoReq, &Options);

    Storage->MakeAnnouncement(requestKey, requestData, Options, LogFrame).Subscribe(
        [
            this, This=IntrusiveThis(),
            requestKey=requestKey, requestData=requestData, status
        ](const auto& reply) mutable {
            DLOG("[ActivationAnnouncement] processes ydbResponse for " << requestKey.UserId << ' ' <<
                 requestData.DeviceId);

            const TActivationYdbStorage::TSingleRowResponse& ydbResponse = reply.GetValueSync();

            AddBackendStats(Storage->GetStorageTag(), ydbResponse.Status, ydbResponse.Stats);
            ::NCachalotProtocol::TActivationAnnouncementResponse *rspPtr = Response.MutableActivationAnnouncement();
            rspPtr->SetZeroRmsFound(ydbResponse.ExtraData.RecordWithZeroRmsFound);
            rspPtr->SetLeaderFound(ydbResponse.ExtraData.LeaderFound);
            AnaLog.UpdateSpotterValidatedBy(ydbResponse.ExtraData.SpotterValidatedBy);

            if (!ydbResponse) {
                // Although there was an error, this spotter should try to make final request,
                // so we log error but allow to continue.
                LogFrame->Log().LogEventErrorCombo<NEvClass::ActivationAnnouncementError>(ydbResponse.Stats.ErrorMessage);
                ActivationAlgorithmMetrics->OnFailedAnnouncementCommit(ArrivalTime);
                rspPtr->SetContinuationAllowed(true);
                rspPtr->SetError(ydbResponse.Stats.ErrorMessage);
                DLOG("[ActivationAnnouncement] Storage failure! Error: " << ydbResponse.Stats.ErrorMessage);
                DLOG("[ActivationAnnouncement] Allowed beacuse of storage failure " << requestKey.UserId << ' ' <<
                    requestData.DeviceId << ' ' << requestData.SpotterFeatures.Validated);
            } else if (!ydbResponse.Data.Defined()) {
                // Better announcement was not found.
                // We allow this spotter to continue.
                LogFrame->Log().LogEventInfoCombo<NEvClass::ActivationAnnouncementBetterRecordWasNotFound>();
                ActivationAlgorithmMetrics->OnSuccessfulAnnouncementCommit(ArrivalTime);
                rspPtr->SetContinuationAllowed(true);
                DLOG("[ActivationAnnouncement] Allowed " << requestKey.UserId << ' ' << requestData.DeviceId << ' ' <<
                     requestData.SpotterFeatures.Validated);
            } else {
                const TActivationStorageData& bestData = ydbResponse.Data.GetRef();
                SetCompetitorInfo(rspPtr->MutableBestCompetitor(), requestKey, bestData);

                if (
                    ydbResponse.ExtraData.LeaderFound ||
                    (
                        false &&  // TODO (paxakor): seems like we are not able to reject by not-leader on announcement
                        bestData.DeviceId != requestData.DeviceId &&
                        requestData.Compare(bestData, /* requireValidation = */ true)
                    )
                ) {
                    rspPtr->SetContinuationAllowed(false);
                    ActivationAlgorithmMetrics->OnWorthlessAnnouncementCommit(ArrivalTime);
                    DLOG("[ActivationAnnouncement] Rejected " << requestKey.UserId << ' ' <<
                         requestData.DeviceId << ' ' << requestData.SpotterFeatures.Validated);
                    LogFrame->Log().LogEventInfoCombo<NEvClass::ActivationAnnouncementRejected>(bestData.DeviceId);
                } else {
                    rspPtr->SetContinuationAllowed(true);
                    ActivationAlgorithmMetrics->OnSuccessfulAnnouncementCommit(ArrivalTime);
                    DLOG("[ActivationAnnouncement] Allowed " << requestKey.UserId << ' ' <<
                         requestData.DeviceId << ' ' << requestData.SpotterFeatures.Validated);
                    LogFrame->Log().LogEventInfoCombo<NEvClass::ActivationAnnouncementAllowed>(bestData.DeviceId);
                }
            }
            status.SetValue(EResponseStatus::OK);
        }
    );

    SetStatus(EResponseStatus::OK);

    DLOG("[ActivationAnnouncement] returns for " << requestKey.UserId << ' ' << requestData.DeviceId);

    return status;
}


TRequestVoiceInputActivationFirstAnnouncement::TRequestVoiceInputActivationFirstAnnouncement(
    NAppHost::TServiceContextPtr ctx,
    TIntrusivePtr<IActivationStorage> storage,
    TActivationServiceConfig config
)
    : TRequestActivationAnnouncement(ctx, std::move(storage), std::move(config))
{
    if (!ctx->HasProtobufItem(ITEM_TYPE_ANNOUNCEMENT_REQUEST)) {
        SetError(EResponseStatus::BAD_REQUEST, "Bad announcement-1 request: no request item");
        RequestMetrics->OnBadData(ArrivalTime);
    } else {
        ctx->GetOnlyProtobufItem<NCachalotProtocol::TActivationAnnouncementRequest>(
            ITEM_TYPE_ANNOUNCEMENT_REQUEST
        ).Swap(Request.MutableActivationAnnouncement());
    }
}

void TRequestVoiceInputActivationFirstAnnouncement::ReplyToApphostContextOnSuccess(
    NAppHost::TServiceContextPtr ctx
) {
    // Forwarding original request to the second stage.
    ctx->AddProtobufItem(Request.GetActivationAnnouncement(), ITEM_TYPE_ANNOUNCEMENT_REQUEST);
}

void TRequestVoiceInputActivationFirstAnnouncement::ReplyToApphostContextOnError(
    NAppHost::TServiceContextPtr ctx
) {
    // In case of any internal or ydb error behave as if the first stage was successful.
    if (Request.HasActivationAnnouncement()) {
        ctx->AddProtobufItem(Request.GetActivationAnnouncement(), ITEM_TYPE_ANNOUNCEMENT_REQUEST);
    }
}


TRequestVoiceInputActivationSecondAnnouncement::TRequestVoiceInputActivationSecondAnnouncement(
    NAppHost::TServiceContextPtr ctx,
    TIntrusivePtr<IActivationStorage> storage,
    TActivationServiceConfig config
)
    : TRequestActivationAnnouncement(ctx, std::move(storage), std::move(config))
{
    if (!ctx->HasProtobufItem(ITEM_TYPE_ANNOUNCEMENT_REQUEST)) {
        SetError(EResponseStatus::BAD_REQUEST, "Bad announcement-2 request: no request item");
        RequestMetrics->OnBadData(ArrivalTime);
    } else if (!ctx->HasProtobufItem(ITEM_TYPE_ASR_SPOTTER_VALIDATION)) {
        SetError(EResponseStatus::BAD_REQUEST, "Bad announcement-2 request: no spotter item");
        RequestMetrics->OnBadData(ArrivalTime);
    } else {
        NCachalotProtocol::TActivationAnnouncementRequest* req = Request.MutableActivationAnnouncement();

        ctx->GetOnlyProtobufItem<NCachalotProtocol::TActivationAnnouncementRequest>(
            ITEM_TYPE_ANNOUNCEMENT_REQUEST
        ).Swap(req);

        const auto spotterRsp = ctx->GetOnlyProtobufItem<NAliceProtocol::TSpotterValidation>(
            ITEM_TYPE_ASR_SPOTTER_VALIDATION
        );

        if (spotterRsp.HasValid()) {
            req->MutableInfo()->MutableSpotterFeatures()->SetValidated(spotterRsp.GetValid());
        }
    }
}

void TRequestVoiceInputActivationSecondAnnouncement::ReplyToApphostContextOnSuccess(
    NAppHost::TServiceContextPtr ctx
) {
    const NCachalotProtocol::TActivationAnnouncementRequest& req = Request.GetActivationAnnouncement();
    const NCachalotProtocol::TActivationAnnouncementResponse& rsp = Response.GetActivationAnnouncement();
    const NCachalotProtocol::TActivationSubjectInfo& info = req.GetInfo();

    // Forwarding request to the final stage.
    // This request is patched with TSpotterResponseForActivation.
    NCachalotProtocol::TActivationFinalRequest finalReq;
    finalReq.MutableInfo()->CopyFrom(info);
    finalReq.SetIgnoreRms(rsp.GetZeroRmsFound());
    if (req.HasFreshnessDeltaMilliSeconds()) {
        finalReq.SetFreshnessDeltaMilliSeconds(req.GetFreshnessDeltaMilliSeconds());
    }
    ctx->AddProtobufItem(std::move(finalReq), ITEM_TYPE_FINAL_REQUEST);

    // Response for MM_RUN.
    ctx->AddProtobufItem(rsp, ITEM_TYPE_ANNOUNCEMENT_RESPONSE);

    // Creating log.
    AnaLog.MutableProto()->SetYandexUid(info.GetUserId());
    AnaLog.MutableProto()->SetDeviceId(info.GetDeviceId());
    AnaLog.MutableProto()->MutableTimestamp()->CopyFrom(info.GetActivationAttemptTime());
    AnaLog.MutableProto()->SetMultiActivationReason("OK");
    AnaLog.MutableProto()->SetAvgRMS(GetPatchedAvgRms(info));

    if (Options.FreshnessManager.Defined()) {
        AnaLog.MutableProto()->SetFreshnessDeltaMilliSeconds(Options.FreshnessManager->FreshnessDeltaMilliSeconds);
    } else {
        AnaLog.MutableProto()->SetFreshnessDeltaMilliSeconds(Storage->GetFreshnessThresholdMilliSeconds());
    }

    if (!rsp.GetContinuationAllowed()) {
        if (rsp.GetLeaderFound()) {
            AnaLog.MutableProto()->SetMultiActivationReason("LeaderAlreadyElected (on second stage)");
        } else {
            AnaLog.MutableProto()->SetMultiActivationReason("AnotherIsBetter (on second stage)");
        }
        if (rsp.HasBestCompetitor()) {
            const NCachalotProtocol::TActivationSubjectInfo& competitor = rsp.GetBestCompetitor();
            AnaLog.MutableProto()->SetActivatedDeviceId(competitor.GetDeviceId());
            AnaLog.MutableProto()->MutableActivatedTimestamp()->CopyFrom(competitor.GetActivationAttemptTime());
            AnaLog.MutableProto()->SetActivatedRMS(competitor.GetSpotterFeatures().GetAvgRMS());
        }
    }

    AnaLog.StoreActivationLog(ctx, /* final = */ false);
}

void TRequestVoiceInputActivationSecondAnnouncement::ReplyToApphostContextOnError(
    NAppHost::TServiceContextPtr ctx
) {
    if (Request.HasActivationAnnouncement()) {
        const NCachalotProtocol::TActivationAnnouncementRequest& req = Request.GetActivationAnnouncement();
        const NCachalotProtocol::TActivationSubjectInfo& info = req.GetInfo();

        NCachalotProtocol::TActivationFinalRequest finalReq;
        finalReq.MutableInfo()->CopyFrom(info);
        ctx->AddProtobufItem(std::move(finalReq), ITEM_TYPE_FINAL_REQUEST);

        AnaLog.MutableProto()->SetYandexUid(info.GetUserId());
        AnaLog.MutableProto()->SetDeviceId(info.GetDeviceId());
        AnaLog.MutableProto()->MutableTimestamp()->CopyFrom(info.GetActivationAttemptTime());

        // Fake response for MM_RUN
        NCachalotProtocol::TActivationAnnouncementResponse rsp;
        rsp.SetContinuationAllowed(true);
        rsp.SetError(Response.GetStatusMessage());
        ctx->AddProtobufItem(std::move(rsp), ITEM_TYPE_ANNOUNCEMENT_RESPONSE);
    }

    AnaLog.MutableProto()->SetMultiActivationReason(
        TStringBuilder() << "Error on second stage: " << Response.GetStatusMessage() << ". Status: " << int(Status)
    );

    AnaLog.StoreActivationLog(ctx, /* final = */ false);
}


TAsyncStatus TRequestActivationFinal::ServeAsync() {
    DLOG("[ActivationFinal] started");

    RequestMetrics->OnServeStarted(ArrivalTime);
    Response.MutableStats()->SetWaitingTime(MillisecondsSince(ArrivalTime));
    ActivationAlgorithmMetrics->OnFinal();

    if (!Request.HasActivationFinal()) {
        SetError(EResponseStatus::BAD_REQUEST, "Invalid proto for handle activation_final");
        return NThreading::MakeFuture(TStatus(Status));
    }

    const auto& protoReq = Request.GetActivationFinal();
    const auto& [requestKey, requestData] = ExtractSubjectInfo(protoReq.GetInfo());
    NThreading::TPromise<TStatus> status = NThreading::NewPromise<TStatus>();

    LogFrame->Log().LogEventInfoCombo<NEvClass::ActivationLeadershipAcquisitionStarted>(requestKey.UserId, requestData.DeviceId);
    DLOG("[ActivationFinal] makes ydb request for " << requestKey.UserId << ' ' << requestData.DeviceId);

    ParseRequestOptions(protoReq, &Options);

    ActivationAlgorithmMetrics->OnCleanup();
    LogFrame->Log().LogEventInfoCombo<NEvClass::ActivationCleanupStarted>();

    Storage->ClenupLeaders(requestKey, requestData, Options, LogFrame).Subscribe(
        [
            this, status, requestKey=requestKey, requestData=requestData,
            This=IntrusiveThis()
        ](const auto&) mutable {
            LogFrame->Log().LogEventInfoCombo<NEvClass::ActivationCleanupFinished>();
            MakeTryAcquireLeadershipRequest(status, requestKey, requestData, Options);
        }
    );

    SetStatus(EResponseStatus::OK);

    DLOG("[ActivationFinal] returns for " << requestKey.UserId << ' ' << requestData.DeviceId);

    return status;
}

void TRequestActivationFinal::MakeTryAcquireLeadershipRequest(
    NThreading::TPromise<TStatus> status,
    const TActivationStorageKey& requestKey,
    const TActivationStorageData& requestData,
    const TActivationStorageRequestOptions& options
) {
    Storage->TryAcquireLeadership(requestKey, requestData, options, LogFrame).Subscribe(
        [
            this, status, requestKey=requestKey, requestData=requestData,
            This=IntrusiveThis(), options
        ](const auto& reply) mutable {
            DLOG("[ActivationFinal] processes ydbResponse for " << requestKey.UserId << ' ' << requestData.DeviceId);
            LogFrame->Log().LogEventInfoCombo<NEvClass::ActivationLeadershipAcquisitionGotYdbRsp>();

            const TActivationYdbStorage::TSingleRowResponse& ydbResponse = reply.GetValueSync();
            AddBackendStats(Storage->GetStorageTag(), ydbResponse.Status, ydbResponse.Stats);
            UpdateAnaLogSpotterValidatedBy(ydbResponse.ExtraData.SpotterValidatedBy);

            ::NCachalotProtocol::TActivationFinalResponse* rspPtr = Response.MutableActivationFinal();

            if (!ydbResponse) {
                LogFrame->Log().LogEventErrorCombo<NEvClass::ActivationLeadershipAcquisitionError>(ydbResponse.Stats.ErrorMessage);
                ActivationAlgorithmMetrics->OnFail(ArrivalTime, false);  // TODO (@paxakor): is it best spotter?
                rspPtr->SetActivationAllowed(false);
                rspPtr->SetError(ydbResponse.Stats.ErrorMessage);
                DLOG("[ActivationFinal] Rejected " << requestKey.UserId << ' ' << requestData.DeviceId <<
                    " '" << ydbResponse.Stats.ErrorMessage << '\'');

                Storage->GetLeader(requestKey, requestData, options, LogFrame).Subscribe(
                    [this, rspPtr, requestKey, requestData, status, This=IntrusiveThis()] (const auto& reply) mutable {
                        const TActivationYdbStorage::TSingleRowResponse& ydbResponse = reply.GetValueSync();
                        AddBackendStats(Storage->GetStorageTag(), ydbResponse.Status, ydbResponse.Stats);
                        UpdateAnaLogSpotterValidatedBy(ydbResponse.ExtraData.SpotterValidatedBy);

                        if (ydbResponse.Status == EResponseStatus::OK) {
                            const TActivationStorageData& leaderData = ydbResponse.Data.GetRef();
                            DLOG("[ActivationFinal] Leader of " << requestKey.UserId << ' ' << requestData.DeviceId <<
                                " is device " << leaderData.DeviceId);

                            LogFrame->Log().LogEventInfoCombo<NEvClass::ActivationLeadershipAcquisitionGotLeader>(leaderData.DeviceId);

                            ActivationAlgorithmMetrics->OnLeaderFound();

                            auto* leaderInfo = rspPtr->MutableLeaderInfo();
                            SetCompetitorInfo(leaderInfo, requestKey, leaderData);
                        } else {
                            // It's ok in ydb model.
                            DLOG("[ActivationFinal] Although this spotter was rejected, leader for "
                                << requestKey.UserId << " was not found");

                            LogFrame->Log().LogEventErrorCombo<NEvClass::ActivationLeadershipAcquisitionErrorLeaderWasNotFound>();

                            ActivationAlgorithmMetrics->OnLeaderNotFound();
                        }
                        status.SetValue(EResponseStatus::OK);
                    }
                );

            } else {
                ActivationAlgorithmMetrics->OnWin(ArrivalTime, false);  // TODO (@paxakor): is it best spotter?
                rspPtr->SetActivationAllowed(true);

                DLOG("[ActivationFinal] Elected " << requestKey.UserId << ' ' << requestData.DeviceId);
                LogFrame->Log().LogEventInfoCombo<NEvClass::ActivationLeadershipAcquisitionIAmLeader>();
                status.SetValue(EResponseStatus::OK);
            }
        }
    );
}

void TRequestActivationFinal::UpdateAnaLogSpotterValidatedBy(const TSpotterValidationDevice& svd) {
    if (const TMaybe<TString> deviceId = AnaLog.UpdateSpotterValidatedBy(svd)) {
        ::NCachalotProtocol::TActivationFinalResponse* rspPtr = Response.MutableActivationFinal();
        rspPtr->SetSpotterValidatedBy(deviceId.GetRef());
    }
}


TRequestVoiceInputActivationFinal::TRequestVoiceInputActivationFinal(
    NAppHost::TServiceContextPtr ctx,
    TIntrusivePtr<IActivationStorage> storage,
    TActivationServiceConfig config
)
    : TRequestActivationFinal(ctx, std::move(storage), std::move(config))
{
    if (!AnaLog.LoadActivationLog(ctx)) {
        DLOG("Bad request: not found activation_log message");
        SetError(EResponseStatus::BAD_REQUEST, "Bad request: empty log protobuf");
        RequestMetrics->OnBadData(ArrivalTime);
    } else if (!ctx->HasProtobufItem(ITEM_TYPE_FINAL_REQUEST)) {
        DLOG("Bad request: not found final_request message");
        SetError(EResponseStatus::BAD_REQUEST, "Bad request: empty request protobuf");
        RequestMetrics->OnBadData(ArrivalTime);
    } else if (!ctx->HasProtobufItem(ITEM_TYPE_MM_RUN_READY)) {
        DLOG("Bad request: not found mm_run_ready message");
        SetError(EResponseStatus::BAD_REQUEST, "Bad request: empty mm_run_ready protobuf");
        RequestMetrics->OnBadData(ArrivalTime);
    } else {
        ctx->GetOnlyProtobufItem<NCachalotProtocol::TActivationFinalRequest>(
            ITEM_TYPE_FINAL_REQUEST
        ).Swap(Request.MutableActivationFinal());
        DLOG("Request.MutableActivationFinal() " << Request.GetActivationFinal().ShortUtf8DebugString());

        auto mmRunRsp = ctx->GetOnlyProtobufItem<NCachalotProtocol::TMMRunResponseForActivation>(
            ITEM_TYPE_MM_RUN_READY
        );
        Y_UNUSED(mmRunRsp);
    }
}

void TRequestVoiceInputActivationFinal::ReplyToApphostContextOnSuccess(
    NAppHost::TServiceContextPtr ctx
) {
    const NCachalotProtocol::TActivationFinalRequest& req = Request.GetActivationFinal();
    const NCachalotProtocol::TActivationFinalResponse& rsp = Response.GetActivationFinal();
    const NCachalotProtocol::TActivationSubjectInfo& info = req.GetInfo();

    DLOG("FinalResponse: " << rsp.ShortUtf8DebugString());

    ctx->AddProtobufItem(rsp, ITEM_TYPE_FINAL_RESPONSE);
    if (rsp.GetActivationAllowed()) {
        ctx->AddProtobufItem(NCachalotProtocol::TActivationSuccessful(), ITEM_TYPE_ACTIVATION_SUCCESSFUL);
    }

    // Creating log.
    AnaLog.MutableProto()->MutableFinishTimestamp()->CopyFrom(NProtoInterop::CastToProto(TInstant::Now()));

    if (!rsp.GetActivationAllowed()) {
        if (rsp.HasLeaderInfo()) {
            const NCachalotProtocol::TActivationSubjectInfo& leader = rsp.GetLeaderInfo();
            AnaLog.MutableProto()->SetActivatedDeviceId(leader.GetDeviceId());
            AnaLog.MutableProto()->MutableActivatedTimestamp()->CopyFrom(leader.GetActivationAttemptTime());
            AnaLog.MutableProto()->SetActivatedRMS(leader.GetSpotterFeatures().GetAvgRMS());

            if (GetPatchedAvgRms(info) < leader.GetSpotterFeatures().GetAvgRMS()) {
                AnaLog.MutableProto()->SetMultiActivationReason("AnotherIsBetter (on third stage)");
            } else {
                AnaLog.MutableProto()->SetMultiActivationReason("LeaderAlreadyElected (on third stage)");
            }
        } else {
            AnaLog.MutableProto()->SetMultiActivationReason("NoValidSpotterFound (on third stage)");
        }
    } else {
        AnaLog.MutableProto()->SetActivatedDeviceId(info.GetDeviceId());
        AnaLog.MutableProto()->MutableActivatedTimestamp()->CopyFrom(info.GetActivationAttemptTime());
        AnaLog.MutableProto()->SetActivatedRMS(GetPatchedAvgRms(info));
    }

    AnaLog.StoreActivationLog(ctx, /* final = */ true);
}

void TRequestVoiceInputActivationFinal::ReplyToApphostContextOnError(
    NAppHost::TServiceContextPtr ctx
) {
    // Fake response.
    NCachalotProtocol::TActivationFinalResponse rsp;
    rsp.SetActivationAllowed(true);
    rsp.SetError(Response.GetStatusMessage());
    ctx->AddProtobufItem(std::move(rsp), ITEM_TYPE_FINAL_RESPONSE);
    ctx->AddProtobufItem(NCachalotProtocol::TActivationSuccessful(), ITEM_TYPE_ACTIVATION_SUCCESSFUL);

    AnaLog.MutableProto()->SetMultiActivationReason(
        TStringBuilder() << AnaLog.MutableProto()->GetMultiActivationReason()
                         << ". Error on third stage: " << Response.GetStatusMessage() << ". Status: " << int(Status)
    );
    AnaLog.StoreActivationLog(ctx, /* final = */ true);
}

}   // namespace NCachalot
