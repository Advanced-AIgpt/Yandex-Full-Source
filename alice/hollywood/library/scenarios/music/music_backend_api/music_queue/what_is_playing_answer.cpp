#include "what_is_playing_answer.h"

#include <util/generic/vector.h>
#include <util/string/builder.h>
#include <util/string/join.h>

namespace NAlice::NHollywood::NMusic {

TString MakeWhatIsPlayingAnswer(const TQueueItem& item, const bool useTrack) {
    TStringBuilder result;

    TVector<TString> composers;
    TVector<TString> artists;
    for (const auto& artist : item.GetTrackInfo().GetArtists()) {
        if (artist.GetComposer()) {
            composers.push_back(artist.GetName());
        } else {
            artists.push_back(artist.GetName());
        }
    }

    result << JoinSeq(", ", composers);

    if (!result.Empty()) {
        result << ", ";
    }
    result << JoinSeq(", ", artists);

    if (!result.Empty() && !result.EndsWith(", ")) {
        result << ", ";
    }

    if (item.GetType() == "fm_radio") {
        result << "радио";
    } else if (item.GetType() == "generative") {
        result << "нейромузыка на станции";
    } else if (item.GetTrackInfo().GetGenre() == "classical") {
        result << "композиция";
    } else if (item.GetType() == "shot") {
        // For Shot first word does not needed
    } else {
        result << (useTrack ? "трек" : "песня");
    }
    if (!item.GetTitle().Empty()) {
        if (!result.Empty()) {
            result << ' ';
        }
        result << '"' << item.GetTitle() << '"';
    }
    return result;
}


} // namespace NAlice::NHollywood::NMusic
