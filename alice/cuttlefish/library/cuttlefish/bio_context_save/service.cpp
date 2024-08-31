#include "service.h"

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/protos/bio_context_save.pb.h>
#include <alice/cachalot/api/protos/cachalot.pb.h>
#include <alice/cuttlefish/library/protos/events.pb.h>
#include <alice/cuttlefish/library/protos/context_save.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/protos/store_audio.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/proto/protobuf.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/speechkit/directives.pb.h>

#include <voicetech/library/proto_api/yabio.pb.h>

#include <util/string/builder.h>
#include <util/generic/guid.h>

namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

using TSaveVoiceprintDirective = NAlice::NScenarios::TSaveVoiceprintDirective;
using TRemoveVoiceprintDirective = NAlice::NScenarios::TRemoveVoiceprintDirective;
using TYabioUsers = ::google::protobuf::RepeatedPtrField<::YabioProtobuf::YabioUser>;

using TReqIdAndSource = std::pair<TString, TString>;  // <RequestId, Source>

constexpr int ENROLLINGS_LIMIT = 40;
constexpr TStringBuf SAVE_VOICEPRINT_DIRECTIVE = "save_voiceprint";
constexpr TStringBuf REMOVE_VOICEPRINT_DIRECTIVE = "remove_voiceprint";

class TBioContextSaveProcessor {
public:
    TBioContextSaveProcessor(const NAliceCuttlefishConfig::TConfig& config, NAppHost::IServiceContext& ctx, TLogContext logContext);

    void Pre();
    void Post();

private:
    // methods for changing yabio_context
    void ApplySpeechkitDirectives();
    void ApplyUserDirectives();
    void ApplySaveVoiceprintDirective(const TSaveVoiceprintDirective& saveVoiceprint);
    void ApplyRemoveVoiceprintDirective(const TRemoveVoiceprintDirective& removeVoiceprint);

    void ApplyNewEnrolling();
    void ApplyMdsUrlCache();
    void ApplyTextCache();

    // method for working with apphost context
    void AddCachalotSaveRequest();
    void AddEventException(const TString& message);

    // helper methods
    bool TagIsCompatible(const TStringBuf tag) const;
    THashMap<TReqIdAndSource, TString> ConstructTextCache() const;
    THashMap<TReqIdAndSource, TString> ConstructMdsUrlCache() const;
    THashSet<TReqIdAndSource> ConstructFreshReqIds() const;

private:
    const NAliceCuttlefishConfig::TConfig& Config;
    NAppHost::IServiceContext& Ctx;
    TLogContext LogContext;
    mutable TSourceMetrics Metrics;

    const NAliceProtocol::TSessionContext SessionCtx;
    const NAliceProtocol::TRequestContext RequestCtx;
    const NAliceProtocol::TBioContextSaveNewEnrolling NewEnrolling;
    YabioProtobuf::YabioContext YabioCtx;  // will be taken from Ctx, then changed and saved in Ctx again

    bool YabioContextChanged = false;
};

template<typename TProtobuf>
TProtobuf TryLoad(const NAppHost::IServiceContext& ctx, const TStringBuf type) {
    TProtobuf proto;
    if (ctx.HasProtobufItem(type)) {
        proto = ctx.GetOnlyProtobufItem<TProtobuf>(type);
    }
    return proto;
}

bool IsTargetUser(const ::YabioProtobuf::YabioUser& user, const TString& userId, const TString& persId) {
    if (user.pers_id().Empty() || persId.Empty()) {
        return user.user_id() == userId;
    }

    return user.user_id() == userId && user.pers_id() == persId;
}

YabioProtobuf::YabioUser* FirstOrNullUser(TYabioUsers& users, const TString& userId, const TString& persId) {
    for (auto& curUser : users) {
        if (IsTargetUser(curUser, userId, persId)) {
            return &curUser;
        }
    }

    return nullptr;
}

bool RemoveUser(TYabioUsers& users, const TString& userId, const TString& persId) {
    for (int userPtr = 0; userPtr < users.size(); ++userPtr) {
        if (IsTargetUser(users[userPtr], userId, persId)) {
            users.DeleteSubrange(/* start = */ userPtr, /* num = */ 1);
            return true;
        }
    }

    return false;
}

