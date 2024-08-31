#include "wonderlogs.h"

#include "ttls.h"

#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/library/parsers/wonderlogs.h>
#include <alice/wonderlogs/library/robot/account_type.h>
#include <alice/wonderlogs/library/yt/utils.h>
#include <alice/wonderlogs/protos/asr_prepared.pb.h>
#include <alice/wonderlogs/protos/megamind_prepared.pb.h>
#include <alice/wonderlogs/protos/private_user.pb.h>
#include <alice/wonderlogs/protos/uniproxy_prepared.pb.h>
#include <alice/wonderlogs/protos/wonderlogs.pb.h>

#include <alice/library/client/protos/client_info.pb.h>
#include <alice/megamind/protos/common/events.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <kernel/geo/utils.h>

#include <library/cpp/threading/future/async.h>
#include <library/cpp/threading/future/future.h>
#include <library/cpp/threading/future/subscription/wait_all_or_exception.h>
#include <library/cpp/yson/node/node_io.h>

#include <mapreduce/yt/library/operation_tracker/operation_tracker.h>

#include <voicetech/library/proto_api/analytics_info.pb.h>

#include <util/generic/size_literals.h>
#include <util/string/builder.h>

namespace NAlice::NWonderlogs::NImpl {

bool DoCensor(const TWonderlog::TPrivacy& privacy) {
    return privacy.GetOriginalDoNotUseUserLogs() | privacy.GetGeoRestrictions().GetProhibitedByRegion() |
           privacy.GetContentProperties().GetContainsSensitiveDataInRequest() |
           privacy.GetContentProperties().GetContainsSensitiveDataInResponse();
}

NAlice::TCensor::TFlags CensorFlags(const TWonderlog::TPrivacy& privacy) {
    if (privacy.GetOriginalDoNotUseUserLogs() || privacy.GetGeoRestrictions().GetProhibitedByRegion()) {
        return NAlice::TCensor::TFlags{NAlice::EAccess::A_PRIVATE_REQUEST} | NAlice::EAccess::A_PRIVATE_RESPONSE;
    }
    NAlice::TCensor::TFlags flags;
    if (privacy.GetContentProperties().GetContainsSensitiveDataInRequest()) {
        flags |= NAlice::EAccess::A_PRIVATE_REQUEST;
    }
    if (privacy.GetContentProperties().GetContainsSensitiveDataInResponse()) {
        flags |= NAlice::EAccess::A_PRIVATE_RESPONSE;
    }
    return flags;
}

TWonderlog::TPrivacy::TGeoRestrictions::ERegion GetRegion(const NGeobase::TLookup* geobase, const TString& ip) {
    Y_ENSURE(geobase);
    const auto regionId = geobase->GetRegionIdByIp(ip);
    const auto countryId = geobase->GetCountryId(regionId);
    if (TWonderlog::TPrivacy::TGeoRestrictions::ERegion_IsValid(countryId)) {
        return static_cast<TWonderlog::TPrivacy::TGeoRestrictions::ERegion>(countryId);
    }
    return TWonderlog::TPrivacy::TGeoRestrictions::R_UNDEFINED;
}

bool ProhibitedRegion(TWonderlog::TPrivacy::TGeoRestrictions::ERegion region) {
    return region == TWonderlog::TPrivacy::TGeoRestrictions::R_COUNTRY_ISRAEL;
}

bool GetDoNotUseUserLogs(const TWonderlog::TPrivacy& privacy) {
    return privacy.GetOriginalDoNotUseUserLogs() | privacy.GetProhibitedByGdpr() |
           privacy.GetGeoRestrictions().GetProhibitedByRegion() |
           privacy.GetContentProperties().GetContainsSensitiveDataInRequest() |
           privacy.GetContentProperties().GetContainsSensitiveDataInResponse();
}

NYT::TTableSchema ChangeSchema(const NYT::TTableSchema& wonderlogsSchema, const TVector<TVector<TString>>& paths) {
    NYT::TNode schema = wonderlogsSchema.ToNode();

    for (const auto& path : paths) {
        auto* curNode = &schema;
        bool first = true;
        for (size_t i = 0; i + 1 < path.size(); ++i) {
            for (auto& col : curNode->AsList()) {
                if (path[i] == col["name"].AsString()) {
                    curNode = &col[first ? "type_v3" : "type"]["item"]["members"];
                    first = false;
                    break;
                }
            }
        }

        size_t colIndex = 0;
        for (; colIndex < curNode->AsList().size(); ++colIndex) {
            if (path.back() == curNode->AsList()[colIndex]["name"].AsString()) {
                break;
            }
        }

        NYT::TNode col;
        col["name"] = path.back();
        col["required"] = false;
        col["type"] = "any";
        col["type_v3"] = curNode->AsList()[colIndex]["type"];

        curNode->AsList().erase(curNode->AsList().begin() + colIndex);
        schema.AsList().push_back(col);
    }

    NYT::TTableSchema newWonderlogsSchema;
    NYT::Deserialize(newWonderlogsSchema, schema);
    return newWonderlogsSchema;
}

NYT::TNode MoveToColumns(NYT::TNode node, const TVector<TVector<TString>>& paths) {
    if (!node.IsMap()) {
        return node;
    }
    for (const auto& path : paths) {
        auto* curNode = &node;
        for (size_t i = 0; i + 1 < path.size(); ++i) {
            if (curNode->IsMap() && curNode->AsMap().contains(path[i])) {
                curNode = &(curNode->AsMap()[path[i]]);
            }
        }
        if (curNode->IsMap()) {
            node[path.back()] = (*curNode)[path.back()];
            curNode->AsMap().erase(path.back());
        }
    }
    return node;
}

} // namespace NAlice::NWonderlogs::NImpl

