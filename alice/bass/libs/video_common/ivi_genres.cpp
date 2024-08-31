#include "ivi_genres.h"

#include "utils.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <util/generic/strbuf.h>
#include <util/string/builder.h>

namespace NVideoCommon {

namespace {
const TStringBuf POSSIBLE_GENRES_FIELDS[] = {TStringBuf("meta_genres"), TStringBuf("genres")};
const auto IVI_GENRES_UPDATE_PERIOD = std::chrono::minutes{5};

TMaybe<THashMap<ui32, TString>> DownloadGenres(TIviGenres::TDelegate& delegate) {
    auto request = delegate.MakeRequest("categories/v5/" /* path */);
    auto handle = request->Fetch();
    auto response = handle->Wait();

    if (!response || response->IsError()) {
        TStringBuilder msg;
        msg << "Ivi genres update error: ";
        if (response)
            msg << response->GetErrorText();
        else
            msg << "no response";
        LOG(ERR) << msg << Endl;
        return Nothing();
    }

    NSc::TValue genresCollection;

    if (!NSc::TValue::FromJson(genresCollection, response->Data) || !genresCollection.Has("result")) {
        LOG(ERR) << "Ivi genres update error: cannot parse JSON" << Endl;
        return Nothing();
    }

    THashMap<ui32, TString> data;

    for (const auto& item : genresCollection.Get("result").GetArray()) {
        for (const auto& field : POSSIBLE_GENRES_FIELDS) {
            if (item[field].ArraySize() == 0)
                continue;

            for (const auto& genre : item.Get(field).GetArray()) {
                if (genre["id"].IsNull() || genre["title"].IsNull())
                    continue;
                const TString singularGenre = SingularizeGenre(genre["title"].ForceString());
                if (singularGenre)
                    data[genre["id"].ForceIntNumber()] = singularGenre;
            }
        }
    }

    LOG(DEBUG) << "Ivi genres downloaded: total " << data.size() << " genres" << Endl;
    return data;
}

} // namespace

// TIviGenres
TIviGenres::TIviGenres(TIviGenres::TDelegate& delegate)
    : Delegate(delegate) {
}

TMaybe<TString> TIviGenres::GetIviGenreById(ui32 id) {
    TReadGuard guard(Mutex);
    if (const auto* entry = Data.FindPtr(id))
        return *entry;
    return Nothing();
}

bool TIviGenres::Update() {
    auto data = DownloadGenres(Delegate);
    if (!data.Defined()) {
        LOG(ERR) << "Failed to update IVI genres" << Endl;
        return false;
    }

    TWriteGuard guard(Mutex);
    Data.swap(*data);
    return true;
}

void TIviGenres::Clear() {
    TWriteGuard guard(Mutex);
    Data.clear();
}

// TAutoUpdateIviGenres
TMaybe<TString> TAutoUpdateIviGenres::GetIviGenreById(ui32 id) {
    bool needUpdate = false;
    {
        TReadGuard guard(Mutex);
        needUpdate = !LastUpdate || TClock::now() >= *LastUpdate + IVI_GENRES_UPDATE_PERIOD;
    }

    if (needUpdate)
        Update();
    return TIviGenres::GetIviGenreById(id);
}

bool TAutoUpdateIviGenres::Update() {
    const auto success = TIviGenres::Update();

    {
        TWriteGuard guard(Mutex);
        LastUpdate = TClock::now();
    }

    return success;
}

} // NVideoCommon
