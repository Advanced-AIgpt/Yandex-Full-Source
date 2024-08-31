#include "proto_eval.h"

namespace NAlice {
    namespace {
        using ETraceEvent = TProtoEvaluator::ETraceEvent;

        void WriteTraceToString(TString& buffer, const ETraceEvent event, const TStringBuf expr, const TString& value, size_t nestingLevel) {
            TStringOutput trace{buffer};
            switch (event) {
                case ETraceEvent::EvalBegin:
                case ETraceEvent::EvalEnd:
                    break;
                case ETraceEvent::Error:
                    trace << "Error: " << TString{expr}.Quote() << ": " << value << "\n";
                    break;
                case ETraceEvent::Warning:
                    trace << "Warning: " << TString{expr}.Quote() << ": " << value << "\n";
                    break;
                case ETraceEvent::ProtoValue:
                case ETraceEvent::AggregateValue:
                case ETraceEvent::ExpressionValue:
                    while (nestingLevel--) {
                        trace << "  ";
                    }
                    trace << TString{expr}.Quote() << " = " << value.Quote() << "\n";
                    break;
            }
        }
    } // namespace

    TProtoEvaluator::TTraceCallback TProtoEvaluator::TraceToString(TString& buffer) {
        return [&buffer](auto&&... args) { WriteTraceToString(buffer, args...); };
    }

    TProtoEvaluatorWithTraceLog::TProtoEvaluatorWithTraceLog(TRTLogger& logger)
        : Logger(logger)
    {
        if (Logger.IsSuitable(TLOG_DEBUG)) {
            SetTraceCallback([this](auto&&... args) { TraceCallback(args...); });
        }
    }

    TProtoEvaluatorWithTraceLog::~TProtoEvaluatorWithTraceLog() {
        // withdraw callback to avoid potential TProtoEvaluatorWithTraceLog reference after destruction
        SetTraceCallback({});
    }

    void TProtoEvaluatorWithTraceLog::TraceCallback(const ETraceEvent event, const TStringBuf expr, const TString& value, const size_t nestingLevel) {
        switch (event) {
            case ETraceEvent::EvalBegin:
                Trace.clear();
                break;
            case ETraceEvent::EvalEnd:
                if (Trace) {
                    LOG_DEBUG(Logger) << "Evaluator trace:\n" << Trace;
                }
                break;
            default:
                WriteTraceToString(Trace, event, expr, value, nestingLevel);
                break;
        }
    }

} // namespace NAlice
