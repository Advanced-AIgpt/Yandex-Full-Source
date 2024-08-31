#pragma once

#include <alice/matrix/library/ydb/storage.h>

#include <alice/megamind/protos/speechkit/directives.pb.h>

#include <alice/protos/api/notificator/api.pb.h>

namespace NMatrix::NNotificator {

class TDirectivesStorage: public IYDBStorage {
public:
    struct TUserDevice {
        TString Puid;
        TString DeviceId;
    };

    struct TDirective {
        TString PushId;
        NAlice::NSpeechKit::TDirective SpeechKitDirective;
    };

    struct TUserDirective {
        TUserDevice UserDevice;
        TDirective Directive;
    };

    struct TGetDirectivesMultiUserDevicesResult {
        TVector<TUserDirective> UserDirectives;
        bool IsTruncated;
    };

public:
    TDirectivesStorage(
        const NYdb::TDriver& driver,
        const TYDBClientSettings& config
    );

    NThreading::TFuture<TExpected<TVector<TDirective>, TString>> GetDirectives(
        const TUserDevice& userDevice,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<TGetDirectivesMultiUserDevicesResult, TString>> GetDirectivesMultiUserDevices(
        const TVector<TUserDevice>& userDevices,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<NAlice::NNotificator::EDirectiveStatus, TString>> GetDirectiveStatus(
        const NAlice::NNotificator::TDirectiveStatus& msg,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<void, TString>> AddDirective(
        const TUserDevice& userDevice,
        const TDirective& directive,
        const TDuration ttl,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<void, TString>> ChangeDirectivesStatus(
        const NAlice::NNotificator::TChangeStatus& msg,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<void, TString>> RemoveAllUserData(
        const TString& puid,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

private:
    // Inherit logic from python notificator
    // expiredAt = realExpiredAt + DIRECTIVE_TTL_AFTER_EXPIRATION
    // This was done in order to be able to get the state of the directive within the DIRECTIVE_TTL_AFTER_EXPIRATION
    // after expiration
    static TInstant GetDirectiveExpiredAt(TInstant realExpiredAt);

private:
    static inline constexpr TStringBuf NAME = "directives";

    // Constants as is from python notificator
    static inline constexpr TDuration MAX_DIRECTIVE_TTL = TDuration::Days(1);
    static inline constexpr TDuration DIRECTIVE_TTL_AFTER_EXPIRATION = TDuration::Days(1);

    static inline constexpr ui32 MAX_DIRECTIVES_IN_GET_DIRECTIVES_MULTI_USER_DEVICES = 1000;
};

} // namespace NMatrix::NNotificator
