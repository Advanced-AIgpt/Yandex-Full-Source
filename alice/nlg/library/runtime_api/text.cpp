#include "text.h"
#include "text_stream.h"

#include <util/digest/sequence.h>
#include <util/stream/str.h>
#include <util/system/yassert.h>

namespace NAlice::NNlg {

bool TText::TView::CheckInvariants() const {
    // check that empty views have no spans, and vice versa
    Y_ASSERT(Bounds.empty() == (SpansBeginIter == SpansEndIter));

    if (Bounds) {
        // check that bounds aren't too crazy
        Y_ASSERT(!std::less<void>{}(Bounds.data(), Data));

        // check that span iterators overlap the bounds correctly
        const auto [beginOffset, endOffset] = GetBoundsOffsets(GetBounds());
        Y_ASSERT(beginOffset < SpansBeginIter->EndOffset);
        Y_ASSERT(endOffset <= std::prev(SpansEndIter)->EndOffset);
    }

    return true;
}

bool TText::CheckInvariants() const {
    Y_ASSERT(Spans != nullptr);
    const auto& spans = GetSpans();

    // check that end offsets are sorted (important both for the data model and std::upper_bound)
    Y_ASSERT(std::is_sorted(spans.begin(), spans.end(), TSpanOffsetLess{}));

    // check that no adjacent offsets are equal (important for the data model)
    auto endOffsetsEqual = [](TSpan lhs, TSpan rhs) {
        return lhs.EndOffset == rhs.EndOffset;
    };
    Y_ASSERT(std::adjacent_find(spans.begin(), spans.end(), endOffsetsEqual) == spans.end());

    // check that a text has no spans iff it's empty,
    // and that a non-empty text is fully covered by spans
    Y_ASSERT(spans.empty() == Str.empty());

    // check that a non-empty text is fully covered by spans
    // and that the first span is not empty
    if (!spans.empty()) {
        Y_ASSERT(spans.front().EndOffset > 0);
        Y_ASSERT(spans.back().EndOffset == Str.size());
    }

    // check that flagged spans never split the text inside a UTF-8 rune
    for (const auto& [str, flags] : *this) {
        Y_UNUSED(flags);
        Y_ASSERT(UTF8RuneLen(str[0]) != 0);
    }

    return true;
}

TString TText::ExtractSpans(TFlags mask) const {
    TString outStr;
    TStringOutput out(outStr);

    for (const auto [span, flags] : *this) {
        if (flags & mask) {
            out << span;
        }
    }

    return outStr;
}

void TText::Append(TStringBuf str, TText::TFlags flags) {
    if (!str || !flags) {
        return;
    }

    Str += str;

    auto& spans = GetSpans();
    if (spans.empty()) {
        spans.push_back({flags, Str.size()});
    } else {
        auto& back = spans.back();
        if (back.Flags == flags) {
            back.EndOffset = Str.size();
        } else {
            spans.push_back({flags, Str.size()});
        }
    }

    Y_ASSERT(CheckInvariants());
}

void TText::Append(const TText& text, TFlags mask) {
    for (const auto [span, flags] : text) {
        Append(span, flags & mask);
    }
}

void TText::Append(const TText::TView& text, TFlags mask) {
    for (const auto [span, flags] : text) {
        Append(span, flags & mask);
    }
}

}  // namespace NAlice::NNlg

size_t THash<NAlice::NNlg::TTextCommon::TSpan>::operator()(const NAlice::NNlg::TTextCommon::TSpan& span) const {
    auto flagsHash = THash<NAlice::NNlg::TTextCommon::TFlags>{}(span.Flags);
    auto offsetHash = THash<size_t>{}(span.EndOffset);
    return CombineHashes(flagsHash, offsetHash);
}

size_t THash<NAlice::NNlg::TText>::operator()(const NAlice::NNlg::TText& text) const {
    return CombineHashes(THash<TString>{}(text.Str), TSimpleRangeHash{}(text.GetSpans()));
};

template <>
void Out<NAlice::NNlg::TText::TView>(
    IOutputStream& out,
    TTypeTraits<NAlice::NNlg::TText::TView>::TFuncParam value) {
    if (auto* textOut = dynamic_cast<NAlice::NNlg::TTextOutput*>(&out)) {
        textOut->Text.Append(value, textOut->Mask);
    } else {
        out << value.GetBounds();
    }
}

template <>
void Out<NAlice::NNlg::TText>(
    IOutputStream& out,
    TTypeTraits<NAlice::NNlg::TText>::TFuncParam value) {
    if (auto* textOut = dynamic_cast<NAlice::NNlg::TTextOutput*>(&out)) {
        textOut->Text.Append(value, textOut->Mask);
    } else {
        out << value.GetBounds();
    }
}