namespace {

using namespace NAlice::NWonderlogs;

using google::protobuf::Message;

class TWonderlogsReducer : public NYT::IReducer<NYT::TTableReader<Message>, NYT::TTableWriter<Message>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TString& uniproxyPreparedTable, const TString& megamindPreparedTable,
                           const TString& asrPreparedTable, const TString& wonderlogsTable,
                           const TString& privateWonderlogsTable, const TString& robotWonderlogsTable,
                           const TString& errorTable)
            : UniproxyPreparedTable(uniproxyPreparedTable)
            , MegamindPreparedTable(megamindPreparedTable)
            , AsrPreparedTable(asrPreparedTable)
            , WonderlogsTable(wonderlogsTable)
            , PrivateWonderlogsTable(privateWonderlogsTable)
            , RobotWonderlogsTable(robotWonderlogsTable)
            , ErrorTable(errorTable) {
        }
        NYT::TReduceOperationSpec AddToOperationSpec(NYT::TReduceOperationSpec&& operationSpec) {
            const auto addAttributes = [](NYT::TRichYPath&& path) -> NYT::TRichYPath {
                return path.CompressionCodec("brotli_8")
                    .ErasureCodec(NYT::EErasureCodecAttr::EC_LRC_12_2_2_ATTR)
                    .OptimizeFor(NYT::EOptimizeForAttr::OF_SCAN_ATTR);
            };
            return operationSpec.AddInput<TUniproxyPrepared>(UniproxyPreparedTable)
                .AddInput<TMegamindPrepared>(MegamindPreparedTable)
                .AddInput<TAsrPrepared>(AsrPreparedTable)
                .AddOutput<TWonderlog>(addAttributes(NYT::TRichYPath(WonderlogsTable))
                                           .Schema(NYT::CreateTableSchema<TWonderlog>({"_uuid", "_message_id"})))
                .AddOutput<TWonderlog>(addAttributes(NYT::TRichYPath(PrivateWonderlogsTable))
                                           .Schema(NYT::CreateTableSchema<TWonderlog>({"_uuid", "_message_id"})))
                .AddOutput<TWonderlog>(addAttributes(NYT::TRichYPath(RobotWonderlogsTable))
                                           .Schema(NYT::CreateTableSchema<TWonderlog>({"_uuid", "_message_id"})))
                .AddOutput<TWonderlog::TError>(ErrorTable);
        }

        static const ui32 UNIPROXY_PREPARED_INDEX = 0;
        static const ui32 MEGAMIND_PREPARED_INDEX = 1;
        static const ui32 ASR_PREPARED_INDEX = 2;

        static const ui32 WONDERLOGS_INDEX = 0;
        static const ui32 PRIVATE_WONDERLOGS_INDEX = 1;
        static const ui32 ROBOT_WONDERLOGS_INDEX = 2;
        static const ui32 ERROR_INDEX = 3;

        enum EInIndices {
            UniproxyPrepared = 0,
            MegamindPrepared = 1,
            AsrPrepared = 2,
        };

        enum EOutIndices {
            Wonderlogs = 0,
            PrivateWonderlogs = 1,
            RobotWonderlogs = 2,
            Error = 3,
        };

    private:
        const TString& UniproxyPreparedTable;
        const TString& MegamindPreparedTable;
        const TString& AsrPreparedTable;
        const TString& WonderlogsTable;
        const TString& PrivateWonderlogsTable;
        const TString& RobotWonderlogsTable;
        const TString& ErrorTable;
    };

    Y_SAVELOAD_JOB(TimestampFrom, TimestampTo, RequestsShift, GeobasePath, Environment);
    TWonderlogsReducer() = default;
    TWonderlogsReducer(const TInstant& timestampFrom, const TInstant& timestampTo, const TDuration& requestsShift,
                       TString geobasePath, TEnvironment environment)
        : TimestampFrom(timestampFrom)
        , TimestampTo(timestampTo)
        , RequestsShift(requestsShift)
        , GeobasePath(std::move(geobasePath))
        , Environment(std::move(environment)) {
    }

    void Start(TWriter*) override {
        Geobase = MakeHolder<NGeobase::TLookup>(GeobasePath);
    }

    void Do(TReader* reader, TWriter* writer) override {
        TWonderlog wonderlog;
        const auto logError = [&wonderlog, writer](const TWonderlog::TError::EReason reason, const TString& message) {
            TWonderlog::TError error;
            error.SetProcess(TWonderlog::TError::P_WONDERLOGS_REDUCER);
            error.SetReason(reason);
            error.SetMessage(message);
            if (wonderlog.HasUuid()) {
                error.SetUuid(wonderlog.GetUuid());
            }
            if (wonderlog.HasMessageId() && wonderlog.GetRealMessageId()) {
                error.SetMessageId(wonderlog.GetMessageId());
            }
            if (wonderlog.HasMegamindRequestId()) {
                error.SetMegamindRequestId(wonderlog.GetMegamindRequestId());
            }
            if (auto setraceUrl = TryGenerateSetraceUrl(
                    {error.HasMessageId() && wonderlog.GetRealMessageId() ? error.GetMessageId() : TMaybe<TString>{},
                     error.HasMegamindRequestId() ? error.GetMegamindRequestId() : TMaybe<TString>{},
                     error.HasUuid() ? error.GetUuid() : TMaybe<TString>{}})) {
                error.SetSetraceUrl(*setraceUrl);
            }
            writer->AddRow(error, TInputOutputTables::ERROR_INDEX);
        };
        auto setUuid = [logError, &wonderlog](const TString& uuid) {
            if (!wonderlog.HasUuid()) {
                wonderlog.SetUuid(uuid);
            } else if (wonderlog.GetUuid() != uuid) {
                logError(TWonderlog::TError::R_DIFFERENT_VALUES,
                         TStringBuilder{} << "Got different uuid: " << wonderlog.GetUuid() << " " << uuid);
            }
        };
        auto setMessageId = [logError, &wonderlog](const TString& messageId) {
            if (!wonderlog.HasMessageId()) {
                wonderlog.SetMessageId(messageId);
            } else if (wonderlog.GetMessageId() != messageId) {
                logError(TWonderlog::TError::R_DIFFERENT_VALUES,
                         TStringBuilder{} << "Got different message_id: " << wonderlog.GetMessageId() << " "
                                          << messageId);
            }
        };
        auto setMegamindRequestId = [logError, &wonderlog](const TString& megamindRequestId) {
            if (!wonderlog.HasMegamindRequestId()) {
                wonderlog.SetMegamindRequestId(megamindRequestId);
            } else if (wonderlog.GetMegamindRequestId() != megamindRequestId) {
                logError(TWonderlog::TError::R_DIFFERENT_VALUES,
                         TStringBuilder{} << "Got different megamind_request_id: " << wonderlog.GetMegamindRequestId()
                                          << " " << megamindRequestId);
            }
        };

        const auto initializeCommonFields = [setUuid, setMessageId](auto message) {
            setMessageId(message.GetMessageId());
            setUuid(message.GetUuid());
        };

        wonderlog.MutableAsr()->SetTrashOrEmpty(false);
        wonderlog.MutableSpotter()->SetFalseActivation(false);
        wonderlog.MutablePresence()->SetMegamind(false);
        wonderlog.MutablePresence()->SetUniproxy(false);
        wonderlog.MutablePresence()->SetAsr(false);
        wonderlog.SetRealMessageId(true);
        TMaybe<TString> connectSessionId;
        bool originalDoNotUseUserLogs = false;
        for (auto& cursor : *reader) {
            switch (cursor.GetTableIndex()) {
                case TInputOutputTables::UNIPROXY_PREPARED_INDEX: {
                    const auto& uniproxyPrepared = cursor.GetRow<TUniproxyPrepared>();
                    if (uniproxyPrepared.HasDoNotUseUserLogs()) {
                        originalDoNotUseUserLogs = uniproxyPrepared.GetDoNotUseUserLogs();
                    }
                    initializeCommonFields(uniproxyPrepared);
                    if (uniproxyPrepared.HasMegamindRequestId()) {
                        setMegamindRequestId(uniproxyPrepared.GetMegamindRequestId());
                    }
                    if (!connectSessionId) {
                        connectSessionId = uniproxyPrepared.GetConnectSessionId();
                    }
                    ParseUniproxyPrepared(wonderlog, uniproxyPrepared);
                    break;
                }
                case TInputOutputTables::MEGAMIND_PREPARED_INDEX: {
                    const auto& megamindPrepared = cursor.GetRow<TMegamindPrepared>();
                    if (!megamindPrepared.GetRealMessageId()) {
                        wonderlog.SetRealMessageId(false);
                    }
                    initializeCommonFields(megamindPrepared);
                    setMegamindRequestId(megamindPrepared.GetSpeechkitRequest().GetHeader().GetRequestId());

                    *wonderlog.MutableSpeechkitRequest() = megamindPrepared.GetSpeechkitRequest();
                    *wonderlog.MutableSpeechkitResponse() = megamindPrepared.GetSpeechkitResponse();
                    if (wonderlog.GetSpeechkitRequest().GetRequest().GetAdditionalOptions().HasServerTimeMs()) {
                        wonderlog.SetServerTimeMs(
                            wonderlog.GetSpeechkitRequest().GetRequest().GetAdditionalOptions().GetServerTimeMs());
                    }
                    wonderlog.SetServerTimeMs(
                        wonderlog.GetSpeechkitRequest().GetRequest().GetAdditionalOptions().GetServerTimeMs());
                    if (wonderlog.GetSpeechkitRequest().GetHeader().HasSequenceNumber()) {
                        wonderlog.SetSequenceNumber(wonderlog.GetSpeechkitRequest().GetHeader().GetSequenceNumber());
                    }

                    switch (wonderlog.GetSpeechkitRequest().GetRequest().GetEvent().GetType()) {
                        case NAlice::voice_input:
                            wonderlog.SetAction(NAlice::NWonderlogs::TWonderlog::VOICE);
                            break;
                        case NAlice::text_input:
                            wonderlog.SetAction(NAlice::NWonderlogs::TWonderlog::TEXT);
                            break;
                        case NAlice::suggested_input:
                            wonderlog.SetAction(NAlice::NWonderlogs::TWonderlog::SUGGEST);
                            break;
                        case NAlice::image_input:
                            wonderlog.SetAction(NAlice::NWonderlogs::TWonderlog::IMAGE);
                            break;
                        case NAlice::music_input:
                            wonderlog.SetAction(NAlice::NWonderlogs::TWonderlog::MUSIC);
                            break;
                        case NAlice::server_action:
                            wonderlog.SetAction(NAlice::NWonderlogs::TWonderlog::CALLBACK);
                            break;
                    }
                    wonderlog.MutablePresence()->SetMegamind(true);
                    *wonderlog.MutableEnvironment()->MutableMegamindEnvironment() = megamindPrepared.GetEnvironment();
                    if (megamindPrepared.GetSpeechkitRequest()
                            .GetRequest()
                            .GetAdditionalOptions()
                            .GetBassOptions()
                            .HasClientIP()) {
                        wonderlog.MutableDownloadingInfo()->MutableMegamind()->SetClientIp(
                            megamindPrepared.GetSpeechkitRequest()
                                .GetRequest()
                                .GetAdditionalOptions()
                                .GetBassOptions()
                                .GetClientIP());
                    }
                    if (megamindPrepared.GetSpeechkitRequest().GetRequest().HasActivationType()) {
                        wonderlog.MutableAsr()->SetActivationType(
                            megamindPrepared.GetSpeechkitRequest().GetRequest().GetActivationType());
                    }
                    wonderlog.MutableClient()->MutableApplication()->MergeFrom(
                        megamindPrepared.GetSpeechkitRequest().GetApplication());
                    if (megamindPrepared.GetSpeechkitRequest()
                            .GetRequest()
                            .GetAdditionalOptions()
                            .HasDoNotUseUserLogs()) {
                        originalDoNotUseUserLogs |= megamindPrepared.GetSpeechkitRequest()
                                                        .GetRequest()
                                                        .GetAdditionalOptions()
                                                        .GetDoNotUseUserLogs();
                    }
                    break;
                }
                case TInputOutputTables::ASR_PREPARED_INDEX: {
                    const auto& asrPrepared = cursor.GetRow<TAsrPrepared>();
                    initializeCommonFields(asrPrepared);
                    *wonderlog.MutableAsr()->MutableData() = asrPrepared.GetData();
                    // TODO check https://wiki.yandex-team.ru/voicetechnology/dev/analytics/logs/#log-remarks
                    if (asrPrepared.GetData().GetTrash()) {
                        wonderlog.MutableAsr()->SetTrashOrEmpty(true);
                    }
                    if (!asrPrepared.GetData().HasRecognition() ||
                        asrPrepared.GetData().GetRecognition().GetNormalized().empty()) {
                        wonderlog.MutableAsr()->SetTrashOrEmpty(true);
                    }
                    if (asrPrepared.HasTopic()) {
                        wonderlog.MutableAsr()->MutableTopics()->SetRequest(asrPrepared.GetTopic());
                    }
                    if (asrPrepared.HasAnalyticsInfo()) {
                        *wonderlog.MutableAsr()->MutableAnalyticsInfo() = asrPrepared.GetAnalyticsInfo();
                    }
                    wonderlog.MutablePresence()->SetAsr(true);
                    *wonderlog.MutablePresence()->MutableAsrPresence() = asrPrepared.GetPresence();
                    if (!wonderlog.HasServerTimeMs() && asrPrepared.HasTimestampLogMs()) {
                        wonderlog.SetServerTimeMs(asrPrepared.GetTimestampLogMs());
                    }
                    if (asrPrepared.HasOnlineValidation()) {
                        wonderlog.MutableSpotter()->MutableValidation()->SetMdsKey(
                            asrPrepared.GetOnlineValidation().GetMdsKey());
                    }

                    break;
                }
                default:
                    Y_FAIL("Unexpected table index in TWonderlogsReducer");
            }
        }
        TMaybe<TInstant> TimestampLogMs;
        if (wonderlog.HasServerTimeMs()) {
            TimestampLogMs = TInstant::MilliSeconds(wonderlog.GetServerTimeMs());
        }
        if (wonderlog.GetAsr().GetTopics().HasRequest() && !AliceTopic(wonderlog.GetAsr().GetTopics().GetRequest())) {
            return;
        }
        // Generating fake connect_session_id
        // To see if the id is generated one can check present_in_uniproxy_logs flag
        if (wonderlog.HasSpeechkitRequest() && wonderlog.HasSpeechkitResponse()) {
            if (wonderlog.GetSpeechkitRequest().GetHeader().GetSessionId().empty()) {
                if (!connectSessionId) {
                    connectSessionId = CreateGuidAsString();
                }
                wonderlog.MutableSpeechkitRequest()->MutableHeader()->SetSessionId(*connectSessionId);
                wonderlog.MutableSpeechkitResponse()->MutableHeader()->SetSessionId(*connectSessionId);
            }
            if (wonderlog.GetSpeechkitRequest().GetHeader().GetRefMessageId().empty()) {
                wonderlog.MutableSpeechkitRequest()->MutableHeader()->SetRefMessageId(wonderlog.GetMessageId());
                wonderlog.MutableSpeechkitResponse()->MutableHeader()->SetRefMessageId(wonderlog.GetMessageId());
            }
        } else if (TimestampLogMs &&
                   SkipWithoutMegamindData(*TimestampLogMs, TimestampFrom, TimestampTo, RequestsShift)) {
            return;
        }
        if (Geobase) {
            TMaybe<TString> ip;
            if (wonderlog.GetDownloadingInfo().GetUniproxy().HasClientIp()) {
                const auto& uniproxyIp = wonderlog.GetDownloadingInfo().GetUniproxy().GetClientIp();
                if (IsIpValid(uniproxyIp)) {
                    const auto ipTraits = Geobase->GetBasicTraitsByIp(uniproxyIp);
                    wonderlog.MutableDownloadingInfo()->MutableUniproxy()->SetYandexNet(ipTraits.IsYandexNet());
                    wonderlog.MutableDownloadingInfo()->MutableUniproxy()->SetStaffNet(ipTraits.IsYandexStaff());
                    wonderlog.MutableDownloadingInfo()->SetUniproxyClientIp(uniproxyIp);
                    wonderlog.MutableDownloadingInfo()->SetUniproxyYandexIp(ipTraits.IsYandexNet());
                    ip = uniproxyIp;
                } else {
                    logError(TWonderlog::TError::R_INVALID_VALUE,
                             TStringBuilder{} << "Got invalid ip from uniproxy: " << uniproxyIp);
                }
            }
            if (wonderlog.GetDownloadingInfo().GetMegamind().HasClientIp()) {
                const auto& megamindIp = wonderlog.GetDownloadingInfo().GetMegamind().GetClientIp();
                if (IsIpValid(megamindIp)) {
                    const auto ipTraits = Geobase->GetBasicTraitsByIp(megamindIp);
                    wonderlog.MutableDownloadingInfo()->MutableMegamind()->SetYandexNet(ipTraits.IsYandexNet());
                    wonderlog.MutableDownloadingInfo()->MutableMegamind()->SetStaffNet(ipTraits.IsYandexStaff());
                    wonderlog.MutableDownloadingInfo()->SetMegamindClientIp(megamindIp);
                    wonderlog.MutableDownloadingInfo()->SetMegamindYandexIp(ipTraits.IsYandexNet());
                    ip = megamindIp;
                } else {
                    logError(TWonderlog::TError::R_INVALID_VALUE,
                             TStringBuilder{} << "Got invalid ip from megamind: " << megamindIp);
                }
            }
            if (ip) {
                const auto region = NImpl::GetRegion(Geobase.Get(), *ip);
                wonderlog.MutablePrivacy()->MutableGeoRestrictions()->SetRegion(region);
                wonderlog.MutablePrivacy()->MutableGeoRestrictions()->SetProhibitedByRegion(
                    NImpl::ProhibitedRegion(region));
            }
        }
        const TAccountTypeInfo accountTypeInfo(wonderlog);
        wonderlog.MutableClient()->SetType(accountTypeInfo.Type());

        if (wonderlog.GetSpeechkitResponse().HasContentProperties()) {
            *wonderlog.MutablePrivacy()->MutableContentProperties() =
                wonderlog.GetSpeechkitResponse().GetContentProperties();
        }
        if (wonderlog.GetSpeechkitResponse().HasContainsSensitiveData()) {
            const auto containsSensitiveData = wonderlog.GetSpeechkitResponse().GetContainsSensitiveData();
            wonderlog.MutablePrivacy()->MutableContentProperties()->SetContainsSensitiveDataInRequest(
                containsSensitiveData);
            wonderlog.MutablePrivacy()->MutableContentProperties()->SetContainsSensitiveDataInResponse(
                containsSensitiveData);
        }
        wonderlog.MutablePrivacy()->SetOriginalDoNotUseUserLogs(originalDoNotUseUserLogs);
        wonderlog.MutablePrivacy()->SetDoNotUseUserLogs(NImpl::GetDoNotUseUserLogs(wonderlog.GetPrivacy()));
        const auto& environment = wonderlog.GetEnvironment();
        const TMaybe<TStringBuf> uniproxyQloudProject = environment.GetUniproxyEnvironment().HasQloudProject()
                                                            ? environment.GetUniproxyEnvironment().GetQloudProject()
                                                            : TMaybe<TStringBuf>{};
        const TMaybe<TStringBuf> uniproxyQloudApplication =
            environment.GetUniproxyEnvironment().HasQloudApplication()
                ? environment.GetUniproxyEnvironment().GetQloudApplication()
                : TMaybe<TStringBuf>{};
        const TMaybe<TStringBuf> megamindEnvironment = environment.GetMegamindEnvironment().HasEnvironment()
                                                           ? environment.GetMegamindEnvironment().GetEnvironment()
                                                           : TMaybe<TStringBuf>{};

        if (TWonderlog::AT_ROBOT == wonderlog.GetClient().GetType() ||
            !Environment.SuitableEnvironment(uniproxyQloudProject, uniproxyQloudApplication, megamindEnvironment)) {
            writer->AddRow(wonderlog, TInputOutputTables::ROBOT_WONDERLOGS_INDEX);
        } else {
            if (NImpl::DoCensor(wonderlog.GetPrivacy())) {
                writer->AddRow(wonderlog, TInputOutputTables::PRIVATE_WONDERLOGS_INDEX);
                const auto flags = NImpl::CensorFlags(wonderlog.GetPrivacy());
                Censor.ProcessMessage(flags, wonderlog);
            }
            writer->AddRow(wonderlog, TInputOutputTables::WONDERLOGS_INDEX);
        }
    }

