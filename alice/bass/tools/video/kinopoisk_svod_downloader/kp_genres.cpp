#include "kp_genres.h"

namespace {

using namespace NVideoCommon;

const THashMap<TString, NVideoCommon::EVideoGenre> GENRES = {{
    {"боевик", EVideoGenre::Action},
    {"для взрослых", EVideoGenre::Adult},
    {"приключения", EVideoGenre::Adventure},
    {"биография", EVideoGenre::Biopic},
    {"комедия", EVideoGenre::Comedy},
    {"криминал", EVideoGenre::Crime},
    {"документальный", EVideoGenre::Documentary},
    {"драма", EVideoGenre::Drama},
    {"семейный", EVideoGenre::Family},
    {"фэнтези", EVideoGenre::Fantasy},
    {"фильм-нуар", EVideoGenre::Noir},
    {"история", EVideoGenre::Historical},
    {"ужасы", EVideoGenre::Horror},
    {"мюзикл", EVideoGenre::Musical},
    {"детектив", EVideoGenre::Detective},
    {"мелодрама", EVideoGenre::Melodramas},
    {"фантастика", EVideoGenre::ScienceFiction},
    {"спорт", EVideoGenre::SportVideo},
    {"триллер", EVideoGenre::Thriller},
    {"военный", EVideoGenre::War},
    {"вестерн", EVideoGenre::Westerns},
    {"аниме", EVideoGenre::Anime},
    {"концерт", EVideoGenre::Concert},
    {"детский", EVideoGenre::Childrens},
}};

} // namespace

TMaybe<NVideoCommon::EVideoGenre> ParseKinopoiskGenre(const TString& genreStr) {
    if (auto genre = GENRES.FindPtr(genreStr)) {
        return *genre;
    }
    return Nothing();
}

NVideoCommon::EContentType ParseKinopoiskContentType(const TString& genreStr) {
    if (genreStr == "мультфильм") {
        return EContentType::Cartoon;
    }
    return EContentType::Movie;
}
