#pragma once

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include <util/generic/yexception.h>

namespace NAlice {

class TParseProtoTextError : public yexception {
};

namespace NImpl {

class TErrorCollector : public google::protobuf::io::ErrorCollector {
public:
    void AddError(int /* line */, google::protobuf::io::ColumnNumber /* column */, const TString& message) override {
        ythrow TParseProtoTextError() << message;
    }
};

} // namespace NImpl

template <typename TProto>
inline TProto ParseProto(const TStringBuf raw) {
    TProto proto;

    google::protobuf::io::CodedInputStream in(reinterpret_cast<const unsigned char*>(raw.data()), raw.size());
    Y_ENSURE(proto.MergeFromCodedStream(&in));

    return proto;
}

template <typename TProto>
inline TProto ParseProtoText(const TStringBuf raw, const bool permissive = false) {
    TProto proto;

    NImpl::TErrorCollector errorCollector;

    google::protobuf::TextFormat::Parser parser;
    parser.RecordErrorsTo(&errorCollector);
    // parser.AllowUnknownExtension(permissive); // NOTE(a-square): not supported by our version of protobuf
    parser.AllowUnknownField(permissive);

    google::protobuf::io::ArrayInputStream in(reinterpret_cast<const unsigned char*>(raw.data()), raw.size());
    Y_ENSURE(parser.Merge(&in, &proto)); // NOTE(a-square): we hope that in case of an error we throw earlier than this

    return proto;
}

inline TString SerializeProtoText(const google::protobuf::Message& proto, bool singleLineMode = true, bool expandAny = false) {
    google::protobuf::TextFormat::Printer printer;
    printer.SetUseUtf8StringEscaping(true);  // e.g. for readble Cyrillic
    printer.SetUseShortRepeatedPrimitives(true);  // ints: [1, 2, 3]
    printer.SetSingleLineMode(singleLineMode);
    printer.SetExpandAny(expandAny);

    TString out;
    Y_ENSURE(printer.PrintToString(proto, &out));
    return out;
}

} // namespace NAlice
