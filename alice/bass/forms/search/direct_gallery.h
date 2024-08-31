#pragma once

#include <alice/bass/forms/context/context.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>

namespace NBASS::NDirectGallery {

TString ReplaceHtmlEntitiesInText(const TString& htmlText);

size_t DirectItemsCount(const NSc::TValue& searchResult);

class TDirectGalleryBuilder {
public:
    TContext* Ctx;

    static TMaybe<TDirectGalleryBuilder> Create(TContext& ctx);

    TContext::TPtr Build(const TStringBuf& query, const NSc::TValue& searchResult);

private:
    explicit TDirectGalleryBuilder(TContext& ctx);

    TMaybe<NSc::TValue> BuildDirectGalleryItem(const NSc::TValue& document);
};

void RegisterDirectGalleryContinuation(TContinuationParserRegistry& registry);

} // namespace NBASS::NDirectGallery
