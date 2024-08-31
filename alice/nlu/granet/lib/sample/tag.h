#pragma once

#include <alice/nlu/libs/interval/interval.h>
#include <alice/nlu/libs/tuple_like_type/tuple_like_type.h>
#include <util/generic/vector.h>

namespace NGranet {

// ~~~~ TTag ~~~~

// Simple tag.
struct TTag {
    // Position in text (indexes of tokens or chars)
    NNlu::TInterval Interval;
    // Name (type) of tag.
    TString Name;

    DECLARE_TUPLE_LIKE_TYPE(TTag, Interval, Name);
};

// ~~~~ TTag methods ~~~~

inline TTag MakeTag(size_t begin, size_t end) {
    return {{begin, end}, {}};
}

inline TTag MakeTag(size_t begin, size_t end, TString name) {
    return {{begin, end}, std::move(name)};
}

inline IOutputStream& operator<<(IOutputStream& out, const TTag& tag) {
    return out << "{" << tag.Interval << ", \"" << tag.Name << "\"}";
}

// ~~~~ Tools ~~~~

// Fill gaps between tags by tags with empty name.
TVector<TTag> AddPadding(const TVector<TTag>& tags, size_t length);

// Write nlu tagged line.
// Example:  "Some 'tagged text'(first_tag_name) and one more 'tagged part'(second_tag_name)"
TString PrintTaggerMarkup(TStringBuf text, const TVector<TTag>& tags);

// Remove tagger markup from text.
// Example:  "text 'of'(tag_name) utterance"  ->  "text of utterance"
TString RemoveTaggerMarkup(TStringBuf markup);

// Read tagger markup.
// [in] line:    Some 'tagged text'(tag_name) without 'templates'(+tag_name)
// [out] text:   Some tagged text without templates
// [out] tags:   {1,2,"tag_name"}, {4,5,"+tag_name"}
bool TryReadTaggerMarkup(TStringBuf line, TString* text, TVector<TTag>* tags);

size_t ScanTagName(TStringBuf text);

} // namespace NGranet

template <>
struct THash<NGranet::TTag>: public TTupleLikeTypeHash {
};
