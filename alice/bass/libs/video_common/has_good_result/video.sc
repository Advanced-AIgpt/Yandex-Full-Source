namespace NVideoCommon::NHasGoodResult;

struct TResult {
    name : string;
    genre : string;
    type : string;
    url : string;

    rating : double;
    release_year : ui32;

    relevance : double;
    relevance_prediction : double;

    result : string (allowed = ["irrel", "rel_minus", "rel_plus", "useful", "vital"]);
};

struct TItem {
    query : string;
    refined_query : string;
    results : [TResult];
};
