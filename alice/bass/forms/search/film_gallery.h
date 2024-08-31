#pragma once

#include <alice/bass/forms/registrator.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>

namespace NBASS {

class TContext;

namespace NFilmGallery {

extern const TVector<TFormHandlerPair> FORM_HANDLER_PAIRS;

class TFilmGalleryBuilder {
public:
    static TMaybe<TFilmGalleryBuilder> Create(TContext& ctx);

    bool TryBuild(const NSc::TValue& searchResult);

private:
    explicit TFilmGalleryBuilder(TContext& ctx);

private:
    TContext& Ctx;
};

} // NFilmGallery
} // NBASS