THashMap<TString, int> NumerateRequestIds(const ::google::protobuf::RepeatedPtrField<TProtoStringType>& requestIds) {
    THashMap<TString, int> requestsIdsMap;

    for (int i = 0; i < requestIds.size(); i++) {
        requestsIdsMap.emplace(requestIds[i], i);
    }

    return requestsIdsMap;
}

TBioContextSaveProcessor::TBioContextSaveProcessor(const NAliceCuttlefishConfig::TConfig& config, NAppHost::IServiceContext& ctx, TLogContext logContext)
    : Config(config)
    , Ctx(ctx)
    , LogContext(logContext)
    , Metrics(ctx, "bio_context_save")
    , SessionCtx(TryLoad<NAliceProtocol::TSessionContext>(ctx, ITEM_TYPE_SESSION_CONTEXT))
    , RequestCtx(TryLoad<NAliceProtocol::TRequestContext>(ctx, ITEM_TYPE_REQUEST_CONTEXT))
    , NewEnrolling(TryLoad<NAliceProtocol::TBioContextSaveNewEnrolling>(ctx, ITEM_TYPE_YABIO_NEW_ENROLLING))
    , YabioCtx(TryLoad<YabioProtobuf::YabioContext>(ctx, ITEM_TYPE_YABIO_CONTEXT))
{
    LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostYabioContext>(
        TStringBuilder() << "group_id=" << YabioCtx.group_id() << " users=" << YabioCtx.users().size() << " guests=" << YabioCtx.guests().size() << " enrollings=" << YabioCtx.enrolling().size()
    );
}

void TBioContextSaveProcessor::Pre() {
    // the order of calls is important!
    ApplyNewEnrolling();
    ApplyMdsUrlCache();
    ApplyTextCache();
    ApplySpeechkitDirectives();
    ApplyUserDirectives();

    // create request for YDB
    AddCachalotSaveRequest();
}

void TBioContextSaveProcessor::Post() {
    const auto& resp = Ctx.GetOnlyProtobufItem<NCachalotProtocol::TYabioContextResponse>(ITEM_TYPE_YABIO_CONTEXT_RESPONSE);
    LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostYabioContextResponse>(resp.ShortUtf8DebugString());
    if (resp.HasError()) {
        Metrics.SetError("yabio_context_save_error_" + ToString(resp.GetError().GetStatus()));
        LogContext.LogEventErrorCombo<NEvClass::YabioContextSavingError>(TStringBuilder()
            << "Yabio context saving error ("
            << ToString(resp.GetError().GetStatus()) << "): "
            << resp.GetError().GetText()
        );
    } else {
        Metrics.PushRate("bio_context_save", "ok");
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostYabioContextSaved>();
        Ctx.AddProtobufItem(NAliceProtocol::TBioContextSaved(), ITEM_TYPE_YABIO_CONTEXT_SAVED);
    }
}

void TBioContextSaveProcessor::ApplySpeechkitDirectives() {
    if (!Ctx.HasProtobufItem(ITEM_TYPE_CONTEXT_SAVE_REQUEST)) {
        return;
    }

    const auto& request = Ctx.GetOnlyProtobufItem<NAliceProtocol::TContextSaveRequest>(ITEM_TYPE_CONTEXT_SAVE_REQUEST);
    LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostContextSaveRequest>(request.ShortUtf8DebugString());
    for (const auto& directive : request.GetDirectives()) {
        if (!directive.HasPayload()) {
            continue;
        }

        const TString& name = directive.GetName();
        if (name == SAVE_VOICEPRINT_DIRECTIVE) {
            ApplySaveVoiceprintDirective(NAlice::StructToMessage<TSaveVoiceprintDirective>(directive.GetPayload()));
        } else if (name == REMOVE_VOICEPRINT_DIRECTIVE) {
            ApplyRemoveVoiceprintDirective(NAlice::StructToMessage<TRemoveVoiceprintDirective>(directive.GetPayload()));
        }
    }
}

