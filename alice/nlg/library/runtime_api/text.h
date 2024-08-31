#pragma once

////////////////////////////////////////////////////////////////////////////////
// In this module, we define TText, a string in which each character
// is tagged with text/voice flags. See TText for the conceptual description
////////////////////////////////////////////////////////////////////////////////

#include <util/charset/utf8.h>
#include <util/generic/flags.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <algorithm>
#include <iterator>
#include <memory>

namespace NAlice::NNlg {

namespace NPrivate {

inline void EnsureContains(const TStringBuf outer, const TStringBuf inner) {
    // outer.begin() <= inner.begin() && inner.end() <= outer.end()
    // - or -
    // !(inner.begin() < outer.begin()) && !(outer.end() < inner.end())
    //
    // NOTE(a-square): std::less has the total order guarantee in the C++ standard
    // while the built-in operator< doesn't
    Y_ENSURE(!std::less<void>{}(inner.begin(), outer.begin()));
    Y_ENSURE(!std::less<void>{}(outer.end(), inner.end()));
}

}  // namespace NPrivate

// definitions common to all TTextBase implementations
struct TTextCommon {
    enum class EFlag {
        Voice = 1UL << 0,
        Text = 1UL << 1,
    };

    Y_DECLARE_FLAGS(TFlags, EFlag);

    static constexpr TFlags AllFlags() {
        return TFlags{} | EFlag::Voice | EFlag::Text;
    }

    static constexpr TFlags DefaultFlags() {
        return AllFlags();
    }

    struct TSpan {
        TFlags Flags;
        size_t EndOffset;  // cannot be an iterator because TText::Append may invalidate it

        bool operator==(TSpan other) const {
            return Flags == other.Flags && EndOffset == other.EndOffset;
        }
    };

    struct TSpanOffsetLess {
        bool operator()(TSpan lhs, TSpan rhs) const {
            return lhs.EndOffset < rhs.EndOffset;
        }

        bool operator()(size_t offset, TSpan rhs) const {
            return offset < rhs.EndOffset;
        }

        bool operator()(TSpan lhs, size_t offset) const {
            return lhs.EndOffset < offset;
        }
    };

    using TSpans = TVector<TSpan>;
    using TSpansConstIterator = typename TSpans::const_iterator;
};

// CRTP base class for TText, TText::TView, potentially other implementations as well
template <typename TDerived>
class TTextBase : public TTextCommon {
public:
    class ConstIterator {
    public:
        // all iterators must declare these types
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<TStringBuf, TFlags>;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;

        ConstIterator(const char* data,
                      size_t offset,
                      size_t endOffset,
                      TSpansConstIterator spansIter)
            : Data(data)
            , Offset(offset)
            , EndOffset(endOffset)
            , SpansIter(spansIter) {
        }

        bool operator==(const ConstIterator& other) const {
            return Offset == other.Offset;
        }

        bool operator!=(const ConstIterator& other) const {
            return !(*this == other);
        }

        ConstIterator& operator++() {
            Offset = GetClippedEndOffset();
            ++SpansIter;
            return *this;
        }

        ConstIterator operator++(int) {
            auto result = *this;
            ++(*this);
            return result;
        }

        value_type operator*() const {
            return {TStringBuf{Data + Offset, Data + GetClippedEndOffset()}, SpansIter->Flags};
        }

    private:
        // clips the current span into [Offset, EndOffset)
        size_t GetClippedEndOffset() const {
            return Min(SpansIter->EndOffset, EndOffset);
        }

    private:
        const char* Data;  // reference point for Iter->EndOffset
        size_t Offset;  // offset because TSpans deal in offsets
        size_t EndOffset;  // offset because TSpans deal in offsets
        TSpansConstIterator SpansIter;  // current flagged span
    };

    ConstIterator begin() const {
        auto [beginOffset, endOffset] = GetBoundsOffsets(GetBounds());
        return ConstIterator(GetData(), beginOffset, endOffset, BeginSpans());
    }

    ConstIterator end() const {
        auto [beginOffset, endOffset] = GetBoundsOffsets(GetBounds());
        Y_UNUSED(beginOffset);
        return ConstIterator(GetData(), endOffset, endOffset, EndSpans());
    }

