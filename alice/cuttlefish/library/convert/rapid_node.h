#pragma once
#include <alice/cuttlefish/library/convert/private/traits.h>
#include <contrib/libs/rapidjson/include/rapidjson/reader.h>
#include <contrib/libs/rapidjson/include/rapidjson/writer.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>
#include <util/system/yassert.h>
#include <util/string/cast.h>
#include <type_traits>


namespace NAlice::NCuttlefish::NConvert {

enum ENodeType {
    T_UNKNOWN       = 0,
    T_MAP           = 1,
    T_ARRAY         = 1 << 1,
    T_NUMBER        = 1 << 2,
    T_STRING        = 1 << 3,
    T_BOOLEAN       = 1 << 4,
    T_NULL          = 1 << 5,

    T_END_OF_MAP    = 1 << 6,
    T_END_OF_ARRAY  = 1 << 7,

    T_VALUE         = T_NUMBER|T_STRING|T_BOOLEAN|T_NULL,
};


struct TIterativeHandler : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, TIterativeHandler>
{
    inline bool Null() {
        LastNodeType = T_NULL;
        return true;
    }
    inline bool Bool(bool b) {
        BoolStorage = b;
        LastNodeType = T_BOOLEAN;
        return true;
    }
    inline bool RawNumber(const char* str, rapidjson::SizeType length, bool) {
        ValueStorage = TStringBuf(str, length);
        LastNodeType = T_NUMBER;
        return true;
    }
    inline bool String(const char* str, rapidjson::SizeType length, bool) {
        ValueStorage = TStringBuf(str, length);
        LastNodeType = T_STRING;
        return true;
    }

    inline bool StartObject() {
        Depth += 1;
        LastNodeType = T_MAP;
        return true;
    }
    inline bool Key(const char* str, rapidjson::SizeType length, bool) {
        KeyStorage = TStringBuf(str, length);
        LastNodeType = T_MAP;  // anything but T_END_OF_MAP
        return true;
    }
    inline bool EndObject(rapidjson::SizeType) {
        Depth -= 1;
        LastNodeType = T_END_OF_MAP;
        return true;
    }

    inline bool StartArray() {
        Depth += 1;
        LastNodeType = T_ARRAY;
        return true;
    }
    inline bool EndArray(rapidjson::SizeType) {
        Depth -= 1;
        LastNodeType = T_END_OF_ARRAY;
        return true;
    }

    ENodeType LastNodeType;
    int Depth = 0;

    TStringBuf KeyStorage;  // for map keys
    TStringBuf ValueStorage;  // for values (strings and raw numbers)
    bool BoolStorage;
};

struct TRapidJsonContext
{
    static const unsigned Flags =
        rapidjson::kParseStopWhenDoneFlag|
        rapidjson::kParseNumbersAsStringsFlag|
        rapidjson::kParseEscapedApostropheFlag;

    TRapidJsonContext(TStringBuf json)
        : InputStream(json.data())
    {
        Reader.IterativeParseInit();
    }

    inline ENodeType ParseNext() {
        Y_ENSURE(!Reader.IterativeParseComplete() && Reader.IterativeParseNext<Flags>(InputStream, Handler));
        return Handler.LastNodeType;
    }

    inline void ParseTo(int Depth) {
        while (Handler.Depth != Depth)
            ParseNext();
    }

    inline void StoreKey(TString& dst) {
        dst.assign(Handler.KeyStorage.data(), Handler.KeyStorage.length());
    }

    rapidjson::StringStream InputStream;
    rapidjson::Reader Reader;
    TIterativeHandler Handler;
};


class TRapidJsonNode
{
public:
    TRapidJsonNode() = default;

    inline operator bool() const {
        return Ctx != nullptr;
    }

    inline bool NextMapNode(TString& key, TRapidJsonNode& node) {
        Y_ASSERT(*this && this != &node);
        if (Type != T_MAP)
            return false;

        Ctx->ParseTo(Depth);  // exit from all subnodes

        if (Ctx->ParseNext() == T_END_OF_MAP) {
            Type = T_END_OF_MAP;  // to prevent further parsing
            return false;
        }
        Ctx->StoreKey(key);
        MakeSubNode(node, Ctx->ParseNext());
        return true;
    }

    inline bool NextArrayNode(TRapidJsonNode& node) {
        Y_ASSERT(*this && this != &node);

        if (Type != T_ARRAY)
            return false;

        Ctx->ParseTo(Depth);  // exit from all subnodes

        const ENodeType t = Ctx->ParseNext();
        if (t == T_END_OF_ARRAY) {
            Type = T_END_OF_ARRAY;  // to prevent further parsing
            return false;
        }
        MakeSubNode(node, t);
        return true;
    }

