#include <alice/bass/forms/market/client.h>

#include <util/string/split.h>

namespace NBASS {

namespace NMarket {

// todo rename file to base_redirect.cpp/h
TReportResponse::TBaseRedirect::TBaseRedirect(NSc::TValue data)
    : Data(data)
{
    const auto& params = Data["redirect"]["params"];
    Text = params["text"][0].ForceString();
    SuggestText = params["suggest_text"][0].ForceString();
    SetGlFilters(params["glfilter"]);
    CgiParams = TRedirectCgiParams(params["rs"][0].ForceString(), params["was_redir"][0].ForceString());
}

const TString& TReportResponse::TBaseRedirect::GetText() const
{
    return Text;
}

const TString& TReportResponse::TBaseRedirect::GetSuggestText() const
{
    return SuggestText;
}

const TCgiGlFilters& TReportResponse::TBaseRedirect::GetGlFilters() const
{
    return GlFilters;
}

const TRedirectCgiParams& TReportResponse::TBaseRedirect::GetCgiParams() const
{
    return CgiParams;
}

void TReportResponse::TBaseRedirect::SetGlFilters(const NSc::TValue& rawFilters)
{
    if (!rawFilters.IsNull()) {
        for (const auto& filterStr : rawFilters.GetArray()) {
            TVector<TString> idWithVal{Reserve(2)};
            Split(filterStr.ForceString(), ":", idWithVal);
            if (idWithVal.size() != 2) {
                LOG(ERR) << "Got unexpected gl filter " << filterStr << ". Ignoring" << Endl;
                return;
            }

            TVector<TString> vals;
            Split(idWithVal[1], ",", vals);

            GlFilters[idWithVal[0]] = vals;
        }
    }
}

} // namespace NMarket

} // namespace NBASS
