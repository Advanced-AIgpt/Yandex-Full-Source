#include "dataset.h"
#include "util/generic/algorithm.h"
#include <util/stream/file.h>
#include <util/string/split.h>

TVector<TString> LoadTextLinesFromDataset(const TFsPath& path) {
    TFileInput input(path);
    TString buf;

    // Read Header
    input.ReadLine(buf);
    TVector<TString> columnNames = StringSplitter(buf).Split('\t').ToList<TString>();
    size_t textColumnIdx = FindIndex(columnNames, "text");
    Y_ENSURE(textColumnIdx != NPOS, "Can't find the 'text' column");

    // Read Body and select the second column (text)
    TVector<TString> lines;
    while (input.ReadLine(buf)) {
        TVector<TString> splittedLine = StringSplitter(buf).Split('\t').ToList<TString>();
        Y_ENSURE(splittedLine.size() == columnNames.size(), "Row has an incorrect number of columns");
        lines.push_back(splittedLine[textColumnIdx]);
    }

    return lines;
}

void SaveTextLinesInTable(const TFsPath& path, const TVector<TString>& lines) {
    TFileOutput output(path);
    // Write header
    output << "text\ttarget\n";
    // Write body
    for (const auto& line : lines) {
        output << line;
    }
}
