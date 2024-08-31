#include "utils.h"

#include <util/string/builder.h>

namespace NAlice::NWonderlogs {

void CreateTable(NYT::IClientBasePtr client, const TString& path, const TMaybe<TInstant>& expirationDate) {
    client->Remove(path, NYT::TRemoveOptions().Recursive(true).Force(true));
    auto options = NYT::TCreateOptions().Recursive(true);
    if (expirationDate) {
        options = options.Attributes(NYT::TNode()("expiration_time", expirationDate->MilliSeconds()));
    }
    client->Create(path, NYT::ENodeType::NT_TABLE, options);
}

TString CreateRandomTable(NYT::IClientBasePtr client, TStringBuf directory, TStringBuf prefix, const bool tempTable,
                          const TDuration ttl) {
    TString path = TStringBuilder{} << directory << (!directory.empty() && directory.back() == '/' ? "" : "/")
                                    << prefix << "-" << CreateGuidAsString();
    CreateTable(client, path, tempTable ? TInstant::Now() + ttl : TMaybe<TInstant>{});
    return path;
}

TMergeAttributes::TMergeAttributes(double compressionRatio, const TMaybe<ui64>& dataWeight,
                                   const TMaybe<ui64>& compressedDataSize, const TString& erasureCodec) {
    // https://a.yandex-team.ru/arc/trunk/arcadia/yt/cron/merge/perform_merge/__main__.py?rev=r7888713#L78-100
    if (dataWeight && compressedDataSize) {
        compressionRatio = static_cast<double>(*compressedDataSize) / *dataWeight;
    }
    DesiredChunkSize = 512ul * 1024ul * 1024ul;
    if (erasureCodec != "none") {
        DesiredChunkSize = 2ul * 1024ul * 1024ul * 1024ul;
    }
    DataSizePerJob = std::min(32ul * 1024ul * 1024ul * 1024ul,
                              std::max(1ul, static_cast<ui64>(DesiredChunkSize / compressionRatio)));
}

} // namespace NAlice::NWonderlogs
