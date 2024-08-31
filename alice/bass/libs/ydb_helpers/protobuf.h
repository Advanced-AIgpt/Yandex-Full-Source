#pragma once

#include "exception.h"
#include "visitors.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <ydb/public/sdk/cpp/client/ydb_result/result.h>
#include <ydb/public/sdk/cpp/client/ydb_value/value.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection.h>

#include <util/generic/algorithm.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/ylimits.h>
#include <util/system/yassert.h>

#include <cstddef>
#include <cstdint>

namespace NYdbHelpers {

using TProtobufType = google::protobuf::FieldDescriptor::CppType;
using TYdbType = NYdb::EPrimitiveType;

TStringBuf ProtobufTypeToString(TProtobufType type);
TStringBuf YdbTypeToString(TYdbType type);

template <google::protobuf::FieldDescriptor::CppType Type>
struct TProtobufTypeTraits;

#if defined(DECLARE_PROTOBUF_TYPE_TRAITS)
#error DECLARE_PROTOBUF_TYPE_TRAITS macro is already defined
#endif

#define DECLARE_PROTOBUF_TYPE_TRAITS(PROTOBUF_CPP_TYPE, CPP_TYPE)                                                     \
    template <>                                                                                                       \
    struct TProtobufTypeTraits<PROTOBUF_CPP_TYPE> {                                                                   \
        using TCppType = CPP_TYPE;                                                                                    \
    };

DECLARE_PROTOBUF_TYPE_TRAITS(google::protobuf::FieldDescriptor::CPPTYPE_BOOL, bool)
DECLARE_PROTOBUF_TYPE_TRAITS(google::protobuf::FieldDescriptor::CPPTYPE_INT32, i32)
DECLARE_PROTOBUF_TYPE_TRAITS(google::protobuf::FieldDescriptor::CPPTYPE_UINT32, ui32)
DECLARE_PROTOBUF_TYPE_TRAITS(google::protobuf::FieldDescriptor::CPPTYPE_INT64, i64)
DECLARE_PROTOBUF_TYPE_TRAITS(google::protobuf::FieldDescriptor::CPPTYPE_UINT64, ui64)
DECLARE_PROTOBUF_TYPE_TRAITS(google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE, double)
DECLARE_PROTOBUF_TYPE_TRAITS(google::protobuf::FieldDescriptor::CPPTYPE_FLOAT, float)
DECLARE_PROTOBUF_TYPE_TRAITS(google::protobuf::FieldDescriptor::CPPTYPE_STRING, TString)

#undef DECLARE_PROTOBUF_TYPE_TRAITS

struct TTableSchema {
    struct TColumn {
        TColumn() = default;
        TColumn(const TString& name, TProtobufType protobufType, TYdbType ydbType, int index, bool optional)
            : Name(name)
            , ProtobufType(protobufType)
            , YdbType(ydbType)
            , Index(index)
            , Optional(optional)
        {
        }

        TString Name;
        TProtobufType ProtobufType = TProtobufType::CPPTYPE_STRING;
        TYdbType YdbType = TYdbType::String;
        int Index = 0;
        bool Optional = false;
    };

    explicit TTableSchema(const google::protobuf::Descriptor& descriptor);

