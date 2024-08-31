#pragma once

#include <library/cpp/json/writer/json_value.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>

namespace NGranet {

class TTableFormatter {
public:
    TTableFormatter();

    // Options
    TTableFormatter& DisableHorSeparators();
    TTableFormatter& SetHeaderRowCount(size_t count);
    TTableFormatter& SetFooterRowCount(size_t count);
    TTableFormatter& SetColumnDelimiter(TStringBuf delimiter);
    TTableFormatter& SetIndent(TStringBuf indent);

    // Columns
    TTableFormatter& AddColumn(TStringBuf caption, size_t width = 0, bool isRightAligned = false);

    // Rows
    TTableFormatter& BeginRow();
    TTableFormatter& AddCell(const TString& text);
    TTableFormatter& AddCell(TStringBuf text);

    template <class T>
    TTableFormatter& AddCell(const T& value) {
        return AddCell(ToString(value));
    }

    // Format table
    TString Print() const;

private:
    struct TColumnParams {
        size_t Width = 0;
        bool IsRightAligned = false;
    };

private:
    bool NeedHorSeparators = true;
    size_t HeaderRowCount = 1;
    size_t FooterRowCount = 0;
    TString ColumnDelimiter = "  ";
    TString Indent;

    TVector<TColumnParams> ColumnsParams;
    TVector<TVector<TString>> Rows;
};

inline IOutputStream& operator<<(IOutputStream& out, const TTableFormatter& value) {
    out << value.Print();
    return out;
}

}; // namespace NGranet
