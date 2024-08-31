#include "http_utils.h"

#include <util/string/ascii.h>


namespace {

    template <typename THttpPackage>
    void AddHeaderImpl(THttpPackage& dst, TString name, TString value) {
        auto* header = dst.AddHeaders();
        Y_ASSERT(header);
        header->SetName(std::move(name));
        header->SetValue(std::move(value));
    }

}  // anonymous namespace


namespace NAlice::NCuttlefish::NAppHostServices {

    void AddHeader(NAppHostHttp::THttpRequest& dst, TString name, TString value) {
        AddHeaderImpl(dst, std::move(name), std::move(value));
    }

    bool IsHeader(const NAppHostHttp::THeader& hdr, const TStringBuf name) {
        return AsciiCompareIgnoreCase(hdr.GetName(), name) == 0;
    }

}  // namespace NAlice::NCuttlefish::NAppHostServices
