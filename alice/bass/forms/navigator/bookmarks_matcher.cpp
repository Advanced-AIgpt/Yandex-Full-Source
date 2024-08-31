#include "bookmarks_matcher.h"

namespace NBASS {

namespace {
const TStringBuf PREPOSITIONS[] = {"в", "к", "от", "до", "на", "по", "из", "около", "рядом", "где", "находится"};

const TStringBuf STREETS[] = {"аллея",  "аллею",    "бульвар", "набережная", "набережную", "переулок", "площадь",
                              "проезд", "проспект", "шоссе",   "тупик",      "улица",      "улицу",    "тракт",
                              "ал",     "бул",      "наб",     "пер",        "пл",         "пр",       "просп",
                              "ш",      "туп",      "ул",      "тр"};

const TStringBuf LOCALITIES[] = {"город", "городской", "деревня", "деревню", "округ",   "поселок", "посёлок",
                                 "село",  "слобода",   "слободу", "станица", "станицу", "хутор"};

const TStringBuf BUILDINGS[] = {"дом"};
} // namespace

// TGeoStopWords ---------------------------------------------------------------
TGeoStopWords::TGeoStopWords() {
    for (const auto preposition : PREPOSITIONS)
        Holder.Add(preposition, 0.01);
    for (const auto street : STREETS)
        Holder.Add(street, 0.01);
    for (const auto locality : LOCALITIES)
        Holder.Add(locality, 0.01);
    for (const auto building : BUILDINGS)
        Holder.Add(building, 0.01);
}

// TBookmarksMatcher -----------------------------------------------------------
// static
constexpr float TBookmarksMatcher::DEFAULT_THRESHOLD;
constexpr float TBookmarksMatcher::INVALID_SCORE;

TBookmarksMatcher::TBookmarksMatcher(TStringBuf query)
    : Query(query) {
}

float TBookmarksMatcher::Match(TStringBuf bookmark, float threshold) const {
    NParsedUserPhrase::TParsedSequence bm(bookmark);

    if (!NParsedUserPhrase::MatchSequences(Query, bm, StopWords.Holder, threshold))
        return INVALID_SCORE;
    return Query.Match(bm, StopWords.Holder);
}

} // namespace NBASS
