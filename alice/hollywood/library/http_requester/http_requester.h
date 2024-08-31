#pragma once

#include <util/generic/hash.h>
#include <util/generic/string.h>

namespace NAlice::NHollywood {

class IHttpRequester {
public:
    enum class EMethod {
        Get = 0,
        Post = 1
    };

    virtual ~IHttpRequester() = default;

    virtual IHttpRequester& Add(
        const TString& requestId,
        EMethod method,
        const TString& url,
        const TString& body,
        const THashMap<TString, TString>& headers
    ) = 0;

    virtual IHttpRequester& Start() = 0;

    virtual TString Fetch(const TString& requestId) = 0;
};

THolder<IHttpRequester> MakeSimpleHttpRequester();

} // namespace NAlice::NHollywood
