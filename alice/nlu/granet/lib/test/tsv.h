#pragma once

#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <library/cpp/iterator/enumerate.h>
#include <util/folder/path.h>
#include <util/generic/map.h>
#include <util/generic/noncopyable.h>
#include <util/memory/blob.h>
#include <util/stream/file.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/join.h>

namespace NGranet {

// ~~~~ Tsv utils ~~~~

inline TVector<TString> SplitTsvLine(TStringBuf line) {
    return StringSplitter(line).Split('\t').ToList<TString>();
}

inline TString JoinTsvLine(const TVector<TString>& parts) {
    return JoinSeq(TStringBuf("\t"), parts);
}

// ~~~~ TTsvTraits ~~~~

template <class IdType>
struct TTsvTraits {
};

// ~~~~ ENoColumnId ~~~~

enum class ENoColumnId {
};

template <>
struct TTsvTraits<ENoColumnId> {
    static const TMap<ENoColumnId, TString>& GetIdNames() {
        return Default<TMap<ENoColumnId, TString>>();
    };
};

// ~~~~ TTsvHeader ~~~~

template <class IdType = ENoColumnId, class Traits = TTsvTraits<IdType>>
class TTsvHeader : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TTsvHeader>;
    using TConstRef = TIntrusiveConstPtr<TTsvHeader>;

public:
    TTsvHeader() = default;

    static TRef Create(TStringBuf line, const TFsPath& path = {}) {
        return new TTsvHeader(line, path);
    }

    static TRef Create(const TVector<TString>& names, const TFsPath& path = {}) {
        return new TTsvHeader(names, path);
    }

    const TFsPath& GetPath() const {
        return Path;
    }

    const TVector<TString>& GetNames() const {
        return Names;
    }

    const TString& GetJoinedNames() const {
        return JoinedNames;
    }

    size_t Size() const {
        return Names.size();
    }

    bool HasColumn(TStringBuf name) const {
        return NameToIndex.contains(name);
    }

    bool HasColumn(IdType id) const {
        return IdToIndex.contains(id);
    }

    bool HasColumn(size_t index) const {
        return index < Size();
    }

    size_t GetColumnIndex(TStringBuf name) const {
        const auto it = NameToIndex.find(name);
        Y_ENSURE(it != NameToIndex.end(), "Can't find column " + Cite(name) + " in file " + Cite(Path));
        return it->second;
    }

    size_t GetColumnIndex(IdType id) const {
        const auto it = IdToIndex.find(id);
        Y_ENSURE(it != IdToIndex.end(), "Can't find column " + Cite(Traits::GetIdNames().at(id)) + " in file " + Cite(Path));
        return it->second;
    }

    size_t GetColumnIndex(size_t index) const {
        Y_ENSURE(index < Size(), "Can't find column " + ToString(index) + " in file " + Cite(Path));
        return index;
    }

    size_t TryGetColumnIndex(TStringBuf name) const {
        return NameToIndex.Value(name, NPOS);
    }

    size_t TryGetColumnIndex(IdType id) const {
        return IdToIndex.Value(id, NPOS);
    }

    size_t TryGetColumnIndex(size_t index) const {
        return index < Size() ? index : NPOS;
    }

private:
    TTsvHeader(TStringBuf line, const TFsPath& path)
        : JoinedNames(line)
        , Names(SplitTsvLine(line))
        , Path(path)
    {
        Init();
    }

    TTsvHeader(const TVector<TString>& names, const TFsPath& path)
        : JoinedNames(JoinTsvLine(names))
        , Names(names)
        , Path(path)
    {
        Init();
    }

    void Init() {
        for (const auto& [index, name] : Enumerate(Names)) {
            const auto it = NameToIndex.emplace(name, index);
            // TString from Names used as storage for TStringBuf from NameToIndex.
            const TStringBuf& key = it.first->first;
            Y_ASSERT(key.Data() == Names[index].Data());
        }
        for (const auto& [id, name] : Traits::GetIdNames()) {
            const size_t index = NameToIndex.Value(name, NPOS);
            if (index != NPOS) {
                IdToIndex[id] = index;
            }
        }
    }

private:
    TString JoinedNames;
    TVector<TString> Names;
    TMap<TStringBuf, size_t> NameToIndex;
    TMap<IdType, size_t> IdToIndex;
    TFsPath Path;
};

// ~~~~ TTsvLine ~~~~

