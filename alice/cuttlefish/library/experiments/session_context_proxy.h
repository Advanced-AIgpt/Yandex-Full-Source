#pragma once
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <util/generic/va_args.h>

#define _SC_PROXY_GET(X)         Get##X().
#define _SC_PROXY_GET_LAST(X)    Get##X();
#define _SC_PROXY_SET(X)         Mutable##X()->
#define _SC_PROXY_SET_LAST(X)    Set##X(value);

#define MAKE_SC_PROXY(NAME, ...) \
    inline const TString& Get##NAME(const NAliceProtocol::TSessionContext& sc) { \
        return sc.Y_MAP_ARGS_WITH_LAST(_SC_PROXY_GET, _SC_PROXY_GET_LAST, __VA_ARGS__) \
    } \
    inline void Set##NAME(NAliceProtocol::TSessionContext& sc, const TString& value) { \
        return sc.Y_MAP_ARGS_WITH_LAST(_SC_PROXY_SET, _SC_PROXY_SET_LAST, __VA_ARGS__) \
    }

/** NOTE: locations of needed fields in TSessionContext may be changed, hence there are
 * functions that wrap access to them
 */
namespace NVoice::NExperiments {

MAKE_SC_PROXY(Lang,         Language)
MAKE_SC_PROXY(VinsUrl,      SourceRewrite, VINS)
MAKE_SC_PROXY(StaffLogin,   UserInfo, StaffLogin)
MAKE_SC_PROXY(Uuid,         UserInfo, Uuid)
MAKE_SC_PROXY(AppToken,     AppToken)
MAKE_SC_PROXY(AppId,        AppId)
MAKE_SC_PROXY(DeviceModel,  DeviceInfo, DeviceModel)


using TSessionContextGetter = const TString&(*)(const NAliceProtocol::TSessionContext&);

TSessionContextGetter GetSessionContextGetter(const TStringBuf path);

} // namespace NVoice::NExperiments
