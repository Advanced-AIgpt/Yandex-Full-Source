namespace NBassApi;

// NOTE (a-sidorin@): The definitions in this file should be kept in consistency
// with alice/protos/data/video/video.proto.

struct TVideoItemDebugInfo {
    // debug field with WEB page related to an item
    web_page_url : string;
    // search request id for items from video-search
    search_reqid : string;
};

struct TLightVideoItem {
    type: string (allowed = ["movie", "tv_show", "tv_show_episode", "video", "tv_stream"]);
    provider_name : string;
    provider_item_id : string;

    // Tor tv_show_episodes this id is an id of the corresponding tv-show season.
    tv_show_season_id : string;

    // For tv_show_episode, this id is an id of the whole tv-show.
    tv_show_item_id : string;

    human_readable_id : string;

    misc_ids : struct {
        kinopoisk : string;
        kinopoisk_uuid : string;
        imdb : string;
        onto_id: string;
    };

    available : bool;
    price_from : ui32;
    episode : ui32;
    season : ui32;
    provider_number : ui32;

    // This field must be used for UI only.
    // TODO (@vi002): extract UI-only fields into an individual entity.
    unauthorized : bool;

    audio_language : string;
    subtitles_language : string;
};

struct TSeasonsItem {
    number : ui64;
    season_id : string;
};

struct TVideoRelated {
    related : string;
    relatedVideo (cppname = RelatedVideo) : string;
    related_family : string;
    related_orig_text : string;
    related_src : string;
    related_url : string;
    related_vfp : i32;
    text : string;
};

struct TVideoExtra {
    related : TVideoRelated;
};

// Deprecated form of TSkippableFragment. Has only two possible fragments: intro and credits
struct TSkippableFragmentsDepr {
    intro_start : double;
    intro_end : double;
    credits_start : double;
};

// Fragments that can be skipped during the video, i.e. intro, credits, recap
struct TSkippableFragment {
    start_time : double;
    end_time : double;
    type : string;
};

struct TAudioStreamOrSubtitle {
    title : string;
    language : string;
    index : ui32;
    suggest : string;
};

struct TPlayerRestrictionConfig {
    subtitles_button_enable : bool;
};

struct TAvatarMdsImage {
    base_url : string;
    sizes : [string];
};

struct TVHLicenceInfo {
    avod : string;
    tvod : string;
    user_has_tvod : bool;
    est: string;
    user_has_est: bool;
    svod : string;
    content_type : string;
    horizontal_poster : string;
};

struct TVideoItem : TLightVideoItem {
    cover_url_2x3 : string;
    cover_url_16x9 : string;
    thumbnail_url_2x3_small : string;
    thumbnail_url_16x9 : string;
    thumbnail_url_16x9_small : string;
    thumbnail : TAvatarMdsImage;
    poster : TAvatarMdsImage;

    name : string;
    normalized_name : string;
    search_query : string;
    description : string;
    hint_description: string;
    duration : ui32;
    genre : string;
    rating : double;
    review_available : bool;
    progress : double;
    seasons_count : ui32;
    episodes_count : ui32;

    struct TSeason {
        id : string;
        episodes : [TVideoItem];
        provider_number : ui64;
        soon : bool;
    };

    kinopoisk_info : struct {
        seasons : [TSeason];
    };

    // YYYY, movies & series
    release_year : ui32;
    // timestamp, videos
    mtime : ui64;
    directors : string;
    actors : string;
    source_host : string;
    view_count : ui64;
    play_uri : string;
    embed_uri : string;
    content_uri : string;
    player_id : string;

    relevance : ui64;
    relevance_prediction : double;

    provider_info : [TLightVideoItem];
    availability_request : any;

    next_items : [TVideoItem];
    previous_items : [TVideoItem];

    // Needed for doc2doc
    related : TVideoRelated;

    // id for webview videoEntity screen
    entref : string;

    // Needed only when type is tv_show
    seasons : [TSeasonsItem];

    // Age restriction on the video item.
    min_age : ui32 (default = 18);
    // Age restriction in text form due to the inability to set 0 correctly
    age_limit : string;

    // Needed only when type is tv_stream
    tv_stream_info : struct {
        channel_type : string;
        tv_episode_id: string;
        tv_episode_name : string;
        deep_hd : bool;
        is_personal : bool;
        project_alias : string;
    };
    // TODO remove when possible (see ASSISTANT-2850)
    channel_type : string;
    tv_episode_name : string;

    soon : bool;
    // update_at_us is non-null only if `soon` is `true`.
    update_at_us : ui64;

    debug_info : TVideoItemDebugInfo;

    // Field for analytics_info to determine source of item.
    source : string;

    skippable_fragments_depr : TSkippableFragmentsDepr;
    skippable_fragments : [TSkippableFragment];

    audio_streams : [TAudioStreamOrSubtitle];
    subtitles : [TAudioStreamOrSubtitle];

    player_restriction_config : TPlayerRestrictionConfig;

    legal : string;
    vh_licenses : TVHLicenceInfo;
};

struct TVideoGalleryDebugInfo {
    ya_video_request : string;
    url : string;
};

