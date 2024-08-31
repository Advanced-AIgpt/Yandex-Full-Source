#include "tag.h"
#include <util/generic/algorithm.h>
#include <util/generic/reserve.h>
#include <util/generic/stack.h>
#include <util/string/builder.h>

namespace NGranet {

// ~~~~ Tools ~~~~

TVector<TTag> AddPadding(const TVector<TTag>& original, size_t length) {
    if (length == 0) {
        return original;
    }
    if (original.empty()) {
        return {MakeTag(0, length)};
    }
    TVector<TTag> sorted = original;
    Sort(sorted);
    TVector<TTag> result(Reserve(sorted.size() * 2 + 1));
    size_t maxEnd = 0;
    for (const TTag& tag : sorted) {
        if (tag.Interval.Begin > maxEnd) {
            result.push_back(MakeTag(maxEnd, tag.Interval.Begin));
        }
        result.push_back(tag);
        maxEnd = Max(maxEnd, tag.Interval.End);
    }
    if (length > maxEnd) {
        result.push_back(MakeTag(maxEnd, length));
    }
    return result;
}

namespace {
    // Edge of tag for sorting
    struct TTagEdge {
        size_t Position = 0;
        enum {
            END,
            BEGIN_OF_EMPTY,
            END_OF_EMPTY,
            BEGIN
        } TypeForSort;
        size_t Length = 0;
        TStringBuf Name;
        bool IsBegin = false;

        TTagEdge(const TTag& tag, bool isBegin)
            : Position(isBegin ? tag.Interval.Begin : tag.Interval.End)
            , TypeForSort(tag.Interval.Empty() ? (isBegin ? BEGIN_OF_EMPTY : END_OF_EMPTY) : (isBegin ? BEGIN : END))
            , Length(tag.Interval.Length())
            , Name(tag.Name)
            , IsBegin(isBegin)
        {
        }

        inline auto TieMembers() const {
            return std::tie(Position, TypeForSort, Length, Name, IsBegin);
        }

        inline bool operator<(const TTagEdge& other) const {
            return TieMembers() < other.TieMembers();
        }
    };
}

TString PrintTaggerMarkup(TStringBuf text, const TVector<TTag>& tags) {
    if (tags.empty()) {
        return TString(text);
    }

    TVector<TTagEdge> edges(Reserve(tags.size() * 2));
    for (const TTag& tag : tags) {
        edges.push_back(TTagEdge(tag, true));
        edges.push_back(TTagEdge(tag, false));
    }
    Sort(edges);

    TStringBuilder out;
    size_t pos = 0;
    for (const TTagEdge& edge : edges) {
        out << text.SubStr(pos, edge.Position - pos);
        pos = edge.Position;
        out << '\'';
        if (!edge.IsBegin) {
            out << '(' << edge.Name << ')';
        }
    }
    out << text.SubStr(pos, text.length());
    return out;
}

TString RemoveTaggerMarkup(TStringBuf markup) {
    if (!markup.Contains('\'')) {
        return TString{markup};
    }

    TString text;
    bool isInsideName = false;
    bool isInsideQuotesInName = false;

    for (auto it = markup.begin(); it != markup.end(); ++it) {
        const char c = *it;
        if (isInsideQuotesInName) {
            if (c == '"') {
                isInsideQuotesInName = false;
            } else if (c == '\\') {
                ++it;
            }
        } else if (isInsideName) {
            if (c == ')') {
                isInsideName = false;
            } else if (c == '"') {
                isInsideQuotesInName = true;
            }
        } else if (c == '\'') {
            if (it + 1 != markup.end() && *(it + 1) == '(') {
                isInsideName = true;
                ++it;
            }
        } else {
            text += c;
        }
    }

    return text;
}

size_t ScanTagName(TStringBuf text) {
    auto it = text.begin();
    while (it != text.end() && *it != ')') {
        if (*it == '"') {
            ++it; // skip left quote
            while (it != text.end() && *it != '"') {
                if (*it == '\\') {
                    ++it;
                    if (it == text.end()) {
                        break;
                    }
                }
                ++it;
            }
            if (it == text.end()) {
                break;
            }
            ++it; // skip right quote
            continue;
        }
        ++it;
    }
    return it - text.begin();
}

bool TryReadTaggerMarkup(TStringBuf markup, TString* text, TVector<TTag>* tags) {
    Y_ENSURE(text);
    Y_ENSURE(tags);

    text->clear();
    tags->clear();

    // Fast check
    if (!markup.Contains('\'')) {
        *text = TString{markup};
        return true;
    }

    TStack<TTag> tagStack;

    auto it = markup.begin();
    while (it != markup.end()) {
        if (*it != '\'') {
            *text += *it;
            ++it;
            continue;
        }
        ++it;
        if (it == markup.end()) {
            return false;
        }
        if (*it != '(') {
            tagStack.emplace().Interval.Begin = text->length();
            continue;
        }
        ++it;
        if (tagStack.empty()) {
            return false;
        }

        TTag& tag = tags->emplace_back(tagStack.top());
        tagStack.pop();
        tag.Interval.End = text->length();
        const size_t nameLength = ScanTagName(TStringBuf(it, markup.end()));
        tag.Name = TStringBuf(it, nameLength);
        it += nameLength;
        if (it == markup.end() || *it != ')') {
            return false;
        }
        ++it;
    }

    Sort(*tags);
    return tagStack.empty();
}

} // namespace NGranet
