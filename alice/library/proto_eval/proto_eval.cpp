#include "proto_eval.h"

#include <library/cpp/regex/pire/regexp.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/wrappers.pb.h>

#include <util/generic/array_ref.h>
#include <util/generic/maybe.h>
#include <util/generic/variant.h>
#include <util/generic/xrange.h>
#include <util/string/split.h>

namespace NAlice {
    namespace {
        /// TScopedMapChanger can change a map and automatically restore it on scope exit
        /// @note  Keys of the map should be never erased; the map value is set to default instead of erasing
        template <typename T>
        class TScopedMapChanger {
            using TMapValue = std::conditional_t<std::is_pointer_v<T>, T, TMaybe<T>>;
            using TMapType = THashMap<TString, TMapValue>;
            using TSavedMapType = THashMap<TStringBuf, TMapValue>;

        public:
            explicit TScopedMapChanger(TMapType& map)
                : Map(map)
            {
            }

            ~TScopedMapChanger() {
                for (auto& [key, value] : SavedValues) {
                    Map[key] = std::move(value);
                }
            }

            void Set(const TString& key, T&& value) {
                const auto it = Map.find(key);
                if (it != Map.end()) {
                    SavedValues.try_emplace(it->first, std::move(it->second));
                    it->second = std::move(value);
                } else {
                    const auto it = Map.try_emplace(key, std::move(value)).first;
                    SavedValues.try_emplace(it->first, Default<TMapValue>());
                }
            }

            const TMapType* operator->() const {
                return &Map;
            }

        private:
            TMapType& Map;
            TSavedMapType SavedValues;
        };

        template <typename T>
        class TScopedChange {
        public:
            explicit TScopedChange(T& var, T value)
                : Var(var)
                , SavedValue(std::move(value))
            {
                std::swap(Var, SavedValue);
            }

            ~TScopedChange()
            {
                std::swap(Var, SavedValue);
            }

        private:
            T& Var;
            T SavedValue;
        };

        template <typename T>
        class TScopedIncrease {
        public:
            explicit TScopedIncrease(T& count)
                : Count(++count)
            {}

            ~TScopedIncrease()
            {
                --Count;
            }

        private:
            T& Count;
        };

        bool ConvertToBool(TStringBuf value) {
            bool boolValue;
            if (TryFromString<bool>(value, boolValue)) {
                return boolValue;
            }
            double number;
            if (TryFromString<double>(value, number)) {
                return number != 0.0;
            }
            return false;
        }

        class IAggregator {
        public:
            virtual ~IAggregator() = default;
            /// @retval false continue aggregation
            /// @retval true  aggregation is finished
            virtual bool AddValue(TStringBuf value) = 0;
            [[nodiscard]] virtual TString GetAggregate() const = 0;
        };

        class TAggregatorCount : public IAggregator {
        public:
            bool AddValue(TStringBuf) override {
                ++Count;
                return false;
            }
            [[nodiscard]] TString GetAggregate() const override {
                return ToString(Count);
            }

        private:
            size_t Count = 0;
        };

        class TAggregatorSum : public IAggregator {
        public:
            bool AddValue(TStringBuf val) override {
                double number;
                if (TryFromString<double>(val, number)) {
                    Sum += number;
                    ++Count;
                }
                return false;
            }
            [[nodiscard]] TString GetAggregate() const override {
                return Count ? ToString(Sum) : "";
            }

        protected:
            double Sum = 0;
            size_t Count = 0;
        };

        class TAggregatorAvg : public TAggregatorSum {
        public:
            [[nodiscard]] TString GetAggregate() const override {
                return Count ? ToString(Sum / Count) : "";
            }
        };

        class TAggregatorBool : public IAggregator {
        public:
            explicit TAggregatorBool(bool final)
                : Final(final)
            {
            }
            bool AddValue(TStringBuf val) override {
                Value = ConvertToBool(val);
                return *Value == Final;
            }
            [[nodiscard]] TString GetAggregate() const override {
                return Value.Defined() ? ToString(Value.GetRef()) : "";
            }

        private:
            TMaybe<bool> Value;
            const bool Final;
        };

        class TAggregatorFirst : public IAggregator {
            bool AddValue(TStringBuf val) override {
                Value = val;
                return true;
            }
            [[nodiscard]] TString GetAggregate() const override {
                return Value;
            }
            TString Value;
        };

