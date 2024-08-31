#pragma once

#include <alice/library/proto_eval/proto/expression.pb.h>

#include <alice/library/logger/logger.h>

#include <google/protobuf/message.h>

#include <library/cpp/expression/expression.h>

#include <util/generic/hash.h>
#include <util/string/cast.h>

namespace NAlice {

    class TProtoEvaluator {
    public:
        virtual ~TProtoEvaluator() = default;

        enum class ETraceEvent {
            EvalBegin,
            EvalEnd,
            ProtoValue,
            AggregateValue,
            ExpressionValue,
            Warning,
            Error,
        };

        using TErrorCallback = std::function<void(const TString&)>;
        using TTraceCallback = std::function<void(ETraceEvent, TStringBuf expr, const TString& value, size_t nestingLevel)>;

        /// Set reference to a message by name
        /// \param name  name of the message
        /// \param msg   the message reference
        void SetProtoRef(TStringBuf name, const NProtoBuf::Message& msg);
        void SetProtoRef(TStringBuf name, NProtoBuf::Message&& msg) = delete;

        /// Set parameter value
        void SetParameterValue(TStringBuf name, TString value);

        /// Set error callback
        void SetErrorCallback(TErrorCallback cb);

        /// Set trace callback
        void SetTraceCallback(TTraceCallback cb);

        /// Enable (disable) trace by default
        void SetTraceEnabled(bool enabled = true) {
            TraceEnabled = enabled;
        }

        /// Calculate an expression by rules
        TString EvaluateString(const TProtoEvalExpression& rules);
        bool EvaluateBool(const TProtoEvalExpression& rules);

        template <typename T>
        T Evaluate(const TProtoEvalExpression& rules) {
            return FromString<T>(EvaluateString(rules));
        }
        template <>
        bool Evaluate(const TProtoEvalExpression& rules) {
            return EvaluateBool(rules);
        }
        template <>
        TString Evaluate(const TProtoEvalExpression& rules) {
            return EvaluateString(rules);
        }

        static TTraceCallback TraceToString(TString& buffer);

    private:
        class TParameterProvider;
        friend TParameterProvider;

    private:
        TString GetParameterValue(const TProtoEvalExpression::TParameter& source, const TParameterProvider& params);
        TString GetExpressionValue(TStringBuf expression, const TParameterProvider& params);
        TString Aggregate(const TProtoEvalExpression::TAggregate& aggregate,
                          const NProtoBuf::Message& msg, const NProtoBuf::FieldDescriptor& field);

        const TExpression& GetCachedExpression(TStringBuf expression);
        void Trace(ETraceEvent, TStringBuf expr, const TString& value) const;

    private:
        THashMap<TString, const NProtoBuf::Message*> ProtoByName;
        THashMap<TString, TMaybe<TString>> ParamValueByName;
        THashMap<TString, TExpression> Expressions;
        static constexpr auto EMPTY_CALLBACK = [](auto&&...) {};
        TErrorCallback ErrorCallback = EMPTY_CALLBACK;
        TTraceCallback TraceCallback = EMPTY_CALLBACK;
        size_t NestingLevel{};
        bool TraceEnabled{};
    };

    class TProtoEvaluatorWithTraceLog : public TProtoEvaluator {
    public:
        TProtoEvaluatorWithTraceLog(TRTLogger& logger);
        ~TProtoEvaluatorWithTraceLog() override;

    private:
        TRTLogger& Logger;
        TString Trace;

    private:
        void TraceCallback(ETraceEvent, TStringBuf expr, const TString& value, size_t nestingLevel);
    };

} // namespace NAlice