private:
    TInstant TimestampFrom;
    TInstant TimestampTo;
    TDuration RequestsShift;
    NAlice::TCensor Censor;
    TString GeobasePath;
    THolder<NGeobase::TLookup> Geobase;
    TEnvironment Environment;
};

class TPrivateWonderlogsMapper : public NYT::IMapper<NYT::TTableReader<TWonderlog>, NYT::TTableWriter<TWonderlog>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TString& wonderlogsTable, const TString& outputTable,
                           const NYT::TSortColumns& sortColumns)
            : WonderlogsTable(wonderlogsTable)
            , OutputTable(outputTable)
            , SortColumns(sortColumns) {
        }
        NYT::TMapOperationSpec AddToOperationSpec(NYT::TMapOperationSpec&& operationSpec) {
            return operationSpec.AddInput<TWonderlog>(WonderlogsTable)
                .AddOutput<TWonderlog>(NYT::TRichYPath(OutputTable)
                                           .Schema(NYT::CreateTableSchema<TWonderlog>(SortColumns))
                                           .CompressionCodec("brotli_8")
                                           .ErasureCodec(NYT::EErasureCodecAttr::EC_LRC_12_2_2_ATTR)
                                           .OptimizeFor(NYT::EOptimizeForAttr::OF_SCAN_ATTR));
        }

    private:
        const TString& WonderlogsTable;
        const TString& OutputTable;
        const NYT::TSortColumns& SortColumns;
    };

    Y_SAVELOAD_JOB(PrivateUsers);
    TPrivateWonderlogsMapper() = default;
    TPrivateWonderlogsMapper(THashMap<TString, i64> privateUsers)
        : PrivateUsers(std::move(privateUsers)) {
    }

    void Do(TReader* reader, TWriter* writer) override {
        for (auto& cursor : *reader) {
            auto wonderlog = cursor.GetRow();
            if (wonderlog.GetSpeechkitRequest().GetRequest().GetAdditionalOptions().HasPuid()) {
                if (const auto* privateUntilTimeMs = PrivateUsers.FindPtr(
                        wonderlog.GetSpeechkitRequest().GetRequest().GetAdditionalOptions().GetPuid())) {
                    if (wonderlog.GetServerTimeMs() <= *privateUntilTimeMs) {
                        const auto flags = NAlice::TCensor::TFlags{NAlice::EAccess::A_PRIVATE_REQUEST} |
                                           NAlice::EAccess::A_PRIVATE_RESPONSE;
                        Censor.ProcessMessage(flags, wonderlog);
                        wonderlog.MutablePrivacy()->SetProhibitedByGdpr(true);
                        wonderlog.MutablePrivacy()->SetDoNotUseUserLogs(true);
                    }
                }
            }
            writer->AddRow(wonderlog);
        }
    }

