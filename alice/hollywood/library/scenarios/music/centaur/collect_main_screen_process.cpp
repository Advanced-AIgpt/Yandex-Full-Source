#include "collect_main_screen_process.h"
#include "common.h"

#include <alice/hollywood/library/scenarios/music/proto/centaur.pb.h>

#include <alice/library/json/json.h>

#include <alice/protos/data/scenario/data.pb.h>
#include <alice/protos/data/scenario/music/infinite_feed.pb.h>
#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NHollywood::NMusic::NCentaur {

namespace {

class TImpl {
public:
    TImpl(THwServiceContext& ctx)
        : Ctx_{ctx}
        , InfiniteFeedResponse_{Ctx_.GetMaybeProto<NAppHostHttp::THttpResponse>(INFINITE_FEED_HTTP_RESPONSE_ITEM)}
        , ChildrenLandingCatalogueResponse_{Ctx_.GetMaybeProto<NAppHostHttp::THttpResponse>(CHILDREN_LANDING_CATALOGUE_HTTP_RESPONSE_ITEM)}
    {
    }

    void Do() {
        if (InfiniteFeedResponse_) {
            auto& musicData = *Response_.MutableScenarioData()->MutableMusicInfiniteFeedData();
            const NJson::TJsonValue rootJson = JsonFromString(InfiniteFeedResponse_->GetContent());
            FillInfiniteFeedData(musicData, rootJson);
        } else {
            LOG_ERROR(Ctx_.Logger()) << "No response from music infinite feed found";
        }

        if (ChildrenLandingCatalogueResponse_) {
            // TODO(sparkle): do something
        } else {
            LOG_ERROR(Ctx_.Logger()) << "No response from children landing catalogue found";
        }

        Ctx_.AddProtobufItemToApphostContext(Response_, COLLECT_MAIN_SCREEN_RESPONSE_ITEM);
    }

private:
    void FillInfiniteFeedData(NData::NMusic::TMusicInfiniteFeedData& musicData, const NJson::TJsonValue& rootJson) {
        for (const auto& rowJson : rootJson["result"]["rows"].GetArray()) {
            FillRow(*musicData.AddMusicObjectsBlocks(), rowJson);
        }
    }

    void FillRow(NData::NMusic::TMusicInfiniteFeedData_TMusicObjectsBlock& block, const NJson::TJsonValue& rowJson) {
        block.SetTitle(rowJson["title"].GetString());
        block.SetType(rowJson["typeForFrom"].GetString());
        for (const auto& entityJson : rowJson["entities"].GetArray()) {
            FillEntity(*block.AddMusicObjects(), entityJson);
        }
    }

    void FillEntity(NData::NMusic::TMusicInfiniteFeedData_TMusicObject& object, const NJson::TJsonValue& entityJson) {
        const auto& entityType = entityJson["type"];
        const auto& data = entityJson["data"];

        if (entityType == "auto-playlist") {
            FillAutoPlaylist(*object.MutableAutoPlaylist(), data);
        } else if (entityType == "playlist") {
            FillPlaylist(*object.MutablePlaylist(), data);
        } else if (entityType == "artist") {
            FillArtist(*object.MutableArtist(), data);
        } else if (entityType == "album") {
            FillAlbum(*object.MutableAlbum(), data);
        } else {
            LOG_WARNING(Ctx_.Logger()) << "Unable to parse unknown music entity with type \"" << entityType << "\"";
        }
    }

    void FillAutoPlaylist(NData::NMusic::TMusicInfiniteFeedData_TAutoPlaylist& autoPlaylist, const NJson::TJsonValue& dataJson) {
        autoPlaylist.SetUid(ToString(dataJson["uid"]));
        autoPlaylist.SetTitle(dataJson["title"].GetString());
        autoPlaylist.SetImageUrl(dataJson["cover"]["uri"].GetString());
        autoPlaylist.SetKind(ToString(dataJson["kind"]));
        autoPlaylist.SetModified(dataJson["modified"].GetString());
    }

    void FillPlaylist(NData::NMusic::TMusicInfiniteFeedData_TPlaylist& playlist, const NJson::TJsonValue& dataJson) {
        playlist.SetUid(ToString(dataJson["uid"]));
        playlist.SetTitle(dataJson["title"].GetString());
        playlist.SetImageUrl(dataJson["cover"]["uri"].GetString());
        playlist.SetKind(ToString(dataJson["kind"]));
        playlist.SetModified(dataJson["modified"].GetString());
        playlist.SetLikesCount(dataJson["likesCount"].GetInteger());
    }

    void FillArtist(NData::NMusic::TMusicInfiniteFeedData_TArtist& artist, const NJson::TJsonValue& dataJson) {
        artist.SetId(dataJson["id"].GetStringRobust());
        artist.SetName(dataJson["name"].GetString());
        artist.SetImageUrl(dataJson["uri"].GetString());
        for (const auto& genre : dataJson["genres"].GetArray()) {
            *artist.AddGenres() = genre.GetString();
        }
    }

    void FillAlbum(NData::NMusic::TMusicInfiniteFeedData_TAlbum& album, const NJson::TJsonValue& dataJson) {
        album.SetId(ToString(dataJson["id"]));
        album.SetTitle(dataJson["title"].GetString());
        album.SetImageUrl(dataJson["coverUri"].GetString());
        album.SetReleaseDate(dataJson["releaseDate"].GetString());
        album.SetLikesCount(dataJson["likesCount"].GetInteger());
        for (const auto& artistJson : dataJson["artists"].GetArray()) {
            FillArtist(*album.AddArtists(), artistJson);
        }
    }

private:
    THwServiceContext& Ctx_;
    TCollectMainScreenResponse Response_;
    const TMaybe<NAppHostHttp::THttpResponse> InfiniteFeedResponse_;
    const TMaybe<NAppHostHttp::THttpResponse> ChildrenLandingCatalogueResponse_;
};

} // namespace

const TString& TCollectMainScreenProcessHandle::Name() const {
    const static TString name = "music/centaur_collect_main_screen_process";
    return name;
}

void TCollectMainScreenProcessHandle::Do(THwServiceContext& ctx) const {
    TImpl{ctx}.Do();
}

} // namespace NAlice::NHollywood::NMusic::NCentaur