template <class IdType = ENoColumnId, class Traits = TTsvTraits<IdType>>
class TTsvLine {
public:
    using THeaderConstRef = typename TTsvHeader<IdType, Traits>::TConstRef;

public:
    TTsvLine() = default;

    explicit TTsvLine(const THeaderConstRef& header)
        : Header(header)
    {
        Y_ENSURE(Header);
        CleanValues();
    }

    TTsvLine(const THeaderConstRef& header, TStringBuf line, size_t index = 0)
        : Header(header)
        , Index(index)
    {
        Y_ENSURE(Header);
        SetValues(SplitTsvLine(line));
    }

    TTsvLine(const THeaderConstRef& header, TVector<TString>&& values, size_t index = 0)
        : Header(header)
        , Index(index)
    {
        Y_ENSURE(Header);
        SetValues(std::move(values));
    }

    TTsvLine(const THeaderConstRef& header, const TVector<TString>& values, size_t index = 0)
        : Header(header)
        , Index(index)
    {
        Y_ENSURE(Header);
        SetValues(values);
    }

    TTsvLine(const THeaderConstRef& header, const TTsvLine& other, size_t index = 0)
        : Header(header)
        , Index(index)
    {
        Y_ENSURE(Header);
        if (Header == other.Header) {
            Values = other.Values;
        } else {
            Values.reserve(Header->Size());
            for (const TString& name : Header->GetNames()) {
                Values.push_back(other.Value(name, TString()));
            }
        }
    }

    // Header

    bool IsDefined() const {
        return Header != nullptr;
    }

    operator bool() const {
        return IsDefined();
    }

    const THeaderConstRef& GetHeader() const {
        return Header;
    }

    const TVector<TString>& GetNames() const {
        return Header->GetNames();
    }

    // Direct access to values

    const TVector<TString>& GetValues() const {
        return Values;
    }

    void SetValues(TVector<TString>&& values) {
        Y_ENSURE(Header->Size() == values.size(), "Invalid tsv-file " + FormatErrorPosition());
        Values = std::move(values);
    }

    void CleanValues() {
        Values = TVector<TString>(Header->Size());
    }

    TString GetJoinedValues() const {
        return JoinTsvLine(Values);
    }

    // Value by index, name or id

    template<class ValueType, class KeyType>
    ValueType Value(const KeyType& key, const ValueType& defaultValue) const {
        const size_t index = Header->TryGetColumnIndex(key);
        return index != NPOS ? FromString<ValueType>(Values[index]) : defaultValue;
    }

    template<class KeyType>
    const TString& operator[](const KeyType& key) const {
        return Values[Header->GetColumnIndex(key)];
    }

    template<class KeyType>
    TString& operator[](const KeyType& key) {
        return Values[Header->GetColumnIndex(key)];
    }

    // Index in TSV

    size_t GetIndex() const {
        return Index;
    }

    void SetIndex(size_t index) {
        Index = index;
    }

    // Get line position in file formatted for error message.
    TString FormatErrorPosition() const {
        return Join(':', Header->GetPath(), Index + 1);
    }

private:
    THeaderConstRef Header;
    TVector<TString> Values;
    size_t Index = 0;
};

// ~~~~ TTsv ~~~~

template <class IdType = ENoColumnId, class Traits = TTsvTraits<IdType>>
class TTsv : public TMoveOnly {
public:
    using THeader = TTsvHeader<IdType, Traits>;
    using THeaderConstRef = typename THeader::TConstRef;
    using TLine = TTsvLine<IdType, Traits>;

public:
    TTsv() = default;

    explicit TTsv(const THeaderConstRef& header)
        : Header(header)
    {
    }

    explicit TTsv(const TVector<TString>& columns)
        : Header(THeader::Create(columns))
    {
    }

    // Header

    const THeaderConstRef& GetHeader() const {
        return Header;
    }

    void SetHeader(const THeaderConstRef& header) {
        Header = header;
        for (const TLine& line : std::move(Lines)) {
            AddLine(line);
        }
    }

    void SetHeader(const TVector<TString>& columns) {
        SetHeader(THeader::Create(columns));
    }

    // Lines

    const TVector<TLine>& GetLines() const {
        return Lines;
    }

    void AddLine(TLine line) {
        if (line.GetHeader() == Header) {
            Lines.push_back(std::move(line));
        } else {
            const size_t index = line.GetIndex();
            Lines.push_back(TLine(Header, std::move(line), index));
        }
    }

    void AddLine(TVector<TString> parts) {
        Lines.push_back(TLine(Header, std::move(parts), Lines.size()));
    }

