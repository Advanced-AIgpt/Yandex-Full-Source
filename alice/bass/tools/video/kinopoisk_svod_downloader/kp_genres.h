#pragma once

#include <alice/bass/libs/video_common/defs.h>

#include <util/generic/maybe.h>

TMaybe<NVideoCommon::EVideoGenre> ParseKinopoiskGenre(const TString& genreStr);
NVideoCommon::EContentType ParseKinopoiskContentType(const TString& genreStr);