    TFlags GetFlagsAt(size_t offset) const {
        auto bounds = GetBounds();
        Y_ENSURE(offset < bounds.size());

        auto globalOffset = bounds.data() - GetData() + offset;

        auto iter = std::upper_bound(BeginSpans(), EndSpans(), globalOffset, TSpanOffsetLess{});
        Y_ASSERT(iter != EndSpans());
        return iter->Flags;
    }

public:
    TDerived& Self() {
        return static_cast<TDerived&>(*this);
    }

    const TDerived& Self() const {
        return static_cast<const TDerived&>(*this);
    }

    const char* GetData() const {
        return Self().GetData();
    }

    TStringBuf GetBounds() const {
        return Self().GetBounds();
    }

    TSpansConstIterator BeginSpans() const {
        return Self().BeginSpans();
    }

    TSpansConstIterator EndSpans() const {
        return Self().EndSpans();
    }

    std::pair<TSpansConstIterator, TSpansConstIterator> GetClippedSpanIters(const TStringBuf bounds) const {
        // We can't use the normal code path for empty bounds
        // because endOffset - 1 would underflow yielding incorrect endIter value.
        if (!bounds) {
            const auto end = EndSpans();
            return {end, end};
        }

        NPrivate::EnsureContains(GetBounds(), bounds);

        const auto [beginOffset, endOffset] = GetBoundsOffsets(bounds);

        const auto textSpansEnd = EndSpans();
        const auto beginIter = std::upper_bound(BeginSpans(), textSpansEnd,
                                                beginOffset, TSpanOffsetLess{});
        const auto endIter = std::next(std::upper_bound(beginIter, textSpansEnd,
                                                        endOffset - 1, TSpanOffsetLess{}));

        Y_ASSERT(beginOffset < beginIter->EndOffset);
        Y_ASSERT(endOffset <= std::prev(endIter)->EndOffset);

        return {beginIter, endIter};
    }

protected:
    // WARNING(a-square): you must call EnsureContains() before calling this
    // so that UB can never happen
    std::pair<size_t, size_t> GetBoundsOffsets(const TStringBuf bounds) const {
        if (!bounds) {
            return {0, 0};
        }

        // we assume that pointers come from the same allocation
        const size_t beginOffset = bounds.data() - GetData();
        const size_t endOffset = beginOffset + bounds.size();
        return {beginOffset, endOffset};
    }
};

// Conceptually, a TText is a string that is divided
// into contiguous spans, and each span has a set of flags
// associated to it, namely Text and Voice flags.
// Thus, every character in the text effectively has a flag.
//
// Text is used for processing text/voice tags in NLG.
// E.g.:
//
// {% phrase hello %}
//   {%tx%}Привет!{%etx%}{%vc%}Приве+т{%evc%} Как прошел ваш день?
// {% endphrase %}
//
// should become:
// {
//     "text": "Привет! Как прошел ваш день?",
//     "voice": "Приве+т Как прошел ваш день?"
// }
//
// To achieve this, we effectively flag each character as text/voice/both,
// and then extract text and voice characters as a part of the phrase postprocessing.
class TText : public TTextBase<TText> {
public:
    class TView : public TTextBase<TView> {
    public:
        // NOTE(a-square): the SFINAE test is imperfect but simple to maintain
        template <typename TDerivedText, typename = std::enable_if_t<std::is_base_of_v<TTextCommon, TDerivedText>>>
        TView(const TDerivedText& text, const TStringBuf bounds) {
            // check that we're clipping at rune boundaries
            if (bounds) {
                NPrivate::EnsureContains(text.GetBounds(), bounds);
                Y_ASSERT(UTF8RuneLen(*bounds.begin()) != 0);
                Y_ASSERT(UTF8RuneLen(*bounds.end()) != 0);  // okay because we never leave a null-terminated string
            }

            Bounds = bounds;
            Data = text.GetData();
            std::tie(SpansBeginIter, SpansEndIter) = text.GetClippedSpanIters(bounds);

            Y_ASSERT(CheckInvariants());
        }

        const char* GetData() const {
            return Data;
        }

        TStringBuf GetBounds() const {
            return Bounds;
        }

        TSpansConstIterator BeginSpans() const {
            return SpansBeginIter;
        }

        TSpansConstIterator EndSpans() const {
            return SpansEndIter;
        }

    private:
        bool CheckInvariants() const;

    private:
        TStringBuf Bounds;
        const char* Data;
        TSpansConstIterator SpansBeginIter;
        TSpansConstIterator SpansEndIter;
    };

public:
    // TODO(a-square): allow for a nullptr to make the presumably common empty text
    // not allocate anything?
    TText()
        : Spans(std::make_unique<TSpans>()) {
        Y_ASSERT(CheckInvariants());
    }

