#include "content_id.h"

namespace NAlice::NHollywood::NMusic {

TMaybe<TContentId> ContentIdFromText(const TStringBuf typeStr, const TStringBuf id) {
    TContentId_EContentType type;
    if (typeStr == "artist") {
        type = TContentId_EContentType_Artist;
    } else if (typeStr == "track") {
        type = TContentId_EContentType_Track;
    } else if (typeStr == "album") {
        type = TContentId_EContentType_Album;
    } else if (typeStr == "playlist") {
        type = TContentId_EContentType_Playlist;
    } else if (typeStr == "generative") {
        type = TContentId_EContentType_Generative;
    } else if (typeStr == "radio") {
        type = TContentId_EContentType_Radio;
    } else if (typeStr == "fm_radio" || typeStr == "fmradio") {
        type = TContentId_EContentType_FmRadio;
    } else {
        return {};
    }
    TContentId rv;
    rv.SetType(type);
    if (type == TContentId_EContentType_Radio) {
        rv.AddIds(id.data(), id.size());
    } else {
        rv.SetId(id.data(), id.size());
    }
    return rv;
}

TString ContentTypeToText(TContentId::EContentType contentType) {
    TString result;
    if (contentType == TContentId_EContentType_FmRadio) {
        result = "fm_radio";
    } else {
        result = NMusic::TContentId::EContentType_Name(contentType);
        result.to_lower();
    }
    return result;
}

TString ContentTypeToNLGType(TContentId::EContentType contentType) {
    switch (contentType) {
    case TContentId_EContentType_Artist:
        return "artist";
    case TContentId_EContentType_Track:
        return "track";
    case TContentId_EContentType_Album:
        return "album";
    case TContentId_EContentType_Playlist:
        return "playlist";
    case TContentId_EContentType_Radio:
        return "filters";
    case TContentId_EContentType_Generative:
        return "generative";
    case TContentId_EContentType_FmRadio:
        return "fm_radio";
    default:
        ythrow yexception() << "Unknown contentType=" << NMusic::TContentId::EContentType_Name(contentType);
    }
}

} // namespace NAlice::NHollywood::NMusic