private:
    NAlice::TCensor Censor;
    THashMap<TString, i64> PrivateUsers;
};

class TWonderlogsSchemaChangeMapper
    : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TString& wonderlogsTable, NYT::TTableSchema wonderlogsSchema,
                           const TString& outputTable)
            : WonderlogsTable(wonderlogsTable)
            , WonderlogsSchema(wonderlogsSchema)
            , OutputTable(outputTable) {
        }
        NYT::TMapOperationSpec AddToOperationSpec(NYT::TMapOperationSpec&& operationSpec) {
            return operationSpec.AddInput<NYT::TNode>(WonderlogsTable)
                .AddOutput<NYT::TNode>(NYT::TRichYPath(OutputTable)
                                           .Schema(NImpl::ChangeSchema(
                                               WonderlogsSchema, {{"speechkit_request", "request", "device_state"}}))
                                           .CompressionCodec("brotli_8")
                                           .ErasureCodec(NYT::EErasureCodecAttr::EC_LRC_12_2_2_ATTR)
                                           .OptimizeFor(NYT::EOptimizeForAttr::OF_SCAN_ATTR));
        }

    private:
        const TString& WonderlogsTable;
        const NYT::TTableSchema WonderlogsSchema;
        const TString& OutputTable;
    };

    void Do(TReader* reader, TWriter* writer) override {
        for (auto& cursor : *reader) {
            auto wonderlog = NImpl::MoveToColumns(cursor.GetRow(), {{"speechkit_request", "request", "device_state"}});
            writer->AddRow(wonderlog);
        }
    }
};