        class TAggregatorBest : public IAggregator {
            bool AddValue(TStringBuf val) override {
                if (val.empty()) {
                    return false;
                }
                if (TryNumber) {
                    double number;
                    if (TryFromString(val, number)) {
                        if (BestNumber.Empty() || IsBetter(number, *BestNumber)) {
                            BestNumber = number;
                        }
                    } else {
                        TryNumber = false;
                    }
                }
                if (BestValue.Empty() || IsBetter(val, *BestValue)) {
                    BestValue = val;
                }
                return false;
            }
            [[nodiscard]] TString GetAggregate() const override {
                return TryNumber && BestNumber.Defined() ? ToString(*BestNumber) : BestValue.GetOrElse({});
            }

            /// Is the first arg better than the second?
            virtual bool IsBetter(double, double) const = 0;
            virtual bool IsBetter(TStringBuf, TStringBuf) const = 0;

            TMaybe<TString> BestValue;
            TMaybe<double> BestNumber;
            bool TryNumber = true;
        };

        class TAggregatorMin : public TAggregatorBest {
            bool IsBetter(double a, double b) const override {
                return a < b;
            }
            bool IsBetter(TStringBuf a, TStringBuf b) const override {
                return a < b;
            }
        };
        class TAggregatorMax : public TAggregatorBest {
            bool IsBetter(double a, double b) const override {
                return a > b;
            }
            bool IsBetter(TStringBuf a, TStringBuf b) const override {
                return a > b;
            }
        };

        THolder<IAggregator> MakeAggregator(TProtoEvalExpression::EReducer func) {
            switch (func) {
                default:
                case TProtoEvalExpression::Count:
                    return MakeHolder<TAggregatorCount>();
                case TProtoEvalExpression::Sum:
                    return MakeHolder<TAggregatorSum>();
                case TProtoEvalExpression::Min:
                    return MakeHolder<TAggregatorMin>();
                case TProtoEvalExpression::Max:
                    return MakeHolder<TAggregatorMax>();
                case TProtoEvalExpression::Avg:
                    return MakeHolder<TAggregatorAvg>();
                case TProtoEvalExpression::And:
                    return MakeHolder<TAggregatorBool>(false);
                case TProtoEvalExpression::Or:
                    return MakeHolder<TAggregatorBool>(true);
                case TProtoEvalExpression::First:
                    return MakeHolder<TAggregatorFirst>();
            }
        }

        std::tuple<TStringBuf, TStringBuf, TMaybe<TString>> ParseFieldIndex(const TStringBuf part) {
            const size_t openBracket = part.find('[');
            if (openBracket == TStringBuf::npos) {
                return {part, {}, {}};
            }

            TStringBuf fieldName;
            TStringBuf idxPart;
            part.SplitOn(openBracket, fieldName, idxPart);

            if (!idxPart.ChopSuffix("]")) {
                return {part, {}, "Index should be enclosed in brackets"};
            }
            return {fieldName, idxPart, {}};
        }

        TString GetFieldValue(const NProtoBuf::Message& msg, const NProtoBuf::FieldDescriptor& field, int idx = -1) {
            TString outString;
            const bool repeated = field.is_repeated();
            const auto reflection = msg.GetReflection();
            switch (field.cpp_type()) {
                case NProtoBuf::FieldDescriptor::CPPTYPE_BOOL:
                    outString = (repeated ? reflection->GetRepeatedBool(msg, &field, idx) : reflection->GetBool(msg, &field)) ? "1" : "0";
                    break;
                case NProtoBuf::FieldDescriptor::CPPTYPE_STRING:
                    outString = repeated ? reflection->GetRepeatedString(msg, &field, idx) : reflection->GetString(msg, &field);
                    break;
                default:
                    NProtoBuf::TextFormat::PrintFieldValueToString(msg, &field, repeated ? idx : -1, &outString);
                    break;
            }
            return outString;
        }

        TStringBuf GetParameterName(const TProtoEvalExpression::TParameter& param) {
            return param.GetName() ?: TStringBuf{param.GetPath()}.RAfter('.');
        }

        TString GetParameterTracePath(const TProtoEvalExpression::TParameter& param) {
            return param.HasAggregate()
                ? TString::Join(TProtoEvalExpression_EReducer_Name(param.GetAggregate().GetReducer()), "(", param.GetPath(), ")")
                : param.GetPath();
        }

        TString GetParameterTraceName(const TProtoEvalExpression::TParameter& param) {
            const auto name = GetParameterName(param);
            const auto path = GetParameterTracePath(param);
            return name == path ? TString{name} : TString::Join(name, "=", path);
        }

