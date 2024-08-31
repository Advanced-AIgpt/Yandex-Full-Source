#include <alice/rtlog/rthub/protos/rtlog.pb.h>

#include <robot/rthub/yql/generic_protos/ydb.pb.h>

#include <ydb/public/sdk/cpp/client/ydb_table/table.h>
#include <ydb/public/sdk/cpp/client/ydb_value/value.h>

#include <google/protobuf/descriptor.pb.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/string.h>
#include <util/string/printf.h>
#include <util/system/env.h>

#include <variant>

using TParsedYdbKeyValue = std::variant<bool, i32, i64, ui8, ui32, ui64, TString>;

const static TDuration YDB_CLIENT_TIMEOUT = TDuration::Seconds(20);


TString GetColumnName(const NProtoBuf::FieldDescriptor& field) {
    TString columnName = field.options().GetExtension(NRobot::column_name);
    if (columnName.empty()) {
        return field.options().GetExtension(NRobot::key_column_name);
    }
    return columnName;
}

TString DumpStatusToString(const NYdb::TStatus& status) {
    return Sprintf("status [%lu], issues [%s], is transport error [%d]",
                    static_cast<size_t>(status.GetStatus()),
                    status.GetIssues().ToString().c_str(),
                    status.IsTransportError());
}

void CheckStatus(const NYdb::TStatus& status, const TString& formatString, const TString& format) {
    if (status.IsSuccess()) {
        return;
    }
    TString message = Sprintf(formatString.c_str(), format.c_str());
    Y_VERIFY(status.IsSuccess(), "%s, %s",
                message.c_str(),
                DumpStatusToString(status).c_str());
}

TParsedYdbKeyValue ParseYdbValue(NYdb::TValueParser& parser, NYdb::TValueBuilder& valueBuilder, const TString& tableName) {
    switch (parser.GetPrimitiveType()) {
        case NYdb::EPrimitiveType::Bool:
            valueBuilder.OptionalBool(parser.GetBool());
            return parser.GetBool();
        case NYdb::EPrimitiveType::Int32:
            valueBuilder.OptionalInt32(parser.GetInt32());
            return parser.GetInt32();
        case NYdb::EPrimitiveType::Int64:
            valueBuilder.OptionalInt64(parser.GetInt64());
            return parser.GetInt64();
        case NYdb::EPrimitiveType::Uint8:
            valueBuilder.OptionalUint8(parser.GetUint8());
            return parser.GetUint8();
        case NYdb::EPrimitiveType::Uint32:
            valueBuilder.OptionalUint32(parser.GetUint32());
            return parser.GetUint32();
        case NYdb::EPrimitiveType::Uint64:
            valueBuilder.OptionalUint64(parser.GetUint64());
            return parser.GetUint64();
        case NYdb::EPrimitiveType::String:
            valueBuilder.OptionalString(parser.GetString());
            return parser.GetString();
        case NYdb::EPrimitiveType::Utf8:
            valueBuilder.OptionalUtf8(parser.GetUtf8());
            return parser.GetUtf8();
        default:
            Y_FAIL("type blablabal cannot be used as a primary key in table [%s]", tableName.c_str());
    }
}

NYdb::EPrimitiveType ToPrimitiveType(const google::protobuf::FieldDescriptor& field) {
    if (field.options().HasExtension(NRobot::type)) {
        return static_cast<NYdb::EPrimitiveType>(field.options().GetExtension(NRobot::type));
    }
    switch (field.type()) {
        case google::protobuf::FieldDescriptor::TYPE_BOOL:
            return NYdb::EPrimitiveType::Bool;
        case google::protobuf::FieldDescriptor::TYPE_INT32:
            return NYdb::EPrimitiveType::Int32;
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
            return NYdb::EPrimitiveType::Uint32;
        case google::protobuf::FieldDescriptor::TYPE_INT64:
            return NYdb::EPrimitiveType::Int64;
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
            return NYdb::EPrimitiveType::Uint64;
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
            return NYdb::EPrimitiveType::Float;
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
            return NYdb::EPrimitiveType::Double;
        case google::protobuf::FieldDescriptor::TYPE_STRING:
            return NYdb::EPrimitiveType::Utf8;
        case google::protobuf::FieldDescriptor::TYPE_BYTES:
            return NYdb::EPrimitiveType::String;
        default:
            Y_FAIL("type [%s] is not supported, field [%s]",
                    field.type_name(), field.full_name().c_str());
    }
}

NYdb::NTable::TExplicitPartitions CreatePartitionsForNewTable(const NYdb::NTable::TTableDescription& prevTableDescription,
                                                              const TString& tablePath)
{
    NYdb::NTable::TExplicitPartitions partitions;
    TMaybe<TParsedYdbKeyValue> prevBoundary;
    for (const auto& range : prevTableDescription.GetKeyRanges()) {
        if (!range.To().Defined()) {
            continue;
        }
        auto value = range.To()->GetValue();
        auto parser = NYdb::TValueParser(value);
        parser.OpenTuple();
        if (!parser.TryNextElement()) {
            continue;
        }
        parser.OpenOptional();
        auto valueBuilder = NYdb::TValueBuilder();
        valueBuilder.BeginTuple().AddElement();
        // Here we make sure that boundaries for the first key are sorted and unique
        // 1, 2, 3, 4 -> 1, 2, 3, 4
        // 1, 2, 2, 3 -> 1, 2, 3
        // 1, 3, 2, 4 -> fail
        const auto parsedYdbKeyValue = ParseYdbValue(parser, valueBuilder, tablePath);
        if (prevBoundary.Defined()) {
            Y_VERIFY(prevBoundary->index() == parsedYdbKeyValue.index(),
                        "Primary keys have different type in different partitions of table %s, most likely YDB error",
                        tablePath.c_str());
            Y_VERIFY(*prevBoundary <= parsedYdbKeyValue,
                        "Partition ranges for table %s are not sorted", tablePath.c_str());
            if (prevBoundary < parsedYdbKeyValue) {
                prevBoundary = parsedYdbKeyValue;
                partitions.AppendSplitPoints(valueBuilder.EndTuple().Build());
            }
        } else {
            prevBoundary = parsedYdbKeyValue;
            partitions.AppendSplitPoints(valueBuilder.EndTuple().Build());
        }
    }
    return partitions;
}


