#include "request_builder.h"

namespace NAlice::NMegamind {

NAppHostHttp::THttpRequest& TAppHostHttpProxyMegamindRequestBuilder::CreateAndPushRequest(IAppHostCtx& ahCtx, TStringBuf itemName, TStringBuf tagName /*={}*/) {
    auto& item = CreateRequest();
    if (tagName.Empty()) {
        LOG_INFO(ahCtx.Log()) << "AppHost Http proxy item '" << itemName << "' path '" << item.GetPath() << '\'';
    } else {
        LOG_INFO(ahCtx.Log()) << TLogMessageTag{TString{tagName}} << "AppHost Http proxy item '" << itemName << "' path '" << item.GetPath() << '\'';
    }
    ahCtx.ItemProxyAdapter().PutIntoContext(item, itemName);
    return item;
}

} // namespace NAlice::NMegamind