REGISTER_REDUCER(TWonderlogsReducer)
REGISTER_MAPPER(TPrivateWonderlogsMapper)
REGISTER_MAPPER(TWonderlogsSchemaChangeMapper)

void CensorWonderlogsTable(NYT::IClientPtr client, const TString& /* tmpDirectory */, const TString& wonderlogsTable,
                           const THashMap<TString, i64>& privateUsers, const TString& outputTable) {
    auto tx = client->StartTransaction();

    CreateTable(tx, outputTable, /* expirationDate= */ {});

    NYT::TTableSchema schema;
    NYT::Deserialize(schema, client->Get(wonderlogsTable + "/@schema"));
    NYT::TSortColumns sortColumns;
    for (const auto& col : schema.Columns()) {
        if (col.SortOrder()) {
            sortColumns.Add(NYT::TSortColumn{col.Name(), *col.SortOrder()});
        }
    }

    tx->Map(TPrivateWonderlogsMapper::TInputOutputTables{wonderlogsTable, outputTable, sortColumns}
                .AddToOperationSpec(NYT::TMapOperationSpec{})
                .Ordered(true)
                .MapperSpec(NYT::TUserJobSpec{}.MemoryLimit(1_GB)),
            new TPrivateWonderlogsMapper{privateUsers}, NYT::TOperationOptions{});

    tx->Commit();
}

} // namespace