    explicit TText(const TString& str, TFlags flags = DefaultFlags())
        : TText(TString{str}, flags) {
        Y_ASSERT(CheckInvariants());
    }

    explicit TText(TString&& str, TFlags flags = DefaultFlags())
        : Str(std::move(str))
        , Spans(FullTextSpans(flags, Str.size())) {
        Y_ASSERT(CheckInvariants());
    }

    // NOTE(a-square): the SFINAE test is imperfect but simple to maintain
    template <typename TDerivedText, typename = std::enable_if_t<std::is_base_of_v<TTextCommon, TDerivedText>>>
    TText(const TDerivedText& text, const TStringBuf bounds) {
        Spans = std::make_unique<TSpans>();

        if (!bounds) {
            Y_ASSERT(CheckInvariants());
            return;
        }

        NPrivate::EnsureContains(text.GetBounds(), bounds);

        // check that we're clipping at rune boundaries
        Y_ASSERT(UTF8RuneLen(*bounds.begin()) != 0);
        Y_ASSERT(UTF8RuneLen(*bounds.end()) != 0);  // okay because we never leave a null-terminated string

        Str = TString{bounds};

        const auto [beginIter, endIter] = text.GetClippedSpanIters(bounds);

        Y_ASSERT(!std::less<void>{}(bounds.data(), text.GetData()));
        const size_t beginOffset = bounds.data() - text.GetData();

        Spans->reserve(endIter - beginIter);
        for (auto iter = beginIter; iter != endIter; ++iter) {
            Y_ASSERT(beginOffset < iter->EndOffset);
            Spans->push_back({iter->Flags, Min(iter->EndOffset - beginOffset, bounds.size())});
        }

        Y_ASSERT(CheckInvariants());
    }

    TText(const TText& other)
        : Str(other.Str)
        , Spans(std::make_unique<TSpans>(*other.Spans)) {
        Y_ASSERT(CheckInvariants());
    }

    TText& operator=(const TText& other) {
        *this = TText{other};
        Y_ASSERT(CheckInvariants());
        return *this;
    }

    TText(TText&& other) noexcept = default;
    TText& operator=(TText&& other) noexcept = default;

    bool operator==(const TText& other) const {
        return Str == other.Str && GetSpans() == other.GetSpans();
    }

    bool IsEmpty() const {
        return Str.empty();
    }

    const TString& GetStr() const {
        return Str;
    }

    TString&& MoveStr() && {
        return std::move(Str);
    }

    const char* GetData() const {
        return Str.data();
    }

    TStringBuf GetBounds() const {
        return TStringBuf{Str};
    }

    TSpansConstIterator BeginSpans() const {
        return GetSpans().begin();
    }

    TSpansConstIterator EndSpans() const {
        return GetSpans().end();
    }

    TString ExtractSpans(TFlags mask) const;

    TView GetView() const {
        return TView(*this, GetBounds());
    }

    TView GetView(TStringBuf bounds) const {
        return TView(*this, bounds);
    }

    void Append(TStringBuf str, TFlags flags);
    void Append(const TText& text, TFlags mask = AllFlags());
    void Append(const TText::TView& text, TFlags mask = AllFlags());

private:
    // for debugging, see implementation for details
    bool CheckInvariants() const;

private:
    // our target size is <=24 bytes (so that sizeof(TValue) == 32)
    TString Str;  // stores character data, 8 bytes
    // XXX(a-square): TCompactVector has less indirection, but it's poorly implemented
    std::unique_ptr<TSpans> Spans;  // stores flagged spans, 8 bytes

private:
    TSpans& GetSpans() {
        return *Spans;
    }

    const TSpans& GetSpans() const {
        return *Spans;
    }

    static std::unique_ptr<TSpans> FullTextSpans(TFlags flags, size_t size) {
        auto spans = std::make_unique<TSpans>();
        if (size > 0) {
            spans->push_back({flags, size});
        }
        return spans;
    }

    friend ::THash<TText>;
};

}  // namespace NAlice::NNlg

template <>
struct THash<NAlice::NNlg::TTextCommon::TSpan> {
    size_t operator()(const NAlice::NNlg::TTextCommon::TSpan& span) const;
};

template <>
struct THash<NAlice::NNlg::TText> {
    size_t operator()(const NAlice::NNlg::TText& text) const;
};