        size_t GetFieldSize(const NProtoBuf::Message& msg, const NProtoBuf::FieldDescriptor& field) {
            const auto& reflection = *msg.GetReflection();
            if (field.is_repeated()) {
                return reflection.FieldSize(msg, &field);
            }
            if (reflection.HasField(msg, &field)) {
                return 1;
            }
            // Assume that primitive types are always present for proto3 (except for oneof members),
            // otherwise default primitive values are treated as missed.
            return field.cpp_type() != field.CPPTYPE_MESSAGE && !field.containing_oneof() &&
                   field.file()->syntax() >= NProtoBuf::FileDescriptor::SYNTAX_PROTO3;
        }

        const NProtoBuf::Message* GetFieldMessage(const NProtoBuf::Message& msg, const NProtoBuf::FieldDescriptor& field, size_t idx) {
            if (field.cpp_type() != NProtoBuf::FieldDescriptor::CPPTYPE_MESSAGE) {
                return nullptr;
            }
            const auto& reflection = *msg.GetReflection();
            return field.is_repeated() ? &reflection.GetRepeatedMessage(msg, &field, idx) : &reflection.GetMessage(msg, &field);
        }

        using TMapEntry = std::pair<const NProtoBuf::Message*, const NProtoBuf::FieldDescriptor*>;
        TMaybe<TMapEntry> FindMapEntry(const NProtoBuf::Message& msg, const NProtoBuf::FieldDescriptor& field, TStringBuf key) {
            TMaybe<TMapEntry> result;
            const auto& reflection = *msg.GetReflection();
            for (const size_t i : xrange(reflection.FieldSize(msg, &field))) {
                const auto& entry = reflection.GetRepeatedMessage(msg, &field, i);
                const auto* const entryDescriptorPtr = entry.GetDescriptor();
                const auto* const keyField = entryDescriptorPtr->FindFieldByName("key");
                if (keyField && GetFieldValue(entry, *keyField) == key) {
                    const auto* const valueField = entryDescriptorPtr->FindFieldByName("value");
                    if (valueField) {
                        result.ConstructInPlace(&entry, valueField);
                    }
                    break;
                }
            }
            return result;
        }
    } // anonymous namespace

    class TProtoEvaluator::TParameterProvider : public IExpressionAdaptor {
    public:
        explicit TParameterProvider(TProtoEvaluator& eval)
            : Evaluator(eval)
            , Params(eval.ParamValueByName)
        {
        }

        void Set(TStringBuf key, TString&& value) {
            Params.Set(TString{key}, std::move(value));
        }

        bool FindValue(const TString& name, TString& value) const override {
            const auto it = Params->find(name);
            if (it == Params->end() || !it->second) {
                constexpr TStringBuf numberFirstChar = "0123456789-.";
                if (name.Contains('.') && !numberFirstChar.Contains(name.front())) {
                    TProtoEvalExpression::TParameter param;
                    param.SetName(name);
                    param.SetPath(name);
                    TScopedIncrease LevelUp{Evaluator.NestingLevel};
                    value = Evaluator.GetParameterValue(param, *this);
                    Params.Set(name, TString{value});
                    return true;
                }
                return false;
            }
            value = *it->second;
            return true;
        }

    private:
        TProtoEvaluator& Evaluator;
        mutable TScopedMapChanger<TString> Params;
    };

    void TProtoEvaluator::SetProtoRef(TStringBuf name, const NProtoBuf::Message& msg) {
        ProtoByName[name] = &msg;
    }

    void TProtoEvaluator::SetParameterValue(TStringBuf name, TString value) {
        ParamValueByName[name] = std::move(value);
    }

    void TProtoEvaluator::SetErrorCallback(TErrorCallback cb) {
        ErrorCallback = cb ? std::move(cb) : EMPTY_CALLBACK;
    }

    void TProtoEvaluator::SetTraceCallback(TTraceCallback cb) {
        TraceCallback = cb ? std::move(cb) : EMPTY_CALLBACK;
    }

    TString TProtoEvaluator::EvaluateString(const TProtoEvalExpression& rules) {
        const auto& parameters = rules.GetParameters();
        const size_t paramCount = parameters.size();

        const TStringBuf expression = rules.GetExpression() ?: (paramCount ? GetParameterName(parameters[paramCount - 1]) : TStringBuf{});
        if (!expression) {
            return {};
        }

        TScopedChange traceChange{TraceEnabled, rules.HasTraceEnabled() ? rules.GetTraceEnabled().value() : TraceEnabled};
        Trace(ETraceEvent::EvalBegin, expression, {});

        TParameterProvider parameterProvider(*this);
        for (const auto& param : parameters) {
            parameterProvider.Set(GetParameterName(param), GetParameterValue(param, parameterProvider));
        }

        auto result = GetExpressionValue(expression, parameterProvider);
        Trace(ETraceEvent::EvalEnd, expression, result);
        return result;
    }

