#pragma once

#include "generic_error.h"

#include <library/cpp/http/misc/httpcodes.h>

#include <util/generic/maybe.h>

#include <utility>

namespace NBASS {

namespace NImpl {
enum class EType {
    ALARMERROR /* "alarmerror" */,
    BADREQUEST /* "bad_request" */,
    BIOMETRY /* biometry */,
    CHATSWEBDISCOVERYERROR /* "chats_web_discovery_error" */,
    COLLECTIONERROR /* "collectionerror" */,
    CONVERTERROR /* "converterror" */,
    DIRECTERROR /* "directerror" */,
    IMAGEERROR /* "imageerror" */,
    INVALIDPARAM /* "invalidparam" */,
    GENERAL_CONVERSATION_ERROR /* "general_conversation_error" */,
    MARKETERROR /* "marketerror" */,
    MUSICERROR /* "musicerror" */,
    NOCURRENTROUTE /* "nocurrentroute" */,
    NODATASYNCKEYFOUND /* "nodatasynckeyfound" */,
    NOGEOFOUND /* "nogeo" */,
    NONEWS /* "nonews" */,
    NOROUTE /* "noroute" */,
    NOSAVEDADDRESS /* "nosavedaddress" */,
    NOTIMEZONE /* "notimezone" */,
    NOTRAFFIC /* "notraffic" */,
    NOTSUPPORTED /* "notsupported" */,
    NOUSERGEO /* "nousergeo" */,
    NOWEATHER /* "noweather" */,
    NOWEATHERFOUND /* "noweather" */,
    ONBOARDINGERROR /* "onboardingerror" */,
    PLAYERERROR /* "playererror" */,
    PROTOCOL_IRRELEVANT /* protocol_irrelevant */,
    RADIOERROR /* "radioerror" */,
    REMINDERERROR /* "remindererror" */,
    SKILLDISABLED /* "external_skill_deactivated" */,
    SKILLSDISCOVERYERROR /* skills_discovery_error */,
    SKILLSERROR /* "external_skill_unavaliable" */,
    SKILLSERVICEERROR /*skill_service_error*/,
    SKILLUNKNOWN /* "external_skill_unknown" */,
    SOCIALISMERROR /* "socialismerror" */,
    SOUNDERROR /* "sounderror" */,
    SYSTEM /* "system" */,
    TAXIERROR /* "taxierror" */,
    TIMEOUT /* "timeout" */,
    TIMERERROR /* "timererror" */,
    TODOERROR /* "todoerror" */,
    TRANSLATEERROR /* "translateerror" */,
    TVERROR /* "tverror" */,
    UNAUTHORIZED /* "unauthorized" */,
    UGCDBERROR /* "ugcdberror" */,
    VHERROR /* "vh_error" */,
    VHNOLICENCE /* "vh_no_licence" */,
    VIDEOERROR /* "videoerror" */,
    VIDEOPROTOCOLERROR /* video_protocol_error */,
    WEATHERERROR /* "weathererror" */,
    SEARCHFILTERERROR /* "searchfiltererror" */,
};
}  // namespace NImpl

using TError = TGenericError<NImpl::EType>;

class TErrorException : public yexception {
public:
    template <class... TArgs>
    explicit TErrorException(TArgs&&... args)
        : Error_(std::forward<TArgs>(args)...)
    {
        static_cast<yexception&>(*this) << Error_.Msg;
    }

    const TError& Error() const {
        return Error_;
    }

private:
    TError Error_;
};

using TResultValue = TMaybe<TError>;

inline TNothing ResultSuccess() {
    return Nothing();
}

inline HttpCodes GetHttpCode(const TError& error) {
    if (error.Type == TError::EType::INVALIDPARAM)
        return HttpCodes::HTTP_BAD_REQUEST;
    if (error.Type == TError::EType::UNAUTHORIZED)
        return HttpCodes::HTTP_UNAUTHORIZED;
    return HttpCodes::HTTP_INTERNAL_SERVER_ERROR;
}

} // namespace NBASS
