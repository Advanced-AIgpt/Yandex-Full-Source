#pragma once

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/string/builder.h>


namespace NAlice::NCuttlefish::NAppHostServices {
    class TBalancingHintHolder {

    public:
        TBalancingHintHolder(TString geo, TString ctype);

        TBalancingHintHolder();

        TMaybe<TStringBuf> GetBalancingHint(TStringBuf mode);

        static void AddBalancingHint(TStringBuf mode, TStringBuilder& headers);

    private:
        TMaybe<TString> Geo_;
        TMaybe<TString> GeoPre_;
    };
}