    bool TProtoEvaluator::EvaluateBool(const TProtoEvalExpression& rules) {
        return ConvertToBool(EvaluateString(rules));
    }

    TString TProtoEvaluator::GetParameterValue(const TProtoEvalExpression::TParameter& source, const TParameterProvider& params) {
        const TVector<TStringBuf> parts = StringSplitter(source.GetPath()).Split('.').ToList<TStringBuf>();
        if (parts.size() < 2) {
            ErrorCallback("Protobuf name expected");
            Trace(ETraceEvent::Error, source.GetPath(), "Protobuf name expected");
            return {};
        }

        const TStringBuf protoName = parts[0];
        const auto it = ProtoByName.find(protoName);
        if (it == ProtoByName.end() || !it->second) {
            ErrorCallback(TString::Join("Protobuf with name ", TString{protoName}.Quote(), " is not defined"));
            Trace(ETraceEvent::Error, protoName, "Protobuf is not defined");
            return {};
        }
        const NProtoBuf::Message* msgPtr = it->second;

        TString outString;
        const size_t lastPartNo = parts.size() - 1;
        for (const size_t partNo : xrange(size_t{1}, parts.size())) {
            const auto [fieldName, idxExpr, optError] = ParseFieldIndex(parts[partNo]);
            if (optError) {
                ErrorCallback(*optError);
                Trace(ETraceEvent::Error, parts[partNo], *optError);
                break;
            }
            const auto* fDescPtr = msgPtr->GetDescriptor()->FindFieldByName(TString(fieldName));
            if (!fDescPtr) {
                ErrorCallback(TString::Join("Field ", TString{fieldName}.Quote(), " is not found"));
                Trace(ETraceEvent::Error, fieldName, "Field is not found");
                break;
            }

            const bool last = (partNo == lastPartNo);
            if (last && source.HasAggregate()) {
                if (!idxExpr.empty()) {
                    ErrorCallback("Aggregate path must not have a final index");
                    Trace(ETraceEvent::Error, source.GetPath(), "Aggregate path must not have a final index");
                    break;
                }
                outString = Aggregate(source.GetAggregate(), *msgPtr, *fDescPtr);
                break;
            }

            size_t size = GetFieldSize(*msgPtr, *fDescPtr);
            size_t idx = 0;

            if (!idxExpr.empty()) {
                if (!fDescPtr->is_repeated()) {
                    ErrorCallback(TString::Join("Field ", TString{fieldName}.Quote(), " is not repeated"));
                    Trace(ETraceEvent::Error, fieldName, "Field is not repeated");
                    break;
                }
                const TString idxValueStr = GetExpressionValue(TString{idxExpr}, params);
                if (fDescPtr->is_map()) {
                    if (const auto entry = FindMapEntry(*msgPtr, *fDescPtr, idxValueStr)) {
                        std::tie(msgPtr, fDescPtr) = *entry;
                        size = 1;
                    } else {
                        ErrorCallback(TString::Join("Index value ", idxValueStr.Quote(), " is not found in map"));
                        Trace(ETraceEvent::Warning, idxValueStr, "Index value is not found in map");
                        size = 0;
                    }
                } else {
                    if (!TryFromString<size_t>(idxValueStr, idx)) {
                        ErrorCallback(TString::Join("Index value ", idxValueStr.Quote(), " is not numeric"));
                        Trace(ETraceEvent::Warning, idxValueStr, "Index value is not numeric");
                    }
                    if (idx >= size) {
                        ErrorCallback(TString::Join("Index ", idxValueStr, " is out of range for field ",
                                                    TString{fieldName}.Quote()));
                        Trace(ETraceEvent::Warning, idxValueStr, "Index is out of range");
                    }
                }
            }

            bool tryFieldMethod = (partNo + 1 == lastPartNo);
            if (idx >= size) {
                size = 0;
                tryFieldMethod = !last;
            }
            if (tryFieldMethod) {
                TStringBuf fn = parts[lastPartNo];
                if (fn.ChopSuffix("()")) {
                    if (fn == "size") {
                        outString = ToString(size);
                        break;
                    } else if (fn == "empty") {
                        outString = ToString(size == 0);
                        break;
                    }
                    ErrorCallback(TString::Join("Unknown field function ", TString{fn}.Quote()));
                    Trace(ETraceEvent::Error, fn, "Unknown field function");
                    break;
                }
            }

            if (idxExpr.empty() && fDescPtr->is_repeated()) {
                ErrorCallback(TString::Join("Field ", TString{fieldName}.Quote(), " is repeated, index is missing"));
                Trace(ETraceEvent::Error, fieldName, "Field is repeated, index is missing");
                break;
            }
            if (idx >= size) {
                break;
            }
            if (last) {
                outString = GetFieldValue(*msgPtr, *fDescPtr, idx);
                break;
            }
            msgPtr = GetFieldMessage(*msgPtr, *fDescPtr, idx);
            if (!msgPtr) {
                ErrorCallback(TString::Join("Field ", TString{fieldName}.Quote(), " is not a message"));
                Trace(ETraceEvent::Error, fieldName, "Field is not a message");
                break;
            }
        }
        Trace(source.HasAggregate() ? ETraceEvent::AggregateValue : ETraceEvent::ProtoValue, GetParameterTraceName(source), outString);
        return outString;
    }

