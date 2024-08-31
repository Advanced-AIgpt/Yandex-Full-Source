#include "hinter.h"

#include <util/system/env.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    TBalancingHintHolder::TBalancingHintHolder(TString geo, TString ctype) {
        if ((geo == "sas" || geo == "man" || geo == "vla" || geo == "myt" || geo == "iva") &&
            (ctype == "prod" || ctype == "prestable")) {
            if (ctype == "prestable") {
                Geo_ = Nothing();
                GeoPre_ = (TStringBuilder() << geo << "-pre");
            } else if (ctype == "prod") {
                GeoPre_ = geo;
                Geo_ = std::move(geo);
            }
        } else {
            Geo_ = Nothing();
            GeoPre_ = Nothing();
        }
    }

    TBalancingHintHolder::TBalancingHintHolder():
        TBalancingHintHolder(GetEnv("UNIPROXY_CUSTOM_GEO", GetEnv("a_geo", GetEnv("a_dc"))), GetEnv("a_ctype")) {}

    TMaybe<TStringBuf> TBalancingHintHolder::GetBalancingHint(TStringBuf mode) {
        if (mode == "pre_prod") {
            return GeoPre_;
        } else if (mode == "prod") {
            return Geo_;
        }
        return Nothing();
    }

    void TBalancingHintHolder::AddBalancingHint(TStringBuf mode, TStringBuilder& headers) {
        TBalancingHintHolder *self = Singleton<TBalancingHintHolder>();
        TMaybe<TStringBuf> hint = self->GetBalancingHint(mode);
        if (hint) {
            headers << "X-Yandex-Balancing-Hint: " << hint.GetRef() << "\r\n";
        }
    }

}