struct TVideoGallery {
    background_16x9 : string;

    items : [TVideoItem];
    tv_show_item : TVideoItem;
    season : ui32;

    entref : string; // gallery entref ('entity reference' in terms of entity search)
    original_entref : string;   //if we have real entref from es

    debug_info : TVideoGalleryDebugInfo;
};

// Commands

struct TShowPayScreenCommandData {
    item : TVideoItem;

    // Needed only when item is a tv_show_episode.
    tv_show_item : TVideoItem;
};

struct TRequestContentPayload {
    item : TVideoItem;

    // Needed only when item is a tv_show.
    season_index : ui32;
};

struct TPlayVideoCommandData {
    uri : string;

    // Needed to pass any aux info from video provider to video
    // player.
    payload : string;

    session_token : string;
    item : TVideoItem;
    next_item : TVideoItem;
    tv_show_item : TVideoItem;
    start_at : double;

    audio_language : string;
    subtitles_language : string;
};

struct TPlayVideoActionData {
    item : TVideoItem;
    tv_show_item : TVideoItem;
};

struct TShowVideoDescriptionCommandData {
    item : TVideoItem;
    tv_show_item : TVideoItem;
};

// Device state

struct TScreenState {
    current_screen : string;
};

struct TDescriptionScreenState : TScreenState {
    item : TVideoItem;
    next_item : TVideoItem;
    tv_show_item : TVideoItem;
};

struct TVideoProgress {
    played : double;
    duration : double;
};

struct TVideoCurrentlyPlaying : TScreenState {
    paused : bool;
    progress : TVideoProgress;
    item : TVideoItem;
    next_item : TVideoItem;
    tv_show_item : TVideoItem;
    audio_language : string;
    subtitles_language : string;
};

struct TWatchedVideoItem : TLightVideoItem {
    progress : TVideoProgress;
    timestamp : i64;
};

struct TWatchedTvShowItem : TLightVideoItem {
    item : TWatchedVideoItem;
    tv_show_item : TWatchedVideoItem;
};

struct TLastWatchedState {
    tv_shows : [TWatchedTvShowItem];
    movies : [TWatchedVideoItem];
    videos : [TWatchedVideoItem];
};

// Billing API

struct TPricingOption {
    userPrice : ui32 (cppname = UserPrice);
    provider : string;
};

struct TQuasarBillingPricingOptions {
    available : bool;

    pricingOptions : [TPricingOption] (cppname = PricingOptions);
};

// Kinopoisk data

struct TKinopoiskContentItem {
    film_kp_id : string;
    film_uuid : string;
};

// Content DB verification checks.
struct TSeasonDescriptorCheck {
    episodes_count : ui32;
    episode_items : [TVideoItem];
    provider_number : ui32;
    soon : bool;
    (validate) {
        if (!HasProviderNumber())
            AddError("Season check item should have provider number set!");
    };
};

struct TSerialDescriptorCheck {
    seasons_count : ui32;
    seasons : [TSeasonDescriptorCheck];
};

struct TSingleItemCheck {
    item : TVideoItem (required);
    serial_descriptor : TSerialDescriptorCheck;
    (validate item) {
        if (!Item()->HasProviderItemId() || !Item()->HasProviderName() || !Item()->HasType()) {
            AddError("Check item should have provider, provider item id and type set!");
        }
    };
};

struct TDbAmountCheck {
    expected_count : ui32;
    percentage_diff : double;
    (validate) {
        if (!HasExpectedCount() && !HasPercentageDiff())
            AddError("Amount check item should not be empty!");
    };
};

struct TVideoItemDbAmountCheck {
    items : TDbAmountCheck;
    serials : TDbAmountCheck;
    seasons : TDbAmountCheck;
};

struct TContentDbCheckList {
    provider_amounts : {string -> TVideoItemDbAmountCheck };
    total_amounts : TVideoItemDbAmountCheck;
    single_items : [TSingleItemCheck];
};

struct TYaVideoClip {
    title: string;
    thmb_href : string;
    url : string;
    PlayerId (cppname = PlayerId) : string;
    HtmlAutoplayVideoPlayer (cppname = HtmlAutoplayVideoPlayer) : string;
    qproxy : string;
    pass : string;
    duration : i64;
    views : string;

    struct TObjectCard {
        KinopoiskRating (cppname = KinopoiskRating) : double;
        ReleaseYear (cppname = ReleaseYear) : ui32;
        Directors (cppname = Directors) : [string];
        Actors (cppname = Actors) : [string];
    };
    struct TVideoObjects {
        ObjectCards (cppname = ObjectCards) : [TObjectCard];
    };
    extra : TVideoExtra;
    VideoObjects (cppname = VideoObjects) : TVideoObjects;
};

struct TPersonItem {
    kp_id : string;
    name : string;
    description : string;
    subtitle : string;
    entref : string;
    search_query : string;
    image : TAvatarMdsImage;
};

struct TCollectionItem {
    id : string;
    title : string;
    entref : string;
    search_query : string;
    images : [TAvatarMdsImage];
};
