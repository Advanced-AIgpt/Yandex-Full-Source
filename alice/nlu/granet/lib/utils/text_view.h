#pragma once

#include <alice/nlu/libs/interval/interval.h>
#include <library/cpp/langs/langs.h>
#include <util/folder/path.h>

namespace NGranet {

// ~~~~ TSourceText ~~~~

class TSourceText : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TSourceText>;
    using TConstRef = TIntrusiveConstPtr<TSourceText>;

public:
    TString Text;
    TFsPath Path;
    bool IsCompatibilityMode = false;
    ELanguage UILang = LANG_ENG;

public:
    static TRef Create() {
        return new TSourceText();
    }
    static TRef Create(const TString& text, const TFsPath& path, bool isCompatibilityMode, ELanguage uiLang) {
        return new TSourceText(text, path, isCompatibilityMode, uiLang);
    }
    static TRef Create(const TString& text, const TSourceText::TConstRef& other) {
        return new TSourceText(text, other->Path, other->IsCompatibilityMode, other->UILang);
    }

private:
    TSourceText() = default;

    TSourceText(const TString& text, const TFsPath& path, bool isCompatibilityMode, ELanguage uiLang)
        : Text(text)
        , Path(path)
        , IsCompatibilityMode(isCompatibilityMode)
        , UILang(uiLang)
    {
    }
};

// ~~~~ TTextView ~~~~

class TTextView {
public:
    TTextView() = default;
    TTextView(const TSourceText::TConstRef& sourceText, size_t position = 0, size_t length = NPOS);

    bool IsDefined() const;
    bool IsEmpty() const;

    const TSourceText::TConstRef& GetSourceText() const;
    void SetSourceText(const TSourceText::TConstRef& text);

    size_t GetPosition() const;
    size_t GetLength() const;

    TStringBuf Str() const;

    size_t GetEndPosition() const;

    NNlu::TInterval GetInterval() const;
    void SetInterval(const NNlu::TInterval& interval);

    // TStringBuf-like methods
    TTextView SubStr(size_t pos, size_t length = NPOS) const;
    TTextView SubStr(const NNlu::TInterval& interval) const;
    void Trunc(size_t length);
    void Skip(size_t length);
    void Chop(size_t length);
    void RSeek(size_t length);
    TTextView Head(size_t pos) const;
    TTextView Tail(size_t pos) const;
    TTextView Last(size_t length) const;

    bool ReadLine(TTextView* line);

    void Strip();
    void StripLeft();
    void StripRight();
    TTextView Stripped() const;
    TTextView StrippedLeft() const;
    TTextView StrippedRight() const;

    void Merge(const TTextView& other);

    void GetCoordinates(size_t* lineIndex, size_t* columnIndex, size_t* charCount) const;
    TString PrintErrorPosition() const;

private:
    bool IsValid() const;
    const TString& FullText() const;

private:
    TSourceText::TConstRef SourceText;
    size_t Position = 0;
    size_t Length = 0;
};

// ~~~~ TTextView inline methods ~~~~

// static
inline TTextView::TTextView(const TSourceText::TConstRef& sourceText, size_t position, size_t length)
    : SourceText(sourceText)
    , Position(position)
    , Length(length)
{
    if (SourceText == nullptr) {
        return;
    }
    const size_t textLength = SourceText->Text.length();
    Y_ASSERT(Position <= textLength);
    Length = Min(textLength - Position, Length);
    Y_ASSERT(IsValid());
}

inline bool TTextView::IsDefined() const {
    return SourceText != nullptr;
}

inline bool TTextView::IsEmpty() const {
    return Length == 0;
}

inline const TSourceText::TConstRef& TTextView::GetSourceText() const {
    return SourceText;
}

inline void TTextView::SetSourceText(const TSourceText::TConstRef& text) {
    SourceText = text;
    Y_ENSURE(IsValid());
}

inline size_t TTextView::GetPosition() const {
    return Position;
}

inline size_t TTextView::GetLength() const {
    return Length;
}

inline TStringBuf TTextView::Str() const {
    return TStringBuf(FullText(), Position, Length);
}

inline NNlu::TInterval TTextView::GetInterval() const {
    return {Position, Position + Length};
}

inline void TTextView::SetInterval(const NNlu::TInterval& interval) {
    Position = interval.Begin;
    Length = interval.Length();
    Y_ENSURE(IsValid());
}

inline size_t TTextView::GetEndPosition() const {
    return Position + Length;
}

inline void TTextView::Trunc(size_t length) {
    Length = Min(length, Length);
}

inline void TTextView::Skip(size_t length) {
    length = Min(length, Length);
    Position += length;
    Length -= length;
}

inline void TTextView::Chop(size_t length) {
    length = Min(length, Length);
    Length -= length;
}

inline void TTextView::RSeek(size_t length) {
    Skip(Length - Min(length, Length));
}

inline TTextView TTextView::Head(size_t pos) const {
    TTextView result = *this;
    result.Trunc(pos);
    return result;
}

inline TTextView TTextView::Tail(size_t pos) const {
    TTextView result = *this;
    result.Skip(pos);
    return result;
}

inline TTextView TTextView::Last(size_t length) const {
    TTextView result = *this;
    result.RSeek(length);
    return result;
}

inline bool TTextView::IsValid() const {
    if (SourceText == nullptr) {
        return true;
    }
    return Position + Length <= SourceText->Text.length();
}

inline const TString& TTextView::FullText() const {
    Y_ENSURE(SourceText);
    return SourceText->Text;
}

// ~~~~ Tools ~~~~

// Offset of 'part' in 'text'.
// WARNING! Be careful then working with TStringBuf in such a manner. TStringBuf is just optimization.
// Most subsystems don't guarantee that result TStringBuf-s located strictly inside processed TString.
size_t GetSafeOffset(TStringBuf text, TStringBuf part);

TTextView Merge(const TTextView& view1, const TTextView& view2);

} // namespace NGranet