    const TExpression& TProtoEvaluator::GetCachedExpression(const TStringBuf expression) {
        const auto [it, inserted] = Expressions.try_emplace(expression, expression);
        auto& cachedExpr = it->second;
        if (inserted) {
            cachedExpr.SetRegexMatcher([](TStringBuf str, TStringBuf rx) {
                const auto opts = NRegExp::TFsm::TOptions{}.SetCharset(CODES_UTF8);
                return NRegExp::TMatcher{NRegExp::TFsm{rx, opts}}.Match(str).Final();
            });
        }
        return cachedExpr;
    }

    TString TProtoEvaluator::GetExpressionValue(const TStringBuf expression, const TParameterProvider& params) {
        const TString value = GetCachedExpression(expression).CalcExpressionStr(static_cast<const IExpressionAdaptor&>(params));
        Trace(ETraceEvent::ExpressionValue, expression, value);
        return value;
    }

    TString TProtoEvaluator::Aggregate(const TProtoEvalExpression::TAggregate& aggregate, const NProtoBuf::Message& msg,
                                       const NProtoBuf::FieldDescriptor& field) {
        TScopedIncrease LevelUp{NestingLevel};
        const TString& name = aggregate.GetName() ?: field.name();
        const auto reducer = aggregate.GetReducer();
        if (reducer == TProtoEvalExpression::Undefined) {
            ErrorCallback("Reducer is undefined");
            Trace(ETraceEvent::Error, name, "Reducer is undefined");
            return {};
        }
        const bool repeated = field.is_repeated();
        const bool fieldIsMessage = (field.cpp_type() == NProtoBuf::FieldDescriptor::CPPTYPE_MESSAGE);
        const auto reflectionPtr = msg.GetReflection();
        const size_t size = GetFieldSize(msg, field);

        if (reducer == TProtoEvalExpression::Count && !aggregate.HasFilter()) {
            return ToString(size);
        }

        const TString idxName = TString::Join(name, "Index");

        TScopedMapChanger<const NProtoBuf::Message*> scopedProto{ProtoByName};
        TScopedMapChanger<TString> scopedParams{ParamValueByName};
        auto aggregator = MakeAggregator(aggregate.GetReducer());
        for (const size_t idx : xrange(size)) {
            if (fieldIsMessage) {
                const auto& fieldMsg = repeated ? reflectionPtr->GetRepeatedMessage(msg, &field, idx)
                                                : reflectionPtr->GetMessage(msg, &field);
                scopedProto.Set(name, &fieldMsg);
            } else {
                TScopedIncrease LevelUp{NestingLevel};
                auto value = GetFieldValue(msg, field, idx);
                Trace(ETraceEvent::ProtoValue, name, value);
                scopedParams.Set(name, std::move(value));
            }
            scopedParams.Set(idxName, ToString(idx));

            if (!aggregate.HasFilter() || EvaluateBool(aggregate.GetFilter())) {
                const TString& value = aggregate.HasValue() ? EvaluateString(aggregate.GetValue()) : (fieldIsMessage ? TString{} : *scopedParams->at(name));
                if (aggregator->AddValue(value)) {
                    break;
                }
            }
        }
        return aggregator->GetAggregate();
    }

    void TProtoEvaluator::Trace(ETraceEvent event, TStringBuf expr, const TString& value) const {
        if (!TraceEnabled) {
            return;
        }
        if (event == ETraceEvent::ExpressionValue && (expr == value || expr == value.Quote())) {
            // don't trace trivial cases
            return;
        }
        if (NestingLevel != 0 && (event == ETraceEvent::EvalBegin || event == ETraceEvent::EvalEnd)) {
            // begin/end are not traced in nested expressions
            return;
        }
        TraceCallback(event, expr, value, NestingLevel);
    }

} // namespace NAlice
