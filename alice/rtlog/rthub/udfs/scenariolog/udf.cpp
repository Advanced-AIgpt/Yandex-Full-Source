#include <alice/rtlog/rthub/udfs/scenariolog/parser.h>
#include <alice/rtlog/rthub/protos/rtlog.pb.h>
#include <alice/rtlog/common/ydb_helpers.h>
#include <alice/library/go/setrace/protos/log.pb.h>

#include <apphost/lib/event_log/decl/events.ev.pb.h>

#include <robot/rthub/yql/generic_protos/serialized_message.pb.h>

#include <yql/library/protobuf_udf/type_builder.h>
#include <yql/library/protobuf_udf/value_builder.h>
#include <yql/library/protobuf_udf/proto_builder.h>
#include <ydb/library/yql/public/udf/udf_value.h>
#include <ydb/library/yql/public/udf/udf_helpers.h>
#include <ydb/library/yql/public/udf/udf_type_builder.h>
#include <ydb/library/yql/minikql/mkql_string_util.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/string/printf.h>
#include <util/datetime/base.h>

namespace NRTLog {
    using namespace NRobot;
    using namespace google::protobuf;
    using namespace NKikimr::NUdf;

    TString GetStringArg(const TUnboxedValuePod* args, size_t index, TStringBuf name) {
        TString result(args[index].AsStringRef());
        Y_VERIFY(!result.empty(), "parameter [%s] not set", name.data());
        return result;
    }

    struct TParseScenarioLogEnvironment {
        TProtoInfo InputProto;
        TProtoInfo OutputItemProto;
        TScenarioLogParserCounters Counters;
    };

    class TParseScenarioLogFunction final : public TBoxedValue {
    public:
        explicit TParseScenarioLogFunction(TParseScenarioLogEnvironment &environment,
                                      TYdbSettings ydbSettings,
                                      const TString &protoDirectory)
                : Environment(environment),
                  ScenarioLogParser(MakeScenarioLogParser(std::move(ydbSettings), protoDirectory, Environment.Counters)) {
        }

    private:
        TUnboxedValue Run(const IValueBuilder* valueBuilder, const TUnboxedValuePod* args) const override {
            try {
                TSerializedMessage serializedMessage;
                FillProtoFromValue(args[0], serializedMessage, Environment.InputProto);
                Y_VERIFY(serializedMessage.GetFormat() == NRobot::MF_BINARY);
                Y_VERIFY(!serializedMessage.GetDecompress());
                TStringBuf chunk(reinterpret_cast<const char *>(serializedMessage.GetDataPtr()),
                                 serializedMessage.GetDataSize());
                const auto records = ScenarioLogParser->Parse(chunk);
                return ToUnboxedValues(records, valueBuilder);
            } catch (...) {
                UdfTerminate(CurrentExceptionMessage().c_str());
            }
        }

    private:
        template<typename T>
        TUnboxedValue ToUnboxedValues(const TVector<T>& source, const IValueBuilder* valueBuilder) const {
            TUnboxedValue *items = nullptr;
            auto result = valueBuilder->NewArray(static_cast<ui32>(source.size()), items);
            for (size_t i = 0; i < source.size(); ++i) {
                items[i] = FillValueFromProto(source[i], valueBuilder, Environment.OutputItemProto);
            }
            return result;
        }

    private:
        TParseScenarioLogEnvironment &Environment;
        THolder<IScenarioLogParser> ScenarioLogParser;
    };

    class TParseScenarioLogFunctionFactory final : public TBoxedValue {
    public:
        static void DeclareSignature(bool typesOnly, IFunctionTypeInfoBuilder& builder) {
            auto environment = MakeHolder<TParseScenarioLogEnvironment>();
            environment->Counters.ErrorsCount = builder.GetCounter("ParseErrorsCount", true);
            environment->Counters.StrippedBytesCount = builder.GetCounter("StrippedBytesCount", true);
            environment->Counters.StrippedMessagesCount = builder.GetCounter("StrippedMessagesCount", true);
            environment->Counters.BadUtf8BytesCount = builder.GetCounter("BadUtf8BytesCount", true);
            environment->Counters.BadUtf8MessagesCount = builder.GetCounter("BadUtf8MessagesCount", true);
            ProtoTypeBuild(TSerializedMessage::descriptor(), EEnumFormat::Number,
                           ERecursionTraits::Fail, true, builder, &environment->InputProto);
            ProtoTypeBuild(TRecord::descriptor(), EEnumFormat::Number,
                           ERecursionTraits::Fail, true, builder, &environment->OutputItemProto);
            auto *functionSignature = builder
                    .Callable(1)
                    ->Arg(environment->InputProto.StructType)
                    .Returns(builder.List()->Item(environment->OutputItemProto.StructType).Build())
                    .Build();
            builder
                    .Args(4)
                    ->Add(builder.SimpleType<char *>()).Name("endpoint")
                    .Add(builder.SimpleType<char *>()).Name("database")
                    .Add(builder.SimpleType<char *>()).Name("authToken")
                    .Add(builder.SimpleType<char *>()).Name("proto_directory")
                    .Done()
                    .Returns(functionSignature);
            if (!typesOnly) {
                builder.Implementation(new TParseScenarioLogFunctionFactory(std::move(environment)));
            }
        }

    private:
        explicit TParseScenarioLogFunctionFactory(THolder<TParseScenarioLogEnvironment>&& environment)
                : Environment(std::move(environment)) {
        }

        TUnboxedValue Run(const IValueBuilder*, const TUnboxedValuePod* args) const override {
            try {
                return TUnboxedValuePod(new TParseScenarioLogFunction(*Environment, {
                                                                         GetStringArg(args, 0, TStringBuf("endpoint")),
                                                                         GetStringArg(args, 1, TStringBuf("database")),
                                                                         TString(args[2].AsStringRef())
                                                                 },
                                                                 TString(args[3].AsStringRef())));
            } catch (...) {
                UdfTerminate(CurrentExceptionMessage().c_str());
            }
        }

    private:
        THolder<TParseScenarioLogEnvironment> Environment;
    };

    class TRTLogUdfScenarioLogModule final : public IUdfModule {
    public:
        TStringRef Name() const {
            return TStringRef::Of("RTLogScenarioLogUdf");
        }

        void CleanupOnTerminate() const override {
        }

        void GetAllFunctions(IFunctionsSink& sink) const override {
            sink.Add(TStringRef::Of("ParseScenarioLog"));
        }

        void BuildFunctionTypeInfo(const TStringRef& name, TType*, const TStringRef&, ui32 flags,
                                   IFunctionTypeInfoBuilder &builder) const override {
            const bool typesOnly = (flags & TFlags::TypesOnly) != 0;
            if (name == "ParseScenarioLog") {
                try {
                    TParseScenarioLogFunctionFactory::DeclareSignature(typesOnly, builder);
                }
                catch (...) {
                    builder.SetError(CurrentExceptionMessage());
                }
                return;
            }
            builder.SetError(TStringBuilder() << "unknown function [" << name << "]");
        }
    };

    REGISTER_MODULES(TRTLogUdfScenarioLogModule)
}