    TVector<TColumn> Columns;
};

template <typename TProto, typename TVisitor>
void VisitProtoFields(TProto& row, TVisitor& visitor) {
    const auto& descriptor = *row.GetDescriptor();
    const auto& reflection = *row.GetReflection();

    for (int i = 0; i < descriptor.field_count(); ++i) {
        const auto& field = *descriptor.field(i);

#if defined(PASS_FIELD)
#error PASS_FIELD macro is already defined!
#endif

#define PASS_FIELD(CPP_TYPE, PROTOBUF_TYPE)                                                                           \
    do {                                                                                                              \
        if (!field.is_optional()) {                                                                                   \
            Y_ASSERT(reflection.HasField(row, &field));                                                               \
            const CPP_TYPE value = reflection.Get##PROTOBUF_TYPE(row, &field);                                        \
            visitor(value, field);                                                                                    \
        } else {                                                                                                      \
            TMaybe<CPP_TYPE> value;                                                                                   \
            if (reflection.HasField(row, &field))                                                                     \
                value = reflection.Get##PROTOBUF_TYPE(row, &field);                                                   \
            visitor(value, field);                                                                                    \
        }                                                                                                             \
    } while (false)

        switch (field.cpp_type()) {
            case TProtobufType::CPPTYPE_INT32:
                PASS_FIELD(i32, Int32);
                break;
            case TProtobufType::CPPTYPE_INT64:
                PASS_FIELD(i64, Int64);
                break;
            case TProtobufType::CPPTYPE_UINT32:
                PASS_FIELD(ui32, UInt32);
                break;
            case TProtobufType::CPPTYPE_UINT64:
                PASS_FIELD(ui64, UInt64);
                break;
            case TProtobufType::CPPTYPE_DOUBLE:
                PASS_FIELD(double, Double);
                break;
            case TProtobufType::CPPTYPE_FLOAT:
                PASS_FIELD(float, Float);
                break;
            case TProtobufType::CPPTYPE_BOOL:
                PASS_FIELD(bool, Bool);
                break;
            case TProtobufType::CPPTYPE_STRING:
                PASS_FIELD(TString, String);
                break;

            // Following types are not supported.
            case TProtobufType::CPPTYPE_ENUM:
            case TProtobufType::CPPTYPE_MESSAGE:
                Y_ENSURE(false);
                break;
        }

#undef PASS_FIELD
    }
}

template <typename TProto, typename TFn>
size_t Deserialize(const NYdb::TResultSet& results, TFn&& onResult) {
    const auto kInvalidIndex = Max<size_t>();

    const auto& descriptor = *TProto::descriptor();

    TVector<NYdb::TColumn> rcols = results.GetColumnsMeta();
    Sort(rcols,
         [](const NYdb::TColumn& lhs, const NYdb::TColumn& rhs) { return lhs.Name < rhs.Name; });

    TVector<TTableSchema::TColumn> pcols = TTableSchema(descriptor).Columns;
    Sort(pcols,
         [](const TTableSchema::TColumn& lhs, const TTableSchema::TColumn& rhs) { return lhs.Name < rhs.Name; });

    TVector<size_t> matching(rcols.size(), kInvalidIndex);
    TVector<bool> optional(rcols.size());

    auto onMissingField = [](const NProtoBuf::FieldDescriptor& field) {
        const TString msg = TStringBuilder() << "No proto field " << field.name() << " in YDB";
        if (field.is_optional())
            LOG(WARNING) << msg << Endl;
        else
            ythrow TBadArgumentException() << msg;
    };

    size_t j = 0;
    for (size_t i = 0; i < rcols.size(); ++i) {
        const auto& rcol = rcols[i];

        NYdb::TTypeParser type(rcol.Type);

        if (type.GetKind() == NYdb::TTypeParser::ETypeKind::Optional) {
            optional[i] = true;
            type.OpenOptional();
        }

        if (type.GetKind() != NYdb::TTypeParser::ETypeKind::Primitive) {
            ythrow TBadArgumentException() << "YDB field " << rcol.Name
                                           << " is not a primitive type: " << static_cast<int>(type.GetKind());
        }

        while (j < pcols.size() && pcols[j].Name < rcols[i].Name) {
            onMissingField(*descriptor.field(pcols[j].Index));
            ++j;
        }

        if (j == pcols.size() || pcols[j].Name != rcols[i].Name) {
            LOG(WARNING) << "No YDB field " << rcol.Name << " in proto" << Endl;
            continue;
        }

        matching[i] = j;
        const auto& pcol = pcols[j];
        if (pcol.YdbType != type.GetPrimitive())
            ythrow TBadArgumentException() << "Type mismatch for field " << rcol.Name;
        ++j;
    }

    for (; j != pcols.size(); ++j)
        onMissingField(*descriptor.field(pcols[j].Index));

    NYdb::TResultSetParser reader(results);

    TVector<NYdb::TValueParser*> parsers(Reserve(rcols.size()));
    for (const auto& rcol : rcols)
        parsers.emplace_back(&reader.ColumnParser(rcol.Name));

    size_t rowsRead = 0;
    for (; reader.TryNextRow(); ++rowsRead) {
        TProto row;
        const auto& reflection = *row.GetReflection();

        for (size_t i = 0; i < rcols.size(); ++i) {
            if (matching[i] == kInvalidIndex)
                continue;

            Y_ASSERT(parsers[i]);
            auto& parser = *parsers[i];
            auto& pcol = pcols[matching[i]];

            auto& field = *descriptor.field(pcol.Index);

#if defined(HANDLE_CASE_FILL_FIELD)
#error HANDLE_CASE_FILL_FIELD macro is already defined!
#endif

#define HANDLE_CASE_FILL_FIELD(PROTO_TYPE, YDB_GET_TYPE, PROTO_SET_TYPE, CPP_TYPE)                                    \
    case PROTO_TYPE: {                                                                                                \
        TMaybe<CPP_TYPE> value;                                                                                       \
        if (optional[i])                                                                                              \
            value = parser.GetOptional##YDB_GET_TYPE();                                                               \
        else                                                                                                          \
            value = parser.Get##YDB_GET_TYPE();                                                                       \
        if (value)                                                                                                    \
            reflection.Set##PROTO_SET_TYPE(&row, &field, *value);                                                     \
        break;                                                                                                        \
    }
            switch (const auto type = pcol.ProtobufType) {
                HANDLE_CASE_FILL_FIELD(TProtobufType::CPPTYPE_BOOL, Bool, Bool, bool)
                HANDLE_CASE_FILL_FIELD(TProtobufType::CPPTYPE_INT32, Int32, Int32, i32)
                HANDLE_CASE_FILL_FIELD(TProtobufType::CPPTYPE_INT64, Int64, Int64, i64)
                HANDLE_CASE_FILL_FIELD(TProtobufType::CPPTYPE_UINT32, Uint32, UInt32, ui32)
                HANDLE_CASE_FILL_FIELD(TProtobufType::CPPTYPE_UINT64, Uint64, UInt64, ui64)
                HANDLE_CASE_FILL_FIELD(TProtobufType::CPPTYPE_DOUBLE, Double, Double, double)
                HANDLE_CASE_FILL_FIELD(TProtobufType::CPPTYPE_FLOAT, Float, Float, float)
                HANDLE_CASE_FILL_FIELD(TProtobufType::CPPTYPE_STRING, String, String, TString)
                case TProtobufType::CPPTYPE_ENUM:
                case TProtobufType::CPPTYPE_MESSAGE:
                    ythrow TBadArgumentException() << "Can't parse " << ProtobufTypeToString(type);
                    break;
            }
        }

#undef HANDLE_CASE_FILL_FIELD

        onResult(row);
    }

    return rowsRead;
}

template <typename TProto>
NYdb::TType ProtoToStructType() {
    const TTableSchema schema{*TProto::descriptor()};

    NYdb::TTypeBuilder builder;

    builder.BeginStruct();
    for (const auto& column : schema.Columns) {
        const auto type = NYdb::TTypeBuilder{}.Primitive(column.YdbType).Build();
        if (column.Optional)
            builder.AddMember(column.Name, NYdb::TTypeBuilder{}.Optional(type).Build());
        else
            builder.AddMember(column.Name, type);
    }
    builder.EndStruct();

    return builder.Build();
}

template <typename TProto>
NYdb::TValue ProtoToStructValue(const TProto& proto) {
    NYdb::TValueBuilder builder;

    builder.BeginStruct();
    {
        TProtobufToYDBValueVisitor visitor{builder};
        VisitProtoFields(proto, visitor);
    }
    builder.EndStruct();

    return builder.Build();
}

template <typename TProto>
NYdb::TValue ProtosToList(const TVector<TProto>& protos) {
    if (protos.empty())
        return NYdb::TValueBuilder{}.EmptyList(ProtoToStructType<TProto>()).Build();

    NYdb::TValueBuilder builder;

    builder.BeginList();
    for (const auto& proto : protos)
        builder.AddListItem(ProtoToStructValue(proto));
    builder.EndList();

    return builder.Build();
}

} // namespace NYdbHelpers
