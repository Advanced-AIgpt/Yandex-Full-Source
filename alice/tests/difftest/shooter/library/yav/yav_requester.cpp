#include "yav_requester.h"

#include <alice/joker/library/log/log.h>

#include <library/cpp/neh/http_common.h>
#include <library/cpp/neh/neh.h>

#include <util/generic/string.h>

namespace NAlice::NShooter {

TString TYavRequester::Request(const TString& secretId, const TString& oauthToken) const {
    TString uri{TString::Join("https://vault-api.passport.yandex.net/1/versions/", secretId, "/")};
    TString headers{TString::Join("Authorization: ", oauthToken)};

    NNeh::TMessage msg{NNeh::TMessage::FromString(uri)};
    if (Y_UNLIKELY(!NNeh::NHttp::MakeFullRequest(msg, headers, /* content =*/ ""))) {
        ythrow yexception() << "Unable to create YAV request";
    }

    LOG(INFO) << "Request for YAV secrets: " << uri << Endl;
    NNeh::TResponseRef r = NNeh::Request(msg)->Wait();

    if (r) {
        LOG(INFO) << "Request for YAV secrets is done" << Endl;
        if (r->IsError()) {
            ythrow yexception() << "Request for YAV secrets returned error: Code \"" << r->GetErrorCode() << "\", Text " << r->GetErrorText().Quote();
        }
        return r->Data;
    }
    ythrow yexception() << "Request for YAV secrets is failed";
};

} // namespace NAlice::NShooter