NYdb::TStatus GetTableDescription(NYdb::NTable::TTableClient& tableClient,
                                  TMaybe<NYdb::NTable::TTableDescription>& tableDescription,
                                  const TString& tablePath)
{
    auto status = tableClient.RetryOperationSync([&tableDescription, &tablePath](NYdb::NTable::TSession session) {
        auto settings = NYdb::NTable::TDescribeTableSettings().WithKeyShardBoundary(true);
        auto result = session.DescribeTable(tablePath, settings).GetValueSync();

        CheckStatus(result, "Failed to get info about table [%s]", tablePath.c_str());
        tableDescription = result.GetTableDescription();
        return result;
    }, NYdb::NTable::TRetryOperationSettings().GetSessionClientTimeout(YDB_CLIENT_TIMEOUT));
    return status;
}


NYdb::TStatus CreateNewTable(NYdb::NTable::TTableClient& tableClient,
                             const TMaybe<NYdb::NTable::TExplicitPartitions>& partitions,
                             const google::protobuf::Descriptor* messageDescriptor,
                             const TString& newTablePath)
{
    NYdb::NTable::TPartitioningPolicy partitioningPolycy = NYdb::NTable::TPartitioningPolicy{}
        .AutoPartitioning(NYdb::NTable::EAutoPartitioningPolicy::AutoSplit);

    if (partitions.Defined()) {
        partitioningPolycy.ExplicitPartitions(*partitions);
    }
    NYdb::NTable::TCreateTableSettings settings;
    settings.PresetName("log_lz4");
    settings.PartitioningPolicy(partitioningPolycy);

    const auto createTableStatus = tableClient.RetryOperationSync([&](NYdb::NTable::TSession session) {
        auto tableBuilder = session.GetTableBuilder();
        TVector<TString> primaryKeys;
        for (int i = 0; i < messageDescriptor->field_count(); ++i) {
            const auto& field = *(messageDescriptor->field(i));
            if (const auto& columnName = GetColumnName(field); !columnName.Empty()) {
                tableBuilder.AddNullableColumn(columnName, ToPrimitiveType(field));
                if (field.options().HasExtension(NRobot::key_column_name)) {
                    primaryKeys.push_back(columnName);
                }
            }
        }
        tableBuilder.SetPrimaryKeyColumns(primaryKeys);

        return session.CreateTable(newTablePath, tableBuilder.Build(), settings).ExtractValueSync();
    });

    return createTableStatus;
}

const google::protobuf::Descriptor* GetDescriptorForTable(const TString& tableName) {
    if (tableName.StartsWith("events_data_")) {
        return NRTLog::TEventItem::descriptor();
    } else if (tableName.StartsWith("events_index_data_")) {
        return NRTLog::TEventIndexItem::descriptor();
    } else if (tableName.StartsWith("special_events_data_")) {
        return NRTLog::TSpecialEventItem::descriptor();
    }
    return nullptr;
}


int main(int argc, const char** argv) {
    NLastGetopt::TOpts opts;

    TString database;
    opts.AddLongOption("database").Help("Database name").Required().StoreResult(&database);

    TString endpoint;
    opts.AddLongOption("endpoint").Help("Database endpoint").Required().StoreResult(&endpoint);

    TString tableName;
    opts.AddLongOption("tablename").Help("Old table name").Required().StoreResult(&tableName);

    TString newTableName;
    opts.AddLongOption("newtablename").Help("New table name").Required().StoreResult(&newTableName);

    bool copyPartitions = false;
    opts.AddLongOption("copy-patrtitions").Help("Preserve partitions from previous table").NoArgument().StoreTrue(&copyPartitions);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    TString token = GetEnv("YDB_TOKEN");

    const TString tablePath = database + "/" + tableName;
    const TString newTablePath = database + "/" + newTableName;

    // YDB accessors
    const auto driverConfig = NYdb::TDriverConfig().SetEndpoint(endpoint);
    NYdb::TDriver driver(driverConfig);
    NYdb::NTable::TClientSettings clientSettings;
    clientSettings.Database(database)
                  .AuthToken(token);

    NYdb::NTable::TTableClient tableClient{driver, clientSettings};

    // Get previous table description
    TMaybe<NYdb::NTable::TTableDescription> tableDescription;
    NYdb::TStatus getDescriptionStatus = GetTableDescription(tableClient, tableDescription, tablePath);
    CheckStatus(getDescriptionStatus, "Get table [%s] description failed", newTableName.c_str());

    // Get schema for new table
    const google::protobuf::Descriptor* descriptorPtr = GetDescriptorForTable(tableName);
    if (descriptorPtr == nullptr) {
        Cerr << "ERROR: Unknown table type for table " << tableName << Endl;
        return 1;
    }

    TMaybe<NYdb::NTable::TExplicitPartitions> partitions;

    if (copyPartitions) {
        // Create partitions for new table
        partitions = CreatePartitionsForNewTable(*tableDescription, tablePath);
    }

    // Create new table
    const auto createTableStatus = CreateNewTable(tableClient, partitions, descriptorPtr, newTablePath);
    CheckStatus(createTableStatus, "create table [%s] failed", newTableName.c_str());
}