namespace NAlice::NWonderlogs {

void MakeWonderlogs(NYT::IClientPtr client, const TString& tmpDirectory, const TString& uniproxyPrepared,
                    const TString& megamindPrepared, const TString& asrPrepared, const TString& outputTable,
                    const TString& privateOutputTable, const TString& robotOutputTable, const TString& errorTable,
                    const TInstant& timestampFrom, const TInstant& timestampTo, const TDuration& requestsShift,
                    const TEnvironment& productionEnvironment, const TString& geodataPath) {
    auto tx = client->StartTransaction();
    const auto uniproxyPreparedTmpTableSorted = CreateRandomTable(tx, tmpDirectory, "uniproxy-prepared-sorted");
    const auto megamindPreparedTmpTableSorted = CreateRandomTable(tx, tmpDirectory, "megamind-prepared-sorted");
    const auto asrPreparedTmpTableSorted = CreateRandomTable(tx, tmpDirectory, "asr-prepared-sorted");

    CreateTable(tx, outputTable, /* expirationDate= */ {});
    CreateTable(tx, privateOutputTable, /* expirationDate= */ {});
    CreateTable(tx, robotOutputTable, /* expirationDate= */ {});
    CreateTable(tx, errorTable, TInstant::Now() + MONTH_TTL);

    {
        auto opearationOptions = NYT::TOperationOptions{};
        auto spec =
            TWonderlogsReducer::TInputOutputTables{uniproxyPrepared,   megamindPrepared, asrPrepared, outputTable,
                                                   privateOutputTable, robotOutputTable, errorTable}
                .AddToOperationSpec(NYT::TReduceOperationSpec{})
                .ReducerSpec(NYT::TUserJobSpec{}.MemoryLimit(1_GB).MemoryReserveFactor(5))
                .ReduceBy({"uuid", "message_id"});

        const TString geobaseLayer = "//porto_layers/delta/geobase/layer_with_geodata_lastest.tar.gz";

        if (tx->Exists(geobaseLayer)) {
            opearationOptions = opearationOptions.Spec(NYT::TNode{}(
                "reducer",
                NYT::TNode{}(
                    "layer_paths",
                    NYT::TNode::CreateList()
                        .Add(geobaseLayer)
                        .Add("//porto_layers/base/bionic/porto_layer_search_ubuntu_bionic_app_lastest.tar.gz"))));
        } else {
            spec = spec.ReducerSpec(NYT::TUserJobSpec{}.AddLocalFile(geodataPath));
        }
        tx->Reduce(
            spec,
            new TWonderlogsReducer{timestampFrom, timestampTo, requestsShift, geodataPath, productionEnvironment},
            opearationOptions);
    }
    tx->Commit();
}

void CensorWonderlogs(NYT::IClientPtr client, const TString& tmpDirectory, const TVector<TString>& wonderlogsTables,
                      const TVector<TString>& outputTables, const TString& privateUsers, size_t threadCount) {
    auto threads = CreateThreadPool(threadCount);

    THashMap<TString, i64> privateUsersMap;
    {
        auto reader = client->CreateTableReader<TPrivateUser>(privateUsers);
        for (auto& cursor : *reader) {
            const auto& privateUser = cursor.GetRow();
            privateUsersMap[privateUser.GetPuid()] = privateUser.GetPrivateUntilTimeMs();
        }
    }

    TVector<NThreading::TFuture<void>> futures;
    {
        size_t count = wonderlogsTables.size();
        for (size_t i = 0; i < count; ++i) {
            futures.push_back(NThreading::Async(
                [client, &tmpDirectory, &wonderlogsTables, &outputTables, &privateUsersMap, i]() {
                    CensorWonderlogsTable(client, tmpDirectory, wonderlogsTables[i], privateUsersMap, outputTables[i]);
                    {
                        auto tx = client->StartTransaction();
                        const auto attributes = tx->Get(outputTables[i] + "/@");
                        const TMergeAttributes mergeAttribute{
                            attributes["compression_ratio"].AsDouble(),
                            attributes.HasKey("data_weight") ? attributes["data_weight"].AsInt64() : TMaybe<ui64>{},
                            attributes.HasKey("compressed_data_size") ? attributes["compressed_data_size"].AsInt64()
                                                                      : TMaybe<ui64>{},
                            attributes["erasure_codec"].AsString()};

                        NYT::TNode spec;
                        spec["combine_chunks"] = true;
                        spec["force_transform"] = true;
                        spec["data_size_per_job"] = mergeAttribute.DataSizePerJob;
                        spec["job_io"]["table_writer"]["desired_chunk_size"] = mergeAttribute.DesiredChunkSize;
                        tx->Merge(NYT::TMergeOperationSpec{}
                                      .AddInput(outputTables[i])
                                      .Output(outputTables[i])
                                      .Mode(NYT::EMergeMode::MM_SORTED),
                                  NYT::TOperationOptions().Spec(spec));
                        tx->Commit();
                    }
                },
                *threads));
        }
    }

    NThreading::NWait::WaitAllOrException(futures).Wait();
}

void ChangeWonderlogsSchema(NYT::IClientPtr client, const TString& wonderlogs, const TString& outputTable,
                            const TString& /* errorTable */) {
    CreateTable(client, outputTable, /* expirationDate= */ {});

    NYT::TTableSchema schema;
    {
        const auto schemaNode = client->Get(wonderlogs + "/@schema");
        NYT::Deserialize(schema, schemaNode);
    }

    client->Map(TWonderlogsSchemaChangeMapper::TInputOutputTables{wonderlogs, schema, outputTable}
                    .AddToOperationSpec(NYT::TMapOperationSpec{})
                    .Ordered(true)
                    .MapperSpec(NYT::TUserJobSpec{}.MemoryLimit(1_GB)),
                new TWonderlogsSchemaChangeMapper, NYT::TOperationOptions{});
}

} // namespace NAlice::NWonderlogs
