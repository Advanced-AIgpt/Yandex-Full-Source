#pragma once
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <library/cpp/json/json_value.h>
#include <util/string/builder.h>


namespace NAlice::NCuttlefish::NProtoConverters {

struct TAuthTokenHandler {
    static constexpr TStringBuf OAUTH_TEAM_PREFIX = "OAuthTeam";
    static constexpr TStringBuf YAMB_AUTH_PREFIX = "YambAuth";
    static constexpr TStringBuf OAUTH_PREFIX = "OAuth";

    static void Parse(const NJson::TJsonValue& src, NAliceProtocol::TUserInfo& dst);

    template <typename WriterT>
    static void Serialize(WriterT&& w, const NAliceProtocol::TUserInfo& src) {
        TStringBuilder val;
        switch (src.GetAuthTokenType()) {
            case NAliceProtocol::TUserInfo::OAUTH_TEAM:
                val << OAUTH_TEAM_PREFIX << " ";
                break;
            case NAliceProtocol::TUserInfo::YAMB_AUTH:
                val << YAMB_AUTH_PREFIX << " ";
                break;
            case NAliceProtocol::TUserInfo::OAUTH:
                val << OAUTH_PREFIX << " ";
                break;
            default:
                break;
        }
        val << src.GetAuthToken();
        w.Value(val);
    }

    static inline bool IsSerializationNeeded(const NAliceProtocol::TUserInfo& src) {
        return src.HasAuthTokenType() && src.HasAuthToken();
    }
};

}  // namespace NAlice::NCuttlefish::NProtoConverters