void TBioContextSaveProcessor::ApplyUserDirectives() {
    if (!Ctx.HasProtobufItem(ITEM_TYPE_REQUEST_BIO_CONTEXT_UPDATE)) {
        return;
    }

    const auto& request = Ctx.GetOnlyProtobufItem<NAliceProtocol::TBioContextUpdate>(ITEM_TYPE_REQUEST_BIO_CONTEXT_UPDATE);
    LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostBioContextUpdate>(request.ShortUtf8DebugString());

    if (request.HasCreateUser()) {
        ApplySaveVoiceprintDirective(request.GetCreateUser());
    } else if (request.HasRemoveUser()) {
        ApplyRemoveVoiceprintDirective(request.GetRemoveUser());
    }
}

void TBioContextSaveProcessor::ApplySaveVoiceprintDirective(const TSaveVoiceprintDirective& saveVoiceprint) {
    const auto& requestsIds = NumerateRequestIds(saveVoiceprint.GetRequestIds());
    const TString& userId = saveVoiceprint.GetUserId();
    const TString& persId = saveVoiceprint.GetPersId();

    // change req_num and find new voiceprints
    THashSet<TString> addedRequestIds;
    TVector<YabioProtobuf::YabioVoiceprint*> newVoiceprints;
    for (auto& enroll : *YabioCtx.mutable_enrolling()) {
        const auto iter = requestsIds.find(enroll.request_id());
        if (!iter.IsEnd()) {
            enroll.set_reg_num(iter->second);
            addedRequestIds.insert(enroll.request_id());
            newVoiceprints.push_back(&enroll);
        }
    }

    if (newVoiceprints.empty()) {
        AddEventException("Not found all requested voiceprints for creating new user");
        return;
    }

    int lostCount = static_cast<int>(requestsIds.size()) - static_cast<int>(addedRequestIds.size());
    if (lostCount > 0) {
        LogContext.LogEventInfoCombo<NEvClass::WarningMessage>(TStringBuilder() << "yabio_context not found all request_id for create/update new user lost=" << lostCount);
    }

    ::google::protobuf::RepeatedPtrField<YabioProtobuf::YabioUser>& users = saveVoiceprint.GetUserType() == TSaveVoiceprintDirective::Owner
        ? *YabioCtx.mutable_users()
        : *YabioCtx.mutable_guests();

    YabioProtobuf::YabioUser* user = FirstOrNullUser(users, userId, persId);

    if (!user) {
        user = users.Add();
        user->set_user_id(userId);
        user->set_pers_id(persId);
    } else {
        user->mutable_voiceprints()->Clear();
    }

    // fill user voiceprints
    for (auto* vp : newVoiceprints) {
        user->mutable_voiceprints()->Add()->CopyFrom(*vp);
    }

    // remove voiceprints used for user enrolling
    for (size_t enrollingPtr = 0; enrollingPtr < YabioCtx.enrollingSize(); ) {
        if (requestsIds.contains(YabioCtx.enrolling(enrollingPtr).request_id())) {
            YabioCtx.mutable_enrolling()->DeleteSubrange(/* start = */ enrollingPtr, /* num = */ 1);
        } else {
            ++enrollingPtr;
        }
    }

    YabioContextChanged = true;
}

void TBioContextSaveProcessor::ApplyRemoveVoiceprintDirective(const TRemoveVoiceprintDirective& removeVoiceprint) {
    // read data from payload
    const TString& userId = removeVoiceprint.GetUserId();
    const TString& persId = removeVoiceprint.GetPersId();

    YabioContextChanged |= RemoveUser(*YabioCtx.mutable_users(), userId, persId);
    YabioContextChanged |= RemoveUser(*YabioCtx.mutable_guests(), userId, persId);
}

