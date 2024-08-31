#pragma once

#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/protos/wonderlogs.pb.h>

#include <alice/library/censor/lib/censor.h>
#include <alice/megamind/protos/common/content_properties.pb.h>

#include <kernel/geo/utils.h>

#include <mapreduce/yt/interface/client.h>

namespace NAlice::NWonderlogs {

namespace NImpl {

bool DoCensor(const TWonderlog::TPrivacy& privacy);
TCensor::TFlags CensorFlags(const TWonderlog::TPrivacy& privacy);
TWonderlog::TPrivacy::TGeoRestrictions::ERegion GetRegion(const NGeobase::TLookup* geobase, const TString& ip);
bool ProhibitedRegion(TWonderlog::TPrivacy::TGeoRestrictions::ERegion region);
bool GetDoNotUseUserLogs(const TWonderlog::TPrivacy& privacy);
NYT::TTableSchema ChangeSchema(const NYT::TTableSchema& wonderlogsSchema, const TVector<TVector<TString>>& paths);
NYT::TNode MoveToColumns(NYT::TNode node, const TVector<TVector<TString>>& paths);

} // namespace NImpl

const TString GEODATA_DEFAULT_PATH = "/var/cache/geobase/geodata6.bin";

void MakeWonderlogs(NYT::IClientPtr client, const TString& tmpDirectory, const TString& uniproxyPrepared,
                    const TString& megamindPrepared, const TString& asrPrepared, const TString& outputTable,
                    const TString& privateOutputTable, const TString& robotOutputTable, const TString& errorTable,
                    const TInstant& timestampFrom, const TInstant& timestampTo, const TDuration& requestsShift,
                    const TEnvironment& productionEnvironment, const TString& geodataPath = GEODATA_DEFAULT_PATH);

void CensorWonderlogs(NYT::IClientPtr client, const TString& tmpDirectory, const TVector<TString>& wonderlogsTables,
                      const TVector<TString>& outputTables, const TString& privateUsers, size_t threadCount);

void ChangeWonderlogsSchema(NYT::IClientPtr client, const TString& wonderlogs, const TString& outputTable,
                            const TString& errorTable);

} // namespace NAlice::NWonderlogs
