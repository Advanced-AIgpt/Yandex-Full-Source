#include "debug_md.h"

#include <alice/library/json/json.h>

#include <alice/protos/data/scenario/objects/books.pb.h>
#include <alice/protos/data/scenario/objects/companies.pb.h>
#include <alice/protos/data/scenario/objects/image.pb.h>
#include <alice/protos/data/scenario/objects/music.pb.h>
#include <alice/protos/data/scenario/objects/person.pb.h>
#include <alice/protos/data/scenario/objects/places.pb.h>
#include <alice/protos/data/scenario/objects/text.pb.h>
#include <alice/protos/data/scenario/objects/video.pb.h>

#include <google/protobuf/text_format.h>

namespace NAlice::NHollywoodFw::NSearch {

namespace {

template <typename TProtoArray, typename TProtoItem>
TString DumpAsArray(const TProtoArray& arr, const TString& (TProtoItem::*fn)() const, bool headRow = false) {
    TString result;
    result.append("\n| ");
    for (const auto& it : arr) {
        result.append((it.*fn)());
        result.append(" | ");
    }
    result.append("\n");
    if (headRow) {
        result.append("\n| ");
        for (const auto& it : arr) {
            Y_UNUSED(it);
            result.append("-------- | ");
        }
        result.append("\n");
    }
    return result;
}

template <typename TProtoArray, typename TProtoItem>
TString DumpAsArray(const TProtoArray& arr, int (TProtoItem::*fn)() const, TStringBuf prefix) {
    TString result;
    result.append("\n| ");
    for (const auto& it : arr) {
        TStringBuilder builder;
        builder << prefix << (it.*fn)();
        result.append(builder);
        result.append(" | ");
    }
    result.append("\n");
    return result;
}

TString DumpMdSimpleText(const NData::TSimpleText& text) {
    TString result = "--type: simpletext--\n\n";
    result.append(text.GetText());
    return result;
}

TString DumpMdFactList(const NData::TFactList& factlist) {
    TString result = "--type: factlist--\n\n| key | value |\n| -- | -- |\n| ";

    for (const auto& it : factlist.GetFacts()) {
        if (it.HasTextAnswer()) {
            result.append(TStringBuilder{} << it.GetFactText() << " | " << it.GetTextAnswer() << " |\n");
        } else if (it.HasMultiTextAnswer()) {
            TString multifact;
            for (const auto& it2 : it.GetMultiTextAnswer().GetTextAnswer()) {
                multifact.append(it2.GetText());
                multifact.append(", ");
            }
            result.append(TStringBuilder{} << it.GetFactText() << " | " << multifact << " |\n");
        }
    }
    return result;
}

TString DumpMdGallery(const NData::TImageGallery& imgg) {
    TString result = "--type: gallery--\n";
    result.append(DumpAsArray(imgg.GetImages(), &NData::TSingleImage::GetText, true));
    result.append("\n| ");
    for (const auto& it : imgg.GetImages()) {
        result.append(TStringBuilder{} << "100x100:" << it.GetUrlAvatar().GetUrl() << " | ");
    }
    return result;
}

TString DumpMdMovies(const NData::TVideoGallery& movies, TStringBuf type) {
    TString result = "--type: ";
    result.append(type);
    result.append("--\n");

    result.append(DumpAsArray(movies.GetVideos(), &NData::TVideoObject::GetTitle, true));
    result.append("\n| ");
    for (const auto& it : movies.GetVideos()) {
        result.append(TStringBuilder{} << "100x100:" << it.GetLogo().GetUrl() << " | ");
    }
    result.append("\n");
    result.append(DumpAsArray(movies.GetVideos(), &NData::TVideoObject::GetReleaseYear, "Release year: "));
    result.append(DumpAsArray(movies.GetVideos(), &NData::TVideoObject::GetAgeLimit, "Age limit: "));
    result.append(DumpAsArray(movies.GetVideos(), &NData::TVideoObject::GetRating));
    return result;
}

TString DumpMdPersons(const NData::TPersons& perss) {
    TString result = "--type: persons--\n\n";
    result.append(DumpAsArray(perss.GetPersones(), &NData::TPerson::GetName, true));
    result.append("\n| ");
    for (const auto& it : perss.GetPersones()) {
        result.append(TStringBuilder{} << "100x100:" << it.GetImage().GetUrl() << " | ");
    }
    result.append(DumpAsArray(perss.GetPersones(), &NData::TPerson::GetDescription));
    return result;
}

TString DumpMdBands(const NData::TMusicBands& bands, TStringBuf type) {
    TString result = "--type: ";
    result.append(type);
    result.append("--\n");
    result.append(DumpAsArray(bands.GetBands(), &NData::TMusicBand::GetTitle, true));
    result.append(DumpAsArray(bands.GetBands(), &NData::TMusicBand::GetSubtitle));
    result.append("\n| ");
    for (const auto& it : bands.GetBands()) {
        result.append(TStringBuilder{} << "100x100:" << it.GetLogo().GetUrl() << " | ");
    }
    result.append("\n");
    result.append(DumpAsArray(bands.GetBands(), &NData::TMusicBand::GetYear, "Year: "));
    return result;
}

TString DumpMdTracks(const NData::TMusicTracks& tracks, TStringBuf type) {
    TString result = "--type: ";
    result.append(type);
    result.append("--\n");

    result.append(DumpAsArray(tracks.GetTracks(), &NData::TTrack::GetTitle, true));
    result.append("\n| ");
    for (const auto& it : tracks.GetTracks()) {
        result.append(TStringBuilder{} << "100x100:" << it.GetArtImageUrl() << " | ");
    }
    result.append("\n");
    return result;
}

TString DumpMdAlbums(const NData::TMusicAlbums& albums, TStringBuf type) {
    TString result = "--type: ";
    result.append(type);
    result.append("--\n");

    result.append(DumpAsArray(albums.GetAlbums(), &NData::TAlbum::GetTitle, true));
    result.append("\n| ");
    for (const auto& it : albums.GetAlbums()) {
        result.append(TStringBuilder{} << "100x100:" << it.GetCoverUri() << " | ");
    }
    result.append(DumpAsArray(albums.GetAlbums(), &NData::TAlbum::GetReleaseYear, "Release year: "));
    return result;
}

TString DumpMdPlaces(const NData::TGeoPlaces& places) {
    TString result = "--type: places";
    result.append("--\n");

    result.append(DumpAsArray(places.GetPlaces(), &NData::TGeoPlace::GetName, true));
    result.append(DumpAsArray(places.GetPlaces(), &NData::TGeoPlace::GetDescription));
    result.append(DumpAsArray(places.GetPlaces(), &NData::TGeoPlace::GetRating));
    return result;
}

TString DumpMdBooks(const NData::TBooks& books) {
    TString result = "--type: books";
    result.append("--\n");

    result.append(DumpAsArray(books.GetBooks(), &NData::TBook::GetName, true));
    result.append(DumpAsArray(books.GetBooks(), &NData::TBook::GetReleaseYear, "Release year: "));
    result.append(DumpAsArray(books.GetBooks(), &NData::TBook::GetRating));
    result.append("\n| ");
    return result;
}

TString DumpMdCompanies(const NData::TCompanies& companies) {
    TString result = "--type: places";
    result.append("--\n");

    result.append(DumpAsArray(companies.GetCompanies(), &NData::TCompany::GetName, true));
    result.append(DumpAsArray(companies.GetCompanies(), &NData::TCompany::GetDescription));
    result.append(DumpAsArray(companies.GetCompanies(), &NData::TCompany::GetRating));
    return result;
}

} // anonymous namespace

/*
    Dump NData::TSearchRichCardData in MDcompatible format
    This function CAN NOT BE USED IN PRODUCTION CODE!
*/
void DumpAsMd(TRTLogger& logger, const NData::TSearchRichCardData& richCard, bool asProto /*= false*/) {
    if (!logger.IsSuitable(ELogPriority::TLOG_DEBUG)) {
        return;
    }

    TString outputString = "MD_RICHCARD:\n```\n";
    if (asProto) {
        TProtoStringType output;
        NProtoBuf::TextFormat::PrintToString(richCard, &output);
        outputString.append(output);
    } else {
        outputString.append(JsonStringFromProto(richCard));
    }
    outputString.append("\n```\n\n");

    // Main title
    outputString.append(TStringBuilder{} << "==" << richCard.GetHeader().GetText() << "==\n");
    outputString.append(TStringBuilder{} << "===" << richCard.GetHeader().GetExtraText() << "===\n");
    if (!richCard.GetHeader().GetImage().GetUrl().Empty()) {
        outputString.append(TStringBuilder{} << "100x100:" << richCard.GetHeader().GetImage().GetUrl() << "\n\n");
    }
    if (!richCard.GetHeader().GetSearchUrl().Empty()) {
        outputString.append(TStringBuilder{} << "~~SearchUrl:~~ " << richCard.GetHeader().GetSearchUrl() << "\n\n");
    }
    if (!richCard.GetHeader().GetUrl().Empty()) {
        outputString.append(TStringBuilder{} << "~~Url:~~ " << richCard.GetHeader().GetUrl() << "\n\n");
    }
    if (!richCard.GetHeader().GetHostname().Empty()) {
        outputString.append(TStringBuilder{} << "~~Hostname:~~ " << richCard.GetHeader().GetHostname() << "\n\n");
    }
    if (!richCard.GetHeader().GetTitle().Empty()) {
        outputString.append(TStringBuilder{} << "~~Title:~~ " << richCard.GetHeader().GetTitle() << "\n\n");
    }

    // Navigation pane
    for (const auto& it: richCard.GetBlocks()) {
        if (it.GetHidden() || it.GetTitleNavigation().Empty()) {
            continue;
        }
        outputString.append(TStringBuilder{} << "[ *" << it.GetTitleNavigation() << "* ] ");
    }
    outputString.append("\n");
    // All blocks
    for (const auto& it: richCard.GetBlocks()) {
        if (it.GetHidden()) {
            continue;
        }
        outputString.append("-------------------------------------------------------------\n");
        if (!it.GetTitle().Empty()) {
            outputString.append(TStringBuilder{} << "====" << it.GetTitle() << "====\n");
        }
        for (const auto& section : it.GetSections()) {
            if (section.GetHidden()) {
                continue;
            }
            if (section.HasText()) {
                outputString.append(DumpMdSimpleText(section.GetText()));
            } else if (section.HasFactList()) {
                outputString.append(DumpMdFactList(section.GetFactList()));
            } else if (section.HasGallery()) {
                outputString.append(DumpMdGallery(section.GetGallery()));
            } else if (section.HasVideoClips()) {
                outputString.append(DumpMdMovies(section.GetVideoClips(), "clips"));
            } else if (section.HasVideoMovies()) {
                outputString.append(DumpMdMovies(section.GetVideoMovies(), "movies"));
            } else if (section.HasMusicAlbums()) {
                outputString.append(DumpMdAlbums(section.GetMusicAlbums(), "albums"));
            } else if (section.HasMusicTracks()) {
                outputString.append(DumpMdTracks(section.GetMusicTracks(), "tracks"));
            } else if (section.HasMusicBands()) {
                outputString.append(DumpMdBands(section.GetMusicBands(), "bands"));
            } else if (section.HasPersons()) {
                outputString.append(DumpMdPersons(section.GetPersons()));
            } else if (section.HasGeoPlaces()) {
                outputString.append(DumpMdPlaces(section.GetGeoPlaces()));
            } else if (section.HasBooks()) {
                outputString.append(DumpMdBooks(section.GetBooks()));
            } else if (section.HasCompanies()) {
                outputString.append(DumpMdCompanies(section.GetCompanies()));
            }
        }
    }
    LOG_DEBUG(logger) << outputString;
}

} // namespace NAlice::NHollywoodFw::NSearch