void TBioContextSaveProcessor::ApplyNewEnrolling() {
    if (!Ctx.HasProtobufItem(ITEM_TYPE_YABIO_NEW_ENROLLING)) {
        return;
    }

    YabioContextChanged = true;
    const auto freshReqIds = ConstructFreshReqIds();

    // delete unsupported compatibility tags
    if (NewEnrolling.SupportedTagsSize() > 0) {
        for (auto* users : {
            YabioCtx.mutable_users(),
            YabioCtx.mutable_guests()
        }) {
            for (int userPtr = 0; userPtr < users->size(); ) {
                auto& user = users->at(userPtr);

                for (size_t voicePtr = 0; voicePtr < user.voiceprintsSize(); ) {
                    const auto& tag = user.voiceprints(voicePtr).compatibility_tag();
                    if (TagIsCompatible(tag)) {
                        ++voicePtr;
                    } else {
                        user.mutable_voiceprints()->DeleteSubrange(/* start = */ voicePtr, /* num = */ 1);
                    }
                }

                if (user.voiceprints_size() == 0) {
                    users->DeleteSubrange(/* start = */ userPtr, /* num = */ 1);
                } else {
                    ++userPtr;
                }
            }
        }
    }

    for (size_t enrollingPtr = 0; enrollingPtr < YabioCtx.enrollingSize(); ) {
        const auto& yabioEnrolling = YabioCtx.enrolling(enrollingPtr);
        const bool unsupportedTag = NewEnrolling.SupportedTagsSize() > 0 && !TagIsCompatible(yabioEnrolling.compatibility_tag());
        const bool needToReplace = freshReqIds.contains(TReqIdAndSource{yabioEnrolling.request_id(), yabioEnrolling.source()});

        if (unsupportedTag || needToReplace) {
            YabioCtx.mutable_enrolling()->DeleteSubrange(/* start = */ enrollingPtr, /* num = */ 1);
        } else {
            ++enrollingPtr;
        }
    }

    // push new enrollings to yabio_context
    for (const auto& enroll : NewEnrolling.GetYabioEnrolling()) {
        YabioCtx.mutable_enrolling()->Add()->CopyFrom(enroll);
    }

    // delete oldest enrollings if overlimited
    int overlimit = YabioCtx.enrolling_size() - ENROLLINGS_LIMIT;
    if (overlimit > 0) {
        YabioCtx.mutable_enrolling()->DeleteSubrange(/* start = */ 0, /* num = */ overlimit);
    }

    LogContext.LogEventInfoCombo<NEvClass::DebugMessage>(
        TStringBuilder() << "after apply new_enrolling: group_id=" << YabioCtx.group_id() << " users=" << YabioCtx.users().size() << " enrollings=" << YabioCtx.enrolling().size()
    );
}

void TBioContextSaveProcessor::ApplyMdsUrlCache() {
    const auto mdsUrlCache = ConstructMdsUrlCache();

    for (auto& enroll : *YabioCtx.mutable_enrolling()) {
        auto iter = mdsUrlCache.find(TReqIdAndSource{enroll.request_id(), enroll.source()});
        if (!iter.IsEnd()) {
            enroll.set_mds_url(iter->second);
            LogContext.LogEventInfoCombo<NEvClass::MdsUrlApplied>(enroll.request_id(), enroll.source(), iter->second);
            YabioContextChanged = true;
        }
    }

    for (auto* users : {
        YabioCtx.mutable_users(),
        YabioCtx.mutable_guests()
    }) {
        for (auto& user : *users) {
            for (auto& vp : *user.mutable_voiceprints()) {
                auto iter = mdsUrlCache.find(TReqIdAndSource{vp.request_id(), vp.source()});
                if (!iter.IsEnd()) {
                    vp.set_mds_url(iter->second);
                    LogContext.LogEventInfoCombo<NEvClass::MdsUrlApplied>(vp.request_id(), vp.source(), iter->second);
                    YabioContextChanged = true;
                }
            }
        }
    }
}

void TBioContextSaveProcessor::ApplyTextCache() {
    const auto textCache = ConstructTextCache();

    for (auto& enroll : *YabioCtx.mutable_enrolling()) {
        auto iter = textCache.find(TReqIdAndSource{enroll.request_id(), enroll.source()});
        if (!iter.IsEnd()) {
            enroll.set_text(iter->second);
            YabioContextChanged = true;
        }
    }
}

