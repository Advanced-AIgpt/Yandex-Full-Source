#include "table_formatter.h"
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <library/cpp/iterator/enumerate.h>
#include <util/charset/wide.h>
#include <util/stream/format.h>
#include <util/string/builder.h>
#include <util/string/printf.h>

namespace NGranet {

TTableFormatter::TTableFormatter()
{
    Rows.emplace_back();
}

TTableFormatter& TTableFormatter::DisableHorSeparators() {
    NeedHorSeparators = false;
    return *this;
}

TTableFormatter& TTableFormatter::SetHeaderRowCount(size_t count) {
    HeaderRowCount = count;
    return *this;
}

TTableFormatter& TTableFormatter::SetFooterRowCount(size_t count) {
    FooterRowCount = count;
    return *this;
}

TTableFormatter& TTableFormatter::SetColumnDelimiter(TStringBuf delimiter) {
    ColumnDelimiter = TString{delimiter};
    return *this;
}

TTableFormatter& TTableFormatter::SetIndent(TStringBuf indent) {
    Indent = TString{indent};
    return *this;
}

TTableFormatter& TTableFormatter::AddColumn(TStringBuf caption, size_t width, bool isRightAligned) {
    Y_ENSURE(!Rows.empty());
    Y_ENSURE(ColumnsParams.size() == Rows.back().size());
    ColumnsParams.push_back({width, isRightAligned});
    return AddCell(caption);
}

TTableFormatter& TTableFormatter::BeginRow() {
    Rows.emplace_back();
    return *this;
}

TTableFormatter& TTableFormatter::AddCell(TStringBuf text) {
    return AddCell(TString(text));
}

TTableFormatter& TTableFormatter::AddCell(const TString& text) {
    // Autoexpand column width
    const size_t columnIndex = Rows.back().size();
    while (columnIndex >= ColumnsParams.size()) {
        ColumnsParams.push_back({0, false});
    }
    size_t& width = ColumnsParams[columnIndex].Width;
    width = Max(width, GetNumberOfUTF8Chars(text));

    // Add cell
    Rows.back().push_back(text);
    return *this;
}

TString TTableFormatter::Print() const {
    TStringBuilder out;

    size_t width = 0;
    for (const TColumnParams& params : ColumnsParams) {
        width += width > 0 ? GetNumberOfUTF8Chars(ColumnDelimiter) : 0;
        width += params.Width;
    }
    if (NeedHorSeparators) {
        out << Indent << TString(width, '=') << Endl;
    }
    for (const auto& [r, row] : Enumerate(Rows)) {
        if (NeedHorSeparators && r > 0) {
            if (r == HeaderRowCount || r + FooterRowCount == Rows.size()) {
                out << Indent << TString(width, '-') << Endl;
            }
        }
        out << Indent;
        for (const auto& [c, cell] : Enumerate(row)) {
            const TColumnParams& params = ColumnsParams[c];
            if (c > 0) {
                out << ColumnDelimiter;
            }
            if (params.IsRightAligned) {
                out << RightJustify(cell, params.Width);
            } else {
                out << LeftJustify(cell, params.Width);
            }
        }
        out << Endl;
    }
    if (NeedHorSeparators) {
        out << Indent << TString(width, '=') << Endl;
    }
    return out;
}

} // namespace NGranet
