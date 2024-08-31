namespace NVideoCommon::NShowOrGallery;

struct TResult {
    name : string (required);
    genre : string (required);
    type : string (required);
    url : string (required);

    rating : double (required);
    release_year : ui32 (required);

    relevance : double (required);
    relevance_prediction : double (required);
};

struct TQuery {
    text : string (required);
    search_text : string (required);
    freq : ui64 (required);
};

struct TItem {
    query : TQuery (required);
    success : bool (required);
    top_relevant_only : bool;
    results : [TResult];
};
