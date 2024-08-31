#include "search_result_parser.h"

#include <alice/megamind/protos/common/frame.pb.h>

#include <google/protobuf/text_format.h>

namespace NAlice {

namespace {

/*
    Addiitonal function to help Dump all protobufs
*/
template <class TProto>
void DumpInternal(TRTLogger& logger, const TProto* proto, TSearchResultParser::EDumpMode mode, TStringBuf dumpName) {
    if (proto == nullptr) {
        LOG_DEBUG(logger) << dumpName << ": Nothing";
        return;
    }

    TProtoStringType output;
    switch (mode) {
        case TSearchResultParser::EDumpMode::Json:
            output = JsonStringFromProto(*proto);
            break;
        case TSearchResultParser::EDumpMode::Proto:
            NProtoBuf::TextFormat::PrintToString(*proto, &output);
            break;
    }
    LOG_DEBUG(logger) << dumpName << ": " << output;
}

/*
    Fix URL from search (like '//aaa.web'->'https://aaa.web')
    Note empty URL will remains empty
*/
TString FixupUrl(const TString& url, bool useHttps = true) {
    if (url.StartsWith("//")) {
        return TString::Join(useHttps ? "https:" : "http:", url);
    }
    return url;
}

const TMaybe<google::protobuf::Struct> FindSnippetDocs(const google::protobuf::Struct& entry, TStringBuf snippet) {
    for (const auto& it : entry.fields()) {
        if (it.first == "type") {
            if (it.second.string_value() == snippet) {
                return entry;
             }
             return Nothing();
        }
    }
    return Nothing();
}

void CollectSnippetsEntry(const google::protobuf::Struct& entry, const TString& parentName, TVector<TSearchResultParser::TEntryDescr>& descrList) {
    TSearchResultParser::TEntryDescr descr;

    for (const auto& it : entry.fields()) {
        if (it.first == "type") {
            descr.Type = it.second.string_value();
        } else if (it.first == "subtype") {
            descr.Subtype = it.second.string_value();
        } else if (it.first == "template") {
            descr.Template = it.second.string_value();
        }
    }
    if (!descr.Type.Empty() || !descr.Subtype.Empty() || !descr.Template.Empty()) {
        descr.Parent = parentName;
        descrList.push_back(descr);
    }
}

/*
    Get an image object (url + sizes)
*/
const TMaybe<NData::TSingleImage::TImageRef> GetImageUrl(const TProtoStructParser& psp, const TMaybe<google::protobuf::Struct>& object,
                                                         const TString path, const TString sizew, const TString sizeh) {
    const auto img = psp.GetValueString(*object, path);
    if (!img || img.Empty()) {
        return Nothing();
    }
    NData::TSingleImage::TImageRef imgRef;
    imgRef.SetUrl(FixupUrl(*img));
    if (!sizew.Empty()) {
        imgRef.SetWidth(psp.GetValueInt(*object, sizew, 0));
    }
    if (!sizeh.Empty()) {
        imgRef.SetHeight(psp.GetValueInt(*object, sizeh, 0));
    }
    return imgRef;
}

} // anonymous namespace

/*
    TSearchResultParser ctor
    Setup datasources which will be used in these requests
*/
TSearchResultParser::TSearchResultParser(TRTLogger& logger)
    : Logger_(logger)
{
}

/*
    Attach existing data source to TSearchResultParser
    Note only datasources specified in TSearchResultParser ctor will be added to context
*/
bool TSearchResultParser::AttachDataSource(const NScenarios::TDataSource* dataSrc) {
    if (dataSrc == nullptr) {
        return false;
    }
    if (dataSrc->HasWebSearchDocs()) {
        Docs_ = &dataSrc->GetWebSearchDocs();
    } else if (dataSrc->HasWebSearchDocsRight()) {
        DocsRight_ = &dataSrc->GetWebSearchDocsRight();
    } else if (dataSrc->HasWebSearchWizplaces()) {
        Wizplaces_ = &dataSrc->GetWebSearchWizplaces();
    } else if (dataSrc->HasWebSearchSummarization()) {
        Summarization_ = &dataSrc->GetWebSearchSummarization();
    } else if (dataSrc->HasWebSearchWizard()) {
        Wizard_ = &dataSrc->GetWebSearchWizard();
    } else if (dataSrc->HasWebSearchBanner()) {
        Banner_ = &dataSrc->GetWebSearchBanner();
    } else if (dataSrc->HasWebSearchRenderrer()) {
        if (!dataSrc->GetWebSearchRenderrer().GetResponse().empty()) {
            RenderrerResponse_ = JsonFromString(dataSrc->GetWebSearchRenderrer().GetResponse());
        }
    } else {
        LOG_DEBUG(Logger_) << "Can't find required source";
        return false;
    }
    return true;
}

/*
    Dump all existing objects for debugging purpose
    NOTE: Don't call this function in production mode! Use it for local debug only or under exp flags!
*/
void TSearchResultParser::Dump(EDumpMode mode) {
    if (!Logger_.IsSuitable(ELogPriority::TLOG_DEBUG)) {
        return;
    }
    DumpInternal(Logger_, Docs_, mode, "DOCS");
    DumpInternal(Logger_, DocsRight_, mode, "DOCS_RIGHT");
    DumpInternal(Logger_, Wizplaces_, mode, "WIZPLACES");
    DumpInternal(Logger_, Summarization_, mode, "SUMMARIZATION");
    DumpInternal(Logger_, Banner_, mode, "BANNER");
    DumpInternal(Logger_, Wizard_, mode, "WIZARD");
    if (!RenderrerResponse_.IsNull()) {
        LOG_DEBUG(Logger_) << "RENDERER_RESPONSE: " << JsonToString(RenderrerResponse_);
    } else {
        LOG_DEBUG(Logger_) << "RENDERER_RESPONSE: Null";
    }
}

bool TSearchResultParser::IsDataFound() const {
    return Docs_ || DocsRight_ || Wizplaces_ || Summarization_ || Wizard_ || Banner_ || !RenderrerResponse_.IsNull();
}

bool TSearchResultParser::HasFactoid() const {
    if (Wizplaces_ == nullptr) {
        return false;
    }
    return Wizplaces_->GetImportant().size() > 0;
}

TVector<TSearchResultParser::TEntryDescr> TSearchResultParser::CollectSnippets(const TVector<EUseDatasource>& sources) const {
    TVector<TSearchResultParser::TEntryDescr> descr;
    for (const auto& it : sources) {
        switch (it) {
            case EUseDatasource::Docs:
                if (Docs_ != nullptr) {
                    for (const auto& d: Docs_->GetDocs()) {
                        if (d.GetSnippets().HasFull()) {
                            CollectSnippetsEntry(d.GetSnippets().GetFull(), "docs/full", descr);
                        }
                        for (const auto& pre : d.GetSnippets().GetPre()) {
                            CollectSnippetsEntry(pre, "docs/pre", descr);
                        }
                        for (const auto& post : d.GetSnippets().GetPost()) {
                            CollectSnippetsEntry(post, "docs/post", descr);
                        }
                        if (d.GetSnippets().HasMain()) {
                            CollectSnippetsEntry(d.GetSnippets().GetMain(), "docs/main", descr);
                        }
                    }
                }
                break;
            case EUseDatasource::DocsRight:
                if (DocsRight_ != nullptr) {
                    for (const auto& d: DocsRight_->GetDocsRight()) {
                        if (d.GetSnippets().HasFull()) {
                            CollectSnippetsEntry(d.GetSnippets().GetFull(), "docsright/full", descr);
                        }
                        for (const auto& pre : d.GetSnippets().GetPre()) {
                            CollectSnippetsEntry(pre, "docsright/pre", descr);
                        }
                        for (const auto& post : d.GetSnippets().GetPost()) {
                            CollectSnippetsEntry(post, "docsright/post", descr);
                        }
                        if (d.GetSnippets().HasMain()) {
                            CollectSnippetsEntry(d.GetSnippets().GetMain(), "docsright/main", descr);
                        }
                    }
                }
                break;
            case EUseDatasource::Wizplaces:
                if (Wizplaces_ != nullptr) {
                    for (const auto& d: Wizplaces_->GetImportant()) {
                        if (d.GetSnippets().HasFull()) {
                        if (d.GetSnippets().HasFull()) {
                            CollectSnippetsEntry(d.GetSnippets().GetFull(), "important/full", descr);
                        }
                        for (const auto& pre : d.GetSnippets().GetPre()) {
                            CollectSnippetsEntry(pre, "important/pre", descr);
                        }
                        for (const auto& post : d.GetSnippets().GetPost()) {
                            CollectSnippetsEntry(post, "important/post", descr);
                        }
                        if (d.GetSnippets().HasMain()) {
                            CollectSnippetsEntry(d.GetSnippets().GetMain(), "important/main", descr);
                        }
                        }
                    }
                }
                break;
            case EUseDatasource::Sumarization:
            case EUseDatasource::Wizard:
            case EUseDatasource::Banner:
            case EUseDatasource::Renderer:
                break;
        }
    }
    return descr;
}

/*
    Search snippet in possible documents
*/
TMaybe<google::protobuf::Struct> TSearchResultParser::FindSnippetByType(const TVector<EUseDatasource>& sources, TStringBuf snippet) const {
    for (const auto& it : sources) {
        switch (it) {
            case EUseDatasource::Docs:
                if (Docs_ != nullptr) {
                    for (const auto& d: Docs_->GetDocs()) {
                        if (d.GetSnippets().HasFull()) {
                            auto res = FindSnippetDocs(d.GetSnippets().GetFull(), snippet);
                            if (res) {
                                return std::move(res);
                            }
                        }
                    }
                }
                break;
            case EUseDatasource::DocsRight:
                if (DocsRight_ != nullptr) {
                    for (const auto& d: DocsRight_->GetDocsRight()) {
                        if (d.GetSnippets().HasFull()) {
                            auto res = FindSnippetDocs(d.GetSnippets().GetFull(), snippet);
                            if (res) {
                                return std::move(res);
                            }
                        }
                    }
                }
                break;
            case EUseDatasource::Wizplaces:
                if (Wizplaces_ != nullptr) {
                    for (const auto& d: Wizplaces_->GetImportant()) {
                        if (d.GetSnippets().HasFull()) {
                            auto res = FindSnippetDocs(d.GetSnippets().GetFull(), snippet);
                            if (res) {
                                return std::move(res);
                            }
                        }
                    }
                }
                break;
            case EUseDatasource::Sumarization:
            case EUseDatasource::Wizard:
            case EUseDatasource::Banner:
            case EUseDatasource::Renderer:
                break;
        }
    }
    return Nothing();
}

TMaybe<google::protobuf::Struct> TSearchResultParser::FindFactoidByType(TStringBuf snippet) const {
    if (Wizplaces_ == nullptr) {
        return Nothing();
    }
    for (const auto& doc : Wizplaces_->GetImportant()) {
        for (const auto& construct : doc.GetConstruct()) {
            if (ProtoStructParser_.GetValueString(construct, "type", "") == snippet) {
                return construct;
            }
        }
    }
    return Nothing();
}

TMaybe<google::protobuf::Struct> TSearchResultParser::FindConstruct(const TVector<EUseDatasource>& sources, TStringBuf snippet) const {
    for (const auto& it : sources) {
        switch (it) {
            case EUseDatasource::Docs:
                if (Docs_ != nullptr) {
                    for (const auto& d: Docs_->GetDocs()) {
                        for (const auto& construct: d.GetConstruct()) {
                            if (ProtoStructParser_.GetValueString(construct, "type", "") == snippet) {
                                return construct;
                            }
                        }
                    }
                }
                break;
            case EUseDatasource::DocsRight:
                if (DocsRight_ != nullptr) {
                    for (const auto& d: DocsRight_->GetDocsRight()) {
                        for (const auto& construct: d.GetConstruct()) {
                            if (ProtoStructParser_.GetValueString(construct, "type", "") == snippet) {
                                return construct;
                            }
                        }
                    }
                }
                break;
            case EUseDatasource::Wizplaces:
                if (Wizplaces_ != nullptr) {
                    auto res = FindFactoidByType(snippet);
                    if (res) {
                        return res;
                    }
                }
                break;
            case EUseDatasource::Sumarization:
            case EUseDatasource::Wizard:
            case EUseDatasource::Banner:
            case EUseDatasource::Renderer:
                break;
        }
    }
    return Nothing();
}

/*
    Get an image from the current object

    Function will check following paths to load image
        "avatar"
        "mds_avatar_id"
        "original"
        "thmb_href"
        "big_thmb_href"
        "img_href"

    TODO (?) Is it need to customize search order and or disable some cases?
*/
TMaybe<NData::TSingleImage> TSearchResultParser::GetImage(const TMaybe<google::protobuf::Struct>& object) const {
    if (!object) {
        return Nothing();
    }
    // Collect possible values from structure
    const auto img = GetImageUrl(ProtoStructParser_, object, "avatar", "", "");
    auto avatarid = GetImageUrl(ProtoStructParser_, object, "mds_avatar_id", "", ""); // URL will be fixed later
    const auto original = GetImageUrl(ProtoStructParser_, object, "original", "original_size.width", "original_size.height");
    const auto original2 = GetImageUrl(ProtoStructParser_, object, "orig", "width", "height");
    const auto thmb_href = GetImageUrl(ProtoStructParser_, object, "thmb_href", "thmb_w_orig", "thmb_h_orig");
    const auto big_thmb_href = GetImageUrl(ProtoStructParser_, object, "big_thmb_href", "thmb_w_orig", "thmb_h_orig");
    const auto img_href = GetImageUrl(ProtoStructParser_, object, "img_href", "img_w", "img_h");
    auto imgTemplate  = GetImageUrl(ProtoStructParser_, object, "urlTemplate", "width", "height");
    auto imgTemplate2  = GetImageUrl(ProtoStructParser_, object, "photoUrlTemplate", "width", "height");

    if (avatarid && !avatarid->GetUrl().Empty()) {
        // `mds_avatar_id` has a syntax 1783226/473840969. It should be converted into a right URL
        avatarid->SetUrl(TStringBuilder{} << "https://avatars.mds.yandex.net/get-entity_search/" << avatarid->GetUrl() << "/S122x122");
    }

    // Try to use following values to setup avatarid
    NData::TSingleImage image;
    if (img) {
        *image.MutableUrlAvatar() = std::move(*img);
    } else if (avatarid) {
        *image.MutableUrlAvatar() = std::move(*avatarid);
    } else if (thmb_href) {
        *image.MutableUrlAvatar() = std::move(*thmb_href);
    } else if (big_thmb_href) {
        *image.MutableUrlAvatar() = std::move(*big_thmb_href);
    } else if (original) {
        *image.MutableUrlAvatar() = std::move(*original);
    } else if (original2) {
        *image.MutableUrlAvatar() = std::move(*original2);
    } else if (img_href) {
        *image.MutableUrlAvatar() = std::move(*img_href);
    } else if (imgTemplate) {
        // Need to replace /%s at end to /orig
        auto url = imgTemplate->GetUrl();
        if (url.EndsWith("/%s")) {
            url.replace(url.size()-3, 3, "/orig");
            imgTemplate->SetUrl(url);
        }
        *image.MutableUrlAvatar() = std::move(*imgTemplate);
    } else if (imgTemplate2) {
        // Need to replace /%s at end to /orig
        auto url = imgTemplate2->GetUrl();
        if (url.EndsWith("/%s")) {
            url.replace(url.size()-3, 3, "/orig");
            imgTemplate2->SetUrl(url);
        }
        *image.MutableUrlAvatar() = std::move(*imgTemplate2);
    } else {
        return Nothing();
    }
    // Full UrlSource (if exist)
    if (original) {
        *image.MutableUrlSource() = std::move(*original);
    } else if (img_href) {
        *image.MutableUrlSource() = std::move(*img_href);
    }
    // Fill other data
    image.SetText(ProtoStructParser_.GetValueString(*object, "title", ""));
    image.SetHttpSource(ProtoStructParser_.GetValueString(*object, "src_page_url", ""));
    // TODO: also may be "html_href" and "raw_text" (?)
    return image;
}

TMaybe<NData::TPerson> TSearchResultParser::GetPerson(const TMaybe<google::protobuf::Struct>& object) const {
    if (!object) {
        return Nothing();
    }
    const auto type = ProtoStructParser_.GetValueString(*object, "type");
    if (!type || *type != "Hum") {
        return Nothing();
    }
    NData::TPerson person;
    person.SetId(ProtoStructParser_.GetValueString(*object, "id", "")); // ? Check it
    person.SetName(ProtoStructParser_.GetValueString(*object, "name", ""));
    person.SetUrl(ProtoStructParser_.GetValueString(*object, "source.url", ""));
    person.SetDescription(ProtoStructParser_.GetValueString(*object, "description", ""));
    const auto image = GetImage(ProtoStructParser_.GetKey(*object, "image"));
    if (image) {
        *person.MutableImage() = std::move(image->GetUrlAvatar());
    }
    const auto searchRequest = ProtoStructParser_.GetValueString(*object, "search_request");
    if (searchRequest) {
        TTypedSemanticFrame tfs;
        tfs.MutableSearchSemanticFrame()->MutableQuery()->SetStringValue(*searchRequest);
        person.MutableTypedAction()->PackFrom(tfs);
    }
    return person;
}

TMaybe<NData::TAlbum> TSearchResultParser::GetMusicAlbum(const TMaybe<google::protobuf::Struct>& object) const {
    if (!object) {
        return Nothing();
    }
    const auto type = ProtoStructParser_.GetValueString(*object, "type");
    if (!type || *type != "Music") {
        return Nothing();
    }

    NData::TAlbum album;
    album.SetId(ProtoStructParser_.GetValueString(*object, "music_info.album_info.music_id", ""));
    album.SetTitle(ProtoStructParser_.GetValueString(*object, "title", ""));
    // Note album.SetGenre(); is not applicable here
    const auto image = TSearchResultParser::GetImage(ProtoStructParser_.GetKey(*object, "image"));
    if (image) {
        album.SetCoverUri(image->GetUrlAvatar().GetUrl());
    }
    // Note album.AddArtists() is not applicable here
    album.SetReleaseYear(ProtoStructParser_.GetValueInt(*object, "release_year", 0));
    return album;
}

TMaybe<NData::TVideoObject> TSearchResultParser::GetClip(const TMaybe<google::protobuf::Struct>& object) const {
    if (!object) {
        return Nothing();
    }
    const auto type = ProtoStructParser_.GetValueString(*object, "type");
    if (!type || *type != "Music") { // Note clip object has internal type: Music
        return Nothing();
    }
    // Additional check for clip presence
    NData::TVideoObject clip;
    clip.SetId(ProtoStructParser_.GetValueString(*object, "music_info.track_short_info.music_id", ""));
    if (!ProtoStructParser_.GetValueBool(*object, "music_info.track_short_info.has_video", false)) {
        clip.SetVideoType(NData::TVideoObject_EVideoType::TVideoObject_EVideoType_Unknown);
    } else {
        clip.SetVideoType(NData::TVideoObject_EVideoType::TVideoObject_EVideoType_Clip);
    }
    clip.SetTitle(ProtoStructParser_.GetValueString(*object, "name", ""));
    clip.SetSubtitle(ProtoStructParser_.GetValueString(*object, "subtitle", ""));
    // TODO: Image for preview, can be also retrived from music_info.track_short_info.clip_thumbnail
    const auto img = GetImage(ProtoStructParser_.GetKey(*object, "image"));
    if (img) {
        *clip.MutableLogo() = std::move(img->GetUrlAvatar());

    }
    clip.SetReleaseYear(ProtoStructParser_.GetValueInt(*object, "release_year", 0));
    clip.SetDescription(ProtoStructParser_.GetValueString(*object, "description", ""));
    clip.SetDuration(ProtoStructParser_.GetValueInt(*object, "music_info.track_short_info.duration", 0));
    return clip;
}

TMaybe<NData::TVideoObject> TSearchResultParser::GetMovie(const TMaybe<google::protobuf::Struct>& object) const {
    if (!object) {
        return Nothing();
    }
    const auto type = ProtoStructParser_.GetValueString(*object, "type");
    if (!type || *type != "Film") {
        return Nothing();
    }
    NData::TVideoObject film;

    film.SetId(ProtoStructParser_.GetValueString(*object, "source.url", ""));
    film.SetVideoType(NData::TVideoObject_EVideoType::TVideoObject_EVideoType_Movie);
    film.SetTitle(ProtoStructParser_.GetValueString(*object, "name", ""));
    film.SetSubtitle(ProtoStructParser_.GetValueString(*object, "subtitle", ""));
    // TODO: Image for preview, can be also retrived from thumb
    const auto img = GetImage(ProtoStructParser_.GetKey(*object, "image"));
    if (img) {
        *film.MutableLogo() = std::move(img->GetUrlAvatar());
    }
    film.SetReleaseYear(ProtoStructParser_.GetValueInt(*object, "release_year", 0));
    film.SetDescription(ProtoStructParser_.GetValueString(*object, "description", ""));
    // N/A film.SetDuration();
    const auto rating = GetObjectRating(object);
    if (rating) {
        film.SetRating(*rating);
    }
    film.SetAgeLimit(ProtoStructParser_.GetValueInt(*object, "age_limit", 0));
    film.SetHintInfo(ProtoStructParser_.GetValueString(*object, "hint_description", ""));
    return film;
}

TMaybe<NData::TMusicBand> TSearchResultParser::GetMusicBand(const TMaybe<google::protobuf::Struct>& object) const {
    if (!object) {
        return Nothing();
    }
    const auto type = ProtoStructParser_.GetValueString(*object, "type");
    if (!type || *type != "Band") {
        return Nothing();
    }
    NData::TMusicBand band;

    band.SetId(ProtoStructParser_.GetValueString(*object, "id", ""));
    band.SetTitle(ProtoStructParser_.GetValueString(*object, "title", ""));
    band.SetSubtitle(ProtoStructParser_.GetValueString(*object, "subtitle", ""));
    band.SetGroupDescription(ProtoStructParser_.GetValueString(*object, "description", ""));
    // TODO: Image for preview, can be also retrived from thumb
    const auto img = GetImage(ProtoStructParser_.GetKey(*object, "image"));
    if (img) {
        *band.MutableLogo() = std::move(img->GetUrlAvatar());

    }
    band.SetYear(ProtoStructParser_.GetValueInt(*object, "release_year", 0));
    band.SetSearchUrl(ProtoStructParser_.GetValueString(*object, "search_request", ""));
    return band;
}

TMaybe<NData::TTrack> TSearchResultParser::GetMusicTrack(const TMaybe<google::protobuf::Struct>& object) const {
    if (!object) {
        return Nothing();
    }
    const auto type = ProtoStructParser_.GetValueString(*object, "type");
    if (!type || *type != "Music") {
        return Nothing();
    }
    NData::TTrack musicTrack;

    musicTrack.SetId(ProtoStructParser_.GetValueString(*object, "ids.yamusic", ""));
    musicTrack.SetTitle(ProtoStructParser_.GetValueString(*object, "title", ""));
    musicTrack.SetSubtype(ProtoStructParser_.GetValueString(*object, "wsubtype.0", ""));
    const auto img = GetImage(ProtoStructParser_.GetKey(*object, "image"));
    if (img) {
        musicTrack.SetArtImageUrl(img->GetUrlAvatar().GetUrl());
    }
    musicTrack.MutableAlbum()->SetTitle(ProtoStructParser_.GetValueString(*object, "hint_description", ""));
    // musicTrack.AddArtists() - N/A)
    // musicTrack.SetIsLiked() - N/A
    // musicTrack.SetIsDisliked() - N/A
    musicTrack.SetDurationMs(ProtoStructParser_.GetValueInt(*object, "music_info.track_short_info.duration", 0) * 1000);
    return musicTrack;
}

TMaybe<NData::TCompany> TSearchResultParser::GetCompany(const TMaybe<google::protobuf::Struct>& object) const {
    if (!object) {
        return Nothing();
    }
    const auto type = ProtoStructParser_.GetValueString(*object, "type");
    if (!type || *type != "Org") {
        return Nothing();
    }
    NData::TCompany company;
    company.SetId(ProtoStructParser_.GetValueString(*object, "id", ""));
    company.SetName(ProtoStructParser_.GetValueString(*object, "name", ""));
    const auto img = GetImage(ProtoStructParser_.GetKey(*object, "image"));
    if (img) {
        *company.MutableImage() = img->GetUrlAvatar();
    }
    company.SetUrl(ProtoStructParser_.GetValueString(*object, "description_source.url", ""));
    company.SetDescription(ProtoStructParser_.GetValueString(*object, "description", ""));
    const auto rating = GetObjectRating(*object);
    if (rating) {
        company.SetRating(*rating);
    }
    company.SetSearchUrl(ProtoStructParser_.GetValueString(*object, "search_request", ""));
    return company;
}

TMaybe<NData::TGeoPlace> TSearchResultParser::GetGeoPlace(const TMaybe<google::protobuf::Struct>& object) const {
    if (!object) {
        return Nothing();
    }
    const auto type = ProtoStructParser_.GetValueString(*object, "type");
    if (!type || *type != "Geo") {
        return Nothing();
    }

    NData::TGeoPlace place;
    place.SetId(ProtoStructParser_.GetValueString(*object, "id", ""));
    place.SetName(ProtoStructParser_.GetValueString(*object, "name", ""));
    const auto img = GetImage(ProtoStructParser_.GetKey(*object, "image"));
    if (img) {
        *place.MutableImage() = img->GetUrlAvatar();
    }
    place.SetUrl(ProtoStructParser_.GetValueString(*object, "description_source.url", ""));
    place.SetDescription(ProtoStructParser_.GetValueString(*object, "description", ""));
    place.SetSearchUrl(ProtoStructParser_.GetValueString(*object, "search_request", ""));
    const auto rating = GetObjectRating(*object);
    if (rating) {
        place.SetRating(*rating);
    }
    return place;
}

TMaybe<NData::TBook> TSearchResultParser::GetBook(const TMaybe<google::protobuf::Struct>& object) const {
    if (!object) {
        return Nothing();
    }
    const auto type = ProtoStructParser_.GetValueString(*object, "type");
    if (!type) {
        return Nothing();
    }
    if (*type != "Text" && *type != "Soft") {
        return Nothing();
    }

    NData::TBook book;
    book.SetId(ProtoStructParser_.GetValueString(*object, "id", ""));
    book.SetName(ProtoStructParser_.GetValueString(*object, "name", ""));
    // book.SetNameOrig()
    const auto img = GetImage(ProtoStructParser_.GetKey(*object, "image"));
    if (img) {
        *book.MutableImage() = img->GetUrlAvatar();
    }
    book.SetSearchUrl(ProtoStructParser_.GetValueString(*object, "search_request", ""));
    book.SetUrl(ProtoStructParser_.GetValueString(*object, "description_source.url", ""));
    book.SetDescription(ProtoStructParser_.GetValueString(*object, "description", ""));
    // TODO book.SetAuthors()
    book.SetReleaseYear(ProtoStructParser_.GetValueInt(*object, "release_year", 0));
    const auto rating = GetObjectRating(*object);
    if (rating) {
        book.SetRating(*rating);
    }
    return book;
}


TMaybe<TString> TSearchResultParser::GetObjectRating(const TMaybe<google::protobuf::Struct>& object) const {
    if (!object) {
        return Nothing();
    }
    TString result;
    auto fn = [this, &result](const google::protobuf::Struct& obj) -> bool {
        // TODO: Need to setup "kinopoisk" and other possible sources (?)
        if (ProtoStructParser_.GetValueString(obj, "type", "") != "kinopoisk") {
            return false;
        }
        // Try to locate values: "original_rating_value"/"original_best_rating"
        // or "rating_value"/"best_rating"
        const auto originalRatingCur = ProtoStructParser_.GetValueString(obj, "original_rating_value");
        const auto originalRatingMax = ProtoStructParser_.GetValueString(obj, "original_best_rating");
        const auto ratingCur = ProtoStructParser_.GetValueString(obj, "rating_value");
        const auto ratingMax = ProtoStructParser_.GetValueString(obj, "best_rating");
        // Find 'Max' value == 10
        if (ratingCur && ratingMax && *ratingMax == "10") {
            result = *ratingCur;
            return true;
        }
        if (originalRatingCur && originalRatingMax && *originalRatingCur == "10") {
            result = *originalRatingCur;
            return true;
        }
        return false;
    };
    // Try to locate rating value for some possible ways
    // <current object>
    // <current object>/rating
    // <current object>/rating.[]
    // <current object>/ysr_org_rating
    if (fn(*object)) {
        return result;
    }
    if (fn(ProtoStructParser_.GetKey(*object, "rating"))) {
        return result;
    }
    if (ProtoStructParser_.EnumerateKeys(*object, "rating.[]", fn)) {
        return result;
    }
    const auto ratingCur = ProtoStructParser_.GetValueString(*object, "ysr_org_rating.score");
    if (ratingCur) {
        return ratingCur;
    }
    return Nothing();
}

/*
    Get primary avatar image from object answer
    @return false if ain avatar not found
*/
TMaybe<NData::TSingleImage> TSearchResultParser::GetMainAvatar(const TMaybe<google::protobuf::Struct>& snippet,
    const TString& path /*= "data.base_info.image"*/) const
{
    if (!snippet) {
        return Nothing();
    }
    auto img = ProtoStructParser_.GetKey(*snippet, path);
    return GetImage(img);
}

/*
    Collect all images from Object Answer gallery section
    @return number of found images or zero
*/
TMaybe<NData::TImageGallery> TSearchResultParser::GetImagesGallery(const TMaybe<google::protobuf::Struct>& snippet,
    const TString& path /*= "data.view.image_gallery.[]"*/) const
{
    if (!snippet) {
        return Nothing();
    }
    NData::TImageGallery gallery;
    ProtoStructParser_.EnumerateKeys(*snippet, path, [this, &gallery](const google::protobuf::Struct& obj) -> bool {
        auto img = GetImage(obj);
        if (img) {
            *gallery.AddImages() = std::move(*img);
        }
        // Continue enumeration
        return false;
    });
    // Nothing collected
    if (gallery.GetImages().size() == 0) {
        return Nothing();
    }
    return gallery;
}

TMaybe<NData::TPersons> TSearchResultParser::GetRelatedPersons(const TMaybe<google::protobuf::Struct>& snippet,
    const TString& path /*= "data.related_object.[].object.[]"*/) const
{
    if (!snippet) {
        return Nothing();
    }
    NData::TPersons persons;
    ProtoStructParser_.EnumerateKeys(*snippet, path, [this, &persons](const google::protobuf::Struct& obj) -> bool {
        auto pers = GetPerson(obj);
        if (pers) {
            *persons.AddPersones() = std::move(*pers);
        }
        // Continue enumeration
        return false;
    });
    return persons;
}

TMaybe<NData::TVideoGallery> TSearchResultParser::GetRelatedClips(
    const TMaybe<google::protobuf::Struct>& snippet,
    const TString& path /*= "data.clips.[].object.[]"*/) const
{
    if (!snippet) {
        return Nothing();
    }
    NData::TVideoGallery gallery;
    ProtoStructParser_.EnumerateKeys(*snippet, path, [this, &gallery](const google::protobuf::Struct& obj) -> bool {
        auto clip = GetClip(obj);
        if (clip) {
            *gallery.AddVideos() = std::move(*clip);
        }
        // Continue enumeration
        return false;
    });
    if (gallery.GetVideos().empty()) {
        return Nothing();
    }
    return gallery;
}

TMaybe<NData::TVideoGallery> TSearchResultParser::GetRelatedMovies(
    const TMaybe<google::protobuf::Struct>& snippet,
    const TString& path /*= "data.related_object.[].object.[]"*/) const
{
    if (!snippet) {
        return Nothing();
    }
    NData::TVideoGallery gallery;
    ProtoStructParser_.EnumerateKeys(*snippet, path, [this, &gallery](const google::protobuf::Struct& obj) -> bool {
        auto movie = GetMovie(obj);
        if (movie) {
            *gallery.AddVideos() = std::move(*movie);
        }
        // Continue enumeration
        return false;
    });
    if (gallery.GetVideos().empty()) {
        return Nothing();
    }
    return gallery;
}

TMaybe<NData::TMusicBands> TSearchResultParser::GetRelatedBands(const TMaybe<google::protobuf::Struct>& snippet,
    const TString& path /*= "data.related_object.[].object.[]"*/,
    bool allowToCollectHum /*= true*/) const
{
    if (!snippet) {
        return Nothing();
    }
    NData::TMusicBands musicBandList;
    ProtoStructParser_.EnumerateKeys(*snippet, path, [this, &musicBandList, allowToCollectHum](const google::protobuf::Struct& obj) -> bool {
        auto band = GetMusicBand(obj);
        if (band) {
            *musicBandList.AddBands() = std::move(*band);
        } else if (allowToCollectHum) {
            auto pers = GetPerson(obj);
            if (pers) {
                // Convert person to Band and merge to list
                NData::TMusicBand band;
                band.SetId(pers->GetId());
                band.SetTitle(pers->GetName());
                band.SetGroupDescription(pers->GetDescription());
                band.MutableLogo()->CopyFrom(pers->GetImage());
                // TODO: Pers info to TMusicBand::Subtitle
                *musicBandList.AddBands() = std::move(band);
            }
        }
        // Continue enumeration
        return false;
    });
    if (musicBandList.GetBands().empty()) {
        return Nothing();
    }
    return musicBandList;
}

TMaybe<NData::TMusicAlbums> TSearchResultParser::GetMusicAlbums(const TMaybe<google::protobuf::Struct>& snippet,
    const TString& path /*= "data.related_object.[].object.[]"*/) const
{
    if (!snippet) {
        return Nothing();
    }
    NData::TMusicAlbums musicAlbums;
    ProtoStructParser_.EnumerateKeys(*snippet, path, [this, &musicAlbums](const google::protobuf::Struct& obj) -> bool {
        auto album = GetMusicAlbum(obj);
        if (album) {
            *musicAlbums.AddAlbums() = std::move(*album);
        }
        // Continue enumeration
        return false;
    });
    if (musicAlbums.GetAlbums().empty()) {
        return Nothing();
    }
    return musicAlbums;
}

TMaybe<NData::TMusicTracks> TSearchResultParser::GetMusicTracks(const TMaybe<google::protobuf::Struct>& snippet,
    const TString& path /*= "data.related_object.[].object.[]"*/) const
{
    if (!snippet) {
        return Nothing();
    }
    NData::TMusicTracks musicTracks;
    ProtoStructParser_.EnumerateKeys(*snippet, path, [this, &musicTracks](const google::protobuf::Struct& obj) -> bool {
        auto track = GetMusicTrack(obj);
        if (track) {
            *musicTracks.AddTracks() = std::move(*track);
        }
        // Continue enumeration
        return false;
    });
    if (musicTracks.GetTracks().empty()) {
        return Nothing();
    }
    return musicTracks;
}

TMaybe<NData::TCompanies> TSearchResultParser::GetCompanies(const TMaybe<google::protobuf::Struct>& snippet,
    const TString& path /*= "data.related_object.[].object.[]"*/) const
{
    if (!snippet) {
        return Nothing();
    }
    NData::TCompanies companies;
    ProtoStructParser_.EnumerateKeys(*snippet, path, [this, &companies](const google::protobuf::Struct& obj) -> bool {
        auto c = GetCompany(obj);
        if (c) {
            *companies.AddCompanies() = std::move(*c);
        }
        // Continue enumeration
        return false;
    });
    if (companies.GetCompanies().empty()) {
        return Nothing();
    }
    return companies;
}

TMaybe<NData::TGeoPlaces> TSearchResultParser::GetGeoPlaces(const TMaybe<google::protobuf::Struct>& snippet,
    const TString& path /*= "data.related_object.[].object.[]"*/,
    bool allowToCollectOrg /*= true*/) const
{
    if (!snippet) {
        return Nothing();
    }
    NData::TGeoPlaces geoPlaces;
    ProtoStructParser_.EnumerateKeys(*snippet, path, [this, &geoPlaces, allowToCollectOrg](const google::protobuf::Struct& obj) -> bool {
        auto place = GetGeoPlace(obj);
        if (place) {
            *geoPlaces.AddPlaces() = std::move(*place);
        } else if (allowToCollectOrg) {
            auto org = GetCompany(obj);
            if (org) {
                // Convert Company to Geo and merge
                NData::TGeoPlace pl;
                pl.SetId(org->GetId());
                pl.SetName(org->GetName());
                pl.MutableImage()->CopyFrom(org->GetImage());
                pl.SetUrl(org->GetUrl());
                pl.SetDescription(org->GetDescription());
                pl.SetRating(org->GetRating());
                *geoPlaces.AddPlaces() = std::move(pl);
            }
        }
        // Continue enumeration
        return false;
    });
    if (geoPlaces.GetPlaces().empty()) {
        return Nothing();
    }
    return geoPlaces;
}

TMaybe<NData::TBooks> TSearchResultParser::GetBooks(const TMaybe<google::protobuf::Struct>& snippet,
    const TString& path /*= "data.related_object.[].object.[]"*/) const
{
    if (!snippet) {
        return Nothing();
    }
    NData::TBooks books;
    ProtoStructParser_.EnumerateKeys(*snippet, path, [this, &books](const google::protobuf::Struct& obj) -> bool {
        auto book = GetBook(obj);
        if (book) {
            *books.AddBooks() = std::move(*book);
        }
        // Continue enumeration
        return false;
    });
    if (books.GetBooks().empty()) {
        return Nothing();
    }
    return books;
}



} // namespace NAlice
