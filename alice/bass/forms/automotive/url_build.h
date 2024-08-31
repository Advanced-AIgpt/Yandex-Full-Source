#pragma once

#include <alice/bass/forms/context/fwd.h>
#include <alice/bass/forms/vins.h>

namespace NBASS {
namespace NAutomotive {

class TFMRadioDatabase;

template<class TDirective>
class TDirectiveBuilder {
private:
    TMap<TString, TString> ParamValue;
    TStringBuf Action;

    TString BuildIntentString() const {
        TStringBuilder yaautoIntent;
        yaautoIntent << "yandexauto://" << Action;
        if (!ParamValue.empty()) {
            yaautoIntent << "?";
            for (auto it = ParamValue.begin(), last = std::prev(ParamValue.end()); it != last; ++it) {
                yaautoIntent << it->first;
                if (!it->second.empty()) {
                    yaautoIntent << "=" << it->second << "&";
                }
            }
            yaautoIntent << ParamValue.rbegin()->first;
            if (!ParamValue.rbegin()->second.empty()) {
                yaautoIntent << "=" << ParamValue.rbegin()->second;
            }
        }
        return yaautoIntent;
    }

public:
    TDirectiveBuilder(TStringBuf action)
            : Action(action)
    {
    }

    void InsertParam(const TString& param, const TString& value) {
        ParamValue.emplace(param, value);
    }

    TResultValue AddDirective(TContext& ctx) const {
        NSc::TValue intentData;
        intentData["uri"].SetString(BuildIntentString());
        ctx.AddCommand<TDirective>(TStringBuf("open_uri"), intentData);

        return TResultValue();
    }
};

TResultValue FMRadioCommandByName(TContext& ctx, const TFMRadioDatabase& FMDB, const TString& radioKey, i32 regionId);
TResultValue FMRadioCommandByFreq(TContext& ctx, const TFMRadioDatabase& FMDB, const TString& radioFreq, i32 regionId);

} // NBASS
} // NAutomotive