void TBioContextSaveProcessor::AddCachalotSaveRequest() {
    if (!YabioContextChanged) {
        return;
    }

    NCachalotProtocol::TYabioContextKey key;
    TString groupId = YabioCtx.group_id();
    if (!groupId) {
        groupId = SessionCtx.GetUserInfo().GetUuid();
    }
    key.SetGroupId(groupId);
    key.SetDevModel(SessionCtx.GetDeviceInfo().GetDeviceModel());
    key.SetDevManuf(SessionCtx.GetDeviceInfo().GetDeviceManufacturer());

    NCachalotProtocol::TYabioContextRequest req;
    auto& saveReq = *req.MutableSave();
    saveReq.MutableKey()->Swap(&key);
    TString rawCtx = YabioCtx.SerializeAsString();
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostYabioContextRequest>(req.ShortUtf8DebugString() + (TStringBuilder() << " #serialized_ctx_size=" << rawCtx.Size()));
    saveReq.SetContext(rawCtx);
    Ctx.AddProtobufItem(req, ITEM_TYPE_YABIO_CONTEXT_REQUEST);
}

void TBioContextSaveProcessor::AddEventException(const TString& message) {
    NAliceProtocol::TDirective directive;
    auto& exc = *directive.MutableException();
    exc.SetScope("BioContextSaveProcessor");
    exc.SetText(message);
    LogContext.LogEventErrorCombo<NEvClass::SendToAppHostDirective>(directive.ShortUtf8DebugString());
    Ctx.AddProtobufItem(directive, ITEM_TYPE_DIRECTIVE);
}

bool TBioContextSaveProcessor::TagIsCompatible(const TStringBuf tag) const {
    return AnyOf(NewEnrolling.GetSupportedTags(), [&tag](const auto& supportedTag) { return tag == supportedTag; });
}

THashMap<TReqIdAndSource, TString> TBioContextSaveProcessor::ConstructTextCache() const {
    THashMap<TReqIdAndSource, TString> map;

    for (const auto& item : Ctx.GetProtobufItemRefs(ITEM_TYPE_YABIO_TEXT)) {
        try {
            NAliceProtocol::TBioContextSaveText textProto;
            ParseProtobufItem(item, textProto);

            const TReqIdAndSource key{RequestCtx.GetHeader().GetMessageId(), textProto.GetSource()};
            map[key] = textProto.GetText();
        } catch (...) {
            Metrics.PushRate("prase_bio_context_save_text", "error");
            LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(
                TStringBuilder()
                    << "Failed to parse bio context save text: "
                    << CurrentExceptionMessage()
            );
        }
    }

    return map;
}

THashMap<TReqIdAndSource, TString> TBioContextSaveProcessor::ConstructMdsUrlCache() const {
    THashMap<TReqIdAndSource, TString> map;

    for (const auto& item : Ctx.GetProtobufItemRefs(ITEM_TYPE_STORE_AUDIO_RESPONSE)) {
        try {
            NAliceProtocol::TStoreAudioResponse audioResp;
            ParseProtobufItem(item, audioResp);

            if (audioResp.HasKey()) {
                const TReqIdAndSource key{RequestCtx.GetHeader().GetMessageId(), audioResp.GetIsSpotter() ? "spotter" : "request"};
                LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostStoreAudioResponse>(audioResp.ShortUtf8DebugString());
                map[key] = TStringBuilder() << Config.mds().download_url() << "get-" << Config.mds().upload_namespace() << "/" << audioResp.GetKey();
            }
        } catch (...) {
            Metrics.PushRate("prase_store_audio_response", "error");
            LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(
                TStringBuilder()
                    << "Failed to parse store audio response: "
                    << CurrentExceptionMessage()
            );
        }
    }

    return map;
}

THashSet<TReqIdAndSource> TBioContextSaveProcessor::ConstructFreshReqIds() const {
    const auto& yabioEnrolling = NewEnrolling.GetYabioEnrolling();

    THashSet<TReqIdAndSource> set;
    set.reserve(yabioEnrolling.size());

    for (const auto& enroll : yabioEnrolling) {
        set.insert(TReqIdAndSource{enroll.request_id(), enroll.source()});
    }

    return set;
}

}

void BioContextSavePre(const NAliceCuttlefishConfig::TConfig& config, NAppHost::IServiceContext& ctx, TLogContext logContext) {
    TBioContextSaveProcessor{config, ctx, logContext}.Pre();
}

void BioContextSavePost(const NAliceCuttlefishConfig::TConfig& config, NAppHost::IServiceContext& ctx, TLogContext logContext) {
    TBioContextSaveProcessor{config, ctx, logContext}.Post();
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
