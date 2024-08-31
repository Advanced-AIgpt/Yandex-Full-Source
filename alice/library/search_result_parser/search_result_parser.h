#pragma once

#include <alice/protos/data/scenario/objects/books.pb.h>
#include <alice/protos/data/scenario/objects/companies.pb.h>
#include <alice/protos/data/scenario/objects/image.pb.h>
#include <alice/protos/data/scenario/objects/music.pb.h>
#include <alice/protos/data/scenario/objects/person.pb.h>
#include <alice/protos/data/scenario/objects/places.pb.h>
#include <alice/protos/data/scenario/objects/text.pb.h>
#include <alice/protos/data/scenario/objects/video.pb.h>
#include <alice/protos/data/scenario/video/gallery.pb.h>

#include <alice/library/proto/proto_struct.h>
#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/web_search_source.pb.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {

class TSearchResultParser {
public:
    enum class EUseDatasource {
        Docs,
        DocsRight,
        Wizplaces,
        Sumarization,
        Wizard,
        Banner,
        Renderer
    };

    struct TEntryDescr {
        TString Parent;
        TString Type;
        TString Subtype;
        TString Template;
    };

    //
    // CTOR and initialization functions
    //
    TSearchResultParser(TRTLogger& logger);
    TSearchResultParser(const TSearchResultParser&) = default;

    bool AttachDataSource(const NScenarios::TDataSource* dataSrc);

    void SetProtoStructParserOptions(const TProtoStructParser& psp) {
        ProtoStructParser_ = psp;
    }
    const TProtoStructParser& GetProtoStructParserOptions() const {
        return ProtoStructParser_;
    }

    // Debug dump stored objects
    // Note this function may dramatically increase log size. Don't use it in production code
    enum class EDumpMode {
        Json,
        Proto
    };
    void Dump(EDumpMode mode);
    // Return false, if Parser doesn't have any data to analyze
    bool IsDataFound() const;
    // Return true, if complete factoid (i.e. WIZPLACES object) found in datasource
    bool HasFactoid() const;

    //
    // Global functions to help search data
    //

    // Collect allavailable snippets for logging purpose
    TVector<TEntryDescr> CollectSnippets(const TVector<EUseDatasource>& sources) const;
    // Find snippet by type. Note ALWAYS USE DocsRight + Docs together.
    TMaybe<google::protobuf::Struct> FindSnippetByType(const TVector<EUseDatasource>& sources, TStringBuf snippet) const;
    TMaybe<google::protobuf::Struct> FindDocSnippetByType(TStringBuf snippet) const {
        return FindSnippetByType({EUseDatasource::Docs, EUseDatasource::DocsRight}, snippet);
    }
    TMaybe<google::protobuf::Struct> FindFactoidByType(TStringBuf snippet) const;
    TMaybe<google::protobuf::Struct> FindConstruct(const TVector<EUseDatasource>& sources, TStringBuf snippet) const;

    //
    // Helper function to extract various data from the current objects
    //
    TMaybe<NData::TSingleImage> GetImage(const TMaybe<google::protobuf::Struct>& object) const;
    TMaybe<TString> GetObjectRating(const TMaybe<google::protobuf::Struct>& object) const;

    TMaybe<NData::TPerson> GetPerson(const TMaybe<google::protobuf::Struct>& object) const;
    TMaybe<NData::TAlbum> GetMusicAlbum(const TMaybe<google::protobuf::Struct>& object) const;
    TMaybe<NData::TVideoObject> GetClip(const TMaybe<google::protobuf::Struct>& object) const;
    TMaybe<NData::TVideoObject> GetMovie(const TMaybe<google::protobuf::Struct>& object) const;
    TMaybe<NData::TMusicBand> GetMusicBand(const TMaybe<google::protobuf::Struct>& object) const;
    TMaybe<NData::TTrack> GetMusicTrack(const TMaybe<google::protobuf::Struct>& object) const;
    TMaybe<NData::TCompany> GetCompany(const TMaybe<google::protobuf::Struct>& object) const;
    TMaybe<NData::TGeoPlace> GetGeoPlace(const TMaybe<google::protobuf::Struct>& object) const;
    TMaybe<NData::TBook> GetBook(const TMaybe<google::protobuf::Struct>& object) const;

    //
    // Functons to collect various data from content
    //
    // Images
    TMaybe<NData::TSingleImage> GetMainAvatar(const TMaybe<google::protobuf::Struct>& snippet,
                                              const TString& path = "data.base_info.image") const;
    TMaybe<NData::TImageGallery> GetImagesGallery(const TMaybe<google::protobuf::Struct>& snippet,
                                                  const TString& path = "data.view.image_gallery.[]") const;
    // Persons (all)
    TMaybe<NData::TPersons> GetRelatedPersons(const TMaybe<google::protobuf::Struct>& snippet,
                                              const TString& path = "data.related_object.[].object.[]") const;
    // Clips and Video
    TMaybe<NData::TVideoGallery> GetRelatedClips(const TMaybe<google::protobuf::Struct>& snippet,
                                                 const TString& path = "data.clips.[].object.[]") const;
    TMaybe<NData::TVideoGallery> GetRelatedMovies(const TMaybe<google::protobuf::Struct>& snippet,
                                                  const TString& path = "data.related_object.[].object.[]") const;
    // Music, tracks and bands
    TMaybe<NData::TMusicBands> GetRelatedBands(const TMaybe<google::protobuf::Struct>& snippet,
                                               const TString& path = "data.related_object.[].object.[]",
                                               bool allowToCollectHum = true) const;
    TMaybe<NData::TMusicAlbums> GetMusicAlbums(const TMaybe<google::protobuf::Struct>& snippet,
                                               const TString& path = "data.related_object.[].object.[]") const;
    TMaybe<NData::TMusicTracks> GetMusicTracks(const TMaybe<google::protobuf::Struct>& snippet,
                                               const TString& path = "data.related_object.[].object.[]") const;
    // Companies, organizations and geoplaces
    TMaybe<NData::TCompanies> GetCompanies(const TMaybe<google::protobuf::Struct>& snippet,
                                           const TString& path = "data.related_object.[].object.[]") const;
    TMaybe<NData::TGeoPlaces> GetGeoPlaces(const TMaybe<google::protobuf::Struct>& snippet,
                                           const TString& path = "data.related_object.[].object.[]",
                                           bool allowToCollectOrg = true) const;
    // Books
    TMaybe<NData::TBooks> GetBooks(const TMaybe<google::protobuf::Struct>& snippet,
                                   const TString& path = "data.related_object.[].object.[]") const;


private:
    TRTLogger& Logger_;

    const NScenarios::TWebSearchDocs* Docs_ = nullptr;
    const NScenarios::TWebSearchDocsRight* DocsRight_ = nullptr;
    const NScenarios::TWebSearchWizplaces* Wizplaces_ = nullptr;
    const NScenarios::TWebSearchSummarization* Summarization_ = nullptr;
    const NScenarios::TWebSearchBanner* Banner_ = nullptr;
    const NScenarios::TWebSearchWizard* Wizard_ = nullptr;
    NJson::TJsonValue RenderrerResponse_;

    TProtoStructParser ProtoStructParser_ = TProtoStructParser{};
};


} // namespace NAlice
