#include "text_view.h"
#include "string_utils.h"
#include <util/charset/utf8.h>
#include <util/generic/algorithm.h>
#include <util/string/builder.h>
#include <util/stream/format.h>

namespace NGranet {

// ~~~~ TTextView ~~~~

TTextView TTextView::SubStr(size_t pos, size_t length) const {
    Y_ENSURE(pos <= Length);
    length = Min(Length - pos, length);
    TTextView result = *this;
    result.Position += pos;
    result.Length = length;
    Y_ENSURE(result.IsValid());
    return result;
}

TTextView TTextView::SubStr(const NNlu::TInterval& interval) const {
    return SubStr(interval.Begin, interval.Length());
}

bool TTextView::ReadLine(TTextView* line) {
    Y_ENSURE(line);
    if (IsEmpty()) {
        return false;
    }
    *line = *this;
    const size_t pos = Str().find_first_of('\n');
    if (pos == TString::npos) {
        Skip(Length);
        return true;
    }
    Skip(pos + 1);
    line->Trunc(pos);
    while (!line->IsEmpty() && line->Str().back() == '\r') {
        line->Chop(1);
    }
    return true;
}

void TTextView::Strip() {
    StripRight();
    StripLeft();
}

void TTextView::StripLeft() {
    const TStringBuf text = FullText();
    while (Length > 0 && IsAsciiSpace(text[Position])) {
        Position++;
        Length--;
    }
}

void TTextView::StripRight() {
    const TStringBuf text = FullText();
    while (Length > 0 && IsAsciiSpace(text[Position + Length - 1])) {
        Length--;
    }
}

TTextView TTextView::Stripped() const {
    TTextView result = *this;
    result.Strip();
    return result;
}

TTextView TTextView::StrippedLeft() const {
    TTextView result = *this;
    result.StripLeft();
    return result;
}

TTextView TTextView::StrippedRight() const {
    TTextView result = *this;
    result.StripRight();
    return result;
}

void TTextView::Merge(const TTextView& other) {
    Y_ENSURE(SourceText.Get() == other.SourceText.Get());
    NNlu::TInterval interval;
    interval.Begin = Min(Position, other.Position);
    interval.End = Max(GetEndPosition(), other.GetEndPosition());
    SetInterval(interval);
}

void TTextView::GetCoordinates(size_t* lineIndex, size_t* columnIndex, size_t* charCount) const {
    Y_ENSURE(IsDefined());
    const TStringBuf text = FullText();
    if (lineIndex != nullptr) {
        *lineIndex = ::Count(text.Head(Position), '\n');
    }
    if (columnIndex != nullptr) {
        *columnIndex = GetNumberOfUTF8Chars(text.Head(Position).RAfter('\n'));
    }
    if (charCount != nullptr) {
        *charCount = GetNumberOfUTF8Chars(text.SubStr(Position, Length));
    }
}

TString TTextView::PrintErrorPosition() const {
    Y_ENSURE(IsDefined());
    return FormatErrorPosition(FullText(), Position, SourceText->Path.GetPath());
}

// ~~~~ Tools ~~~~

size_t GetSafeOffset(TStringBuf text, TStringBuf part) {
    if (part.empty()) {
        // Can be outside text.
        return 0;
    }
    const size_t offset = part.data() - text.data();
    Y_ENSURE(offset <= text.length());
    Y_ENSURE(offset + part.length() <= text.length());
    return offset;
}

TTextView Merge(const TTextView& view1, const TTextView& view2) {
    TTextView result = view1;
    result.Merge(view2);
    return result;
}

} // namespace NGranet