    void AddLine(TStringBuf line) {
        Lines.push_back(TLine(Header, line, Lines.size()));
    }

    // Read and write

    void Read(const TFsPath& path) {
        TFileInput input(path);
        TString str;
        Y_ENSURE(input.ReadLine(str), "No tsv-file header in " + Cite(path));
        Header = THeader::Create(str, path);
        Lines.clear();
        while (input.ReadLine(str)) {
            if (str.empty()) {
                continue;
            }
            Lines.push_back(TLine(Header, str, Lines.size()));
        }
    }

    void Write(const TFsPath& path) const {
        TFileOutput output(path);
        output << Header->GetJoinedNames() << '\n';
        for (const TLine& line : Lines) {
            Y_ENSURE(line.GetHeader() == Header);
            output << line.GetJoinedValues() << '\n';
        }
    }

private:
    THeaderConstRef Header;
    TVector<TLine> Lines;
};

// ~~~~ TTsvReader ~~~~

template <class IdType = ENoColumnId, class Traits = TTsvTraits<IdType>>
class TTsvReader : public TMoveOnly {
public:
    using THeader = TTsvHeader<IdType, Traits>;
    using THeaderConstRef = typename THeader::TConstRef;
    using TLine = TTsvLine<IdType, Traits>;

public:
    TTsvReader(const TFsPath& path)
        : Input(path)
    {
        TString str;
        Y_ENSURE(Input.ReadLine(str), "No tsv-file header in " + Cite(path));
        Header = THeader::Create(str, path);
    }

    // Header info (initialized in constructor).
    const THeaderConstRef& GetHeader() const {
        return Header;
    }

    // Read next line.
    bool ReadLine(TLine* line) {
        *line = ReadLine();
        return line->IsDefined();
    }

    TLine ReadLine() {
        TString str;
        return Input.ReadLine(str) ? TLine(Header, str, LineIndex++) : TLine{};
    }

private:
    TFileInput Input;
    THeaderConstRef Header;
    size_t LineIndex = 0;
};

// ~~~~ TTsvWriter ~~~~

template <class IdType = ENoColumnId, class Traits = TTsvTraits<IdType>>
class TTsvWriter : public TMoveOnly {
public:
    using THeader = TTsvHeader<IdType, Traits>;
    using THeaderConstRef = typename THeader::TConstRef;
    using TLine = TTsvLine<IdType, Traits>;

public:
    TTsvWriter(const TFsPath& path, const TVector<TString>& columns)
        : Output(path)
        , Header(THeader::Create(columns, path))
    {
        Output << Header->GetJoinedNames() << '\n';
    }

    // Header info (initialized in constructor).
    const THeaderConstRef& GetHeader() const {
        return Header;
    }

    // Write next line.
    void WriteLine(const TLine& line) {
        if (line.GetHeader() == Header) {
            Output << line.GetJoinedValues() << '\n';
        } else {
            Output << TLine(Header, line).GetJoinedValues() << '\n';
        }
    }

private:
    TFileOutput Output;
    THeaderConstRef Header;
};

// ~~~~ TMemMapTsv ~~~~

template <class IdType = ENoColumnId, class Traits = TTsvTraits<IdType>>
class TMemMapTsv : public TMoveOnly {
public:
    using THeader = TTsvHeader<IdType, Traits>;
    using THeaderConstRef = typename THeader::TConstRef;
    using TLine = TTsvLine<IdType, Traits>;

public:
    explicit TMemMapTsv(const TFsPath& path)
        : Blob(TBlob::FromFile(path))
    {
        Y_ENSURE(Blob.Data(), "Can't open input file " + Cite(path));
        TStringBuf text(Blob.AsCharPtr(), Blob.Size());
        TStringBuf line;
        text.ReadLine(line);
        Header = THeader::Create(line, path);
        while (text.ReadLine(line)) {
            if (!line.empty()) {
                Lines.push_back(line);
            }
        }
    }

    // Header info (initialized in constructor).
    const THeaderConstRef& GetHeader() const {
        return Header;
    }

    // Lines (without header line).

    size_t Size() const {
        return Lines.size();
    }

    const TVector<TStringBuf>& GetLines() const {
        return Lines;
    }

    TLine CreateTsvLine(size_t index) const {
        return TLine(Header, Lines.at(index), index);
    }

private:
    TBlob Blob;
    THeaderConstRef Header;
    TVector<TStringBuf> Lines;
};

} // namespace NGranet
