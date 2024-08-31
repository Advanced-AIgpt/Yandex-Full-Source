#pragma once

#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/protos/wonderlogs.pb.h>

#include <library/cpp/binsaver/bin_saver.h>

#include <mapreduce/yt/interface/client.h>

#include <util/generic/hash_set.h>

namespace NAlice::NWonderlogs {

namespace NImpl {

TMaybe<bool> DoNotUseUserLogs(TMaybe<bool> doNotUseUserLogs, TMaybe<bool> prohibitedByRegion);

const auto DIALOGS_SCHEMA =
    NYT::TTableSchema()
        .AddColumn(NYT::TColumnSchema().Name("server_time_ms").Type(NYT::VT_UINT64, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("uuid").Type(NYT::VT_STRING, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("sequence_number").Type(NYT::VT_UINT64, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("analytics_info").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("app_id").Type(NYT::VT_STRING, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("biometry_classification").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("biometry_scoring").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("callback_args").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("callback_name").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("client_time").Type(NYT::VT_UINT64, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("client_tz").Type(NYT::VT_STRING, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("contains_sensitive_data").Type(NYT::VT_BOOLEAN, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("device_id").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("device_revision").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("dialog_id").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("do_not_use_user_logs").Type(NYT::VT_BOOLEAN, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("enrollment_headers").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("environment").Type(NYT::VT_STRING, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("error").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("experiments").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("form").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("form_name").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("guest_data").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("lang").Type(NYT::VT_STRING, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("location_lat").Type(NYT::VT_DOUBLE, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("location_lon").Type(NYT::VT_DOUBLE, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("message_id").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("provider").Type(NYT::VT_STRING, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("puid").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("request").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("request_id").Type(NYT::VT_STRING, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("request_stat").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("response").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("response_id").Type(NYT::VT_STRING, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("server_time").Type(NYT::VT_UINT64, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("session_id").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("session_status").Type(NYT::VT_INT32, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("type").Type(NYT::VT_STRING, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("utterance_source").Type(NYT::VT_STRING, /* required= */ false))
        // TODO unicode
        .AddColumn(NYT::TColumnSchema().Name("utterance_text").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("trash_or_empty_request").Type(NYT::VT_BOOLEAN, /* required= */ false));

void ChangeIds(NYT::TNode& dialog, const TString& newPuid, const TString& newUuid, const TString& newNormalizedUuid,
               const TString& newDeviceId);

struct TIds {
    TString Puid;
    TString Uuid;
    TString DeviceId;

    static TIds Generate(ui64 seed);
};

} // namespace NImpl

struct TBannedUsers {
    THashSet<TString> Ips;
    THashSet<TString> Uuids;
    THashSet<TString> DeviceIds;

    [[nodiscard]] bool Banned(const TWonderlog& wonderlog) const;

    Y_SAVELOAD_DEFINE(Ips, Uuids, DeviceIds);
    SAVELOAD(Ips, Uuids, DeviceIds);
};

void MakeDialogs(NYT::IClientPtr client, const TString& tmpDirectory, const TString& wonderlogs,
                 const TString& outputTable, const TString& bannedTable, const TString& errorTable,
                 const TBannedUsers& bannedUsers, const TMaybe<TEnvironment>& productionEnvironment);

void MakeDialogsMultiple(NYT::IClientPtr client, const TString& tmpDirectory, const TVector<TString>& wonderlogsTables,
                         const TVector<TString>& outputTables, const TVector<TString>& bannedTables,
                         const TVector<TString>& errorTables, size_t threadCount, const TBannedUsers& bannedUsers,
                         const TMaybe<TEnvironment>& productionEnvironment);

void CensorDialogs(NYT::IClientPtr client, const TString& tmpDirectory, const TVector<TString>& dialogsTables,
                   const TVector<TString>& outputTables, const TString& privateUsers, size_t threadCount);

} // namespace NAlice::NWonderlogs