    inline ENodeType GetType() const {
        return Type;
    }
    inline bool IsValue() const {  // string|number|bool|null
        return Type & T_VALUE;
    }
    inline bool IsMap() const noexcept {
        return Type == T_MAP;
    }
    inline bool IsArray()  const noexcept {
        return Type == T_ARRAY;
    }

    inline TStringBuf GetValue() const {
        Y_ASSERT(*this && Type == T_STRING|T_NUMBER);
        return Ctx->Handler.ValueStorage;
    }
    inline bool GetBoolValue() const {
        Y_ASSERT(*this && Type == T_BOOLEAN);
        return Ctx->Handler.BoolStorage;
    }

protected:
    TRapidJsonNode(TRapidJsonContext* ctx, ENodeType type)
        : Ctx(ctx)
        , Type(type)
        , Depth(Ctx->Handler.Depth)
    { }

    inline void MakeSubNode(TRapidJsonNode& dst, ENodeType type) {
        dst.Ctx = Ctx;
        dst.Type = type;
        dst.Depth = Depth + 1;
    };

    TRapidJsonContext* Ctx = nullptr;
    ENodeType Type;
    unsigned Depth;
};


class TRapidJsonRootNode
    : private TRapidJsonContext
    , public TRapidJsonNode
{
public:
    TRapidJsonRootNode(TStringBuf json)
        : TRapidJsonContext(json)
        , TRapidJsonNode((TRapidJsonContext*)this, ParseNext())
    { }
};


// ------------------------------------------------------------------------------------------------
struct TRapidJsonWriteContext
{
    using TWriter = rapidjson::Writer<rapidjson::StringBuffer>;

    TRapidJsonWriteContext()
        : Writer(Buffer)
    { }

    inline TStringBuf GetString() const {
        return TStringBuf(Buffer.GetString(), Buffer.GetSize());
    }

    rapidjson::StringBuffer Buffer;
    TWriter Writer;
};


template <ENodeType NodeType>
class TRapidJsonWriter {
public:
    ~TRapidJsonWriter() {
        if constexpr (NodeType == T_MAP) {
            Writer.EndObject();
        }
        else if constexpr (NodeType == T_ARRAY) {
            Writer.EndArray();
        }
    }

    inline TRapidJsonWriter<T_MAP> Map() {
        return TRapidJsonWriter<T_MAP>(Writer);
    }

    inline TRapidJsonWriter<T_ARRAY> Array() {
        return TRapidJsonWriter<T_ARRAY>(Writer);
    }

    template <typename T>
    inline void Key(T&& val) {
        static_assert(NodeType == T_MAP);
        Writer.Key(val.data(), val.length());
    }

    template <size_t Size>
    inline void Key(const char (&val)[Size]) {
        static_assert(NodeType == T_MAP);
        Writer.Key(val, Size - 1);
    }

    inline void Null() {
        Writer.Null();
    }
    inline void Value(TStringBuf val) {
        Writer.String(val.data(), val.length());
    }
    inline void Value(bool val) {
        Writer.Bool(val);
    }
    template <typename T>
    inline IfSignedInt<T> Value(T val) {
        Writer.Int64(val);
    }
    template <typename T>
    inline IfUnsignedInt<T> Value(T val) {
        Writer.Uint64(val);
    }
    template <typename T>
    inline IfFloat<T> Value(T val) {
        Writer.Double(val);
    }

    // just sugar for manual usage
    template <typename KeyT, typename ValueT>
    inline void Insert(KeyT&& key, ValueT&& value) {
        Key(std::forward<KeyT>(key));
        Value(std::forward<ValueT>(value));
    }

protected:
    template <ENodeType> friend class TRapidJsonWriter;

    inline TRapidJsonWriter(TRapidJsonWriteContext::TWriter& writer)
        : Writer(writer)
    {
        if constexpr (NodeType == T_MAP) {
            Writer.StartObject();
        }
        else if constexpr (NodeType == T_ARRAY) {
            Writer.StartArray();
        }
    }

    TRapidJsonWriteContext::TWriter& Writer;
};


class TRapidJsonRootWriter
    : public TRapidJsonWriteContext
    , public TRapidJsonWriter<T_UNKNOWN>
{
public:
    TRapidJsonRootWriter()
        : TRapidJsonWriter<T_UNKNOWN>(TRapidJsonWriteContext::Writer)
    { }
};


}  // namespace NAlice::NCuttlefish::NConvert

