#pragma once

#include <util/generic/map.h>
#include <util/generic/strbuf.h>

namespace NMordoviaTabs {

enum class ETabName {
    Main /* "main" */,
    Movies /* "movies" */,
    Series /* "series" */,
    Cartoons /* "cartoons" */,
    TvChannels /* "tv_channels" */
};

const TMap<ETabName, TStringBuf> MORDOVIA_TAB_PATH = {
        {ETabName::Main, "/video/quasar/home/"},
        {ETabName::Movies,  "/video/quasar/home/0/movie/?offset=0&supertag=movie"},
        {ETabName::Series, "/video/quasar/home/0/series/?offset=0&supertag=series"},
        {ETabName::Cartoons, "/video/quasar/home/0/kids/?offset=0&supertag=kids"},
        {ETabName::TvChannels, "/video/quasar/channels/0/"}
};

} // namespace NMordoviaTabs
