//
// HOLLYWOOD FRAMEWORK
// Scenario error definition class
//
// Typical usage:
//    TError err;
//    err.Diags() << "Some " << "details";
//    return err;
// OR
//    HW_ERROR("Some " << "details");
//    // never return
//

#pragma once

#include <util/generic/yexception.h>
#include <util/stream/str.h>

namespace NAlice::NHollywoodFw {

//
// Forward declarations
//
class TProtoHwError;
class TScenario;

class TError {
public:
    //
    // Typical scenario errors
    // Note: Irrelevant is not an error
    // To return Irrelevant answer you have to use TProtoRenderIrrelevant object
    //
    enum class EErrorDefinition {
        Success = 0,      // Error structure is not filled (no error occured)
        Unknown = 1,      // An unknown error occured in scenario
        Exception,        // Exception occured during scenario processing
        ScenarioNotFound, // Scenario with this name is not found
        HandlerNotFound,  // Unable to find a scenario handler for specified name
        ProtobufCast,     // Error casting protobuf object to user-defined class
        Timeout,          // Scenario did not answer for specified time
        NotAllowed,       // Scenario handler can't be called due to scenario restrictions
        HwError,          // HW_ERROR() call
        SubsystemError,   // Unrecovered error occured during subsystem processing (i.e. slot parsing, etc)
        ExternalError,    // External service error (error sending, http error, etc)
    };

    // Internal helper for Diags() messages
    class TIntrLogger {
    public:
        TIntrLogger(TString& str)
            : Output_(str)
        {
        }

        template <typename T>
        TIntrLogger& operator<<(const T& t) {
            Output_ << t;
            return *this;
        }
    private:
        TStringOutput Output_;
    };

    // TError constructor
    // Requires reference to source request and error code for future processing
    // Optionally TError may also have constructed answer for user
    explicit TError(EErrorDefinition err, const char* fileName = nullptr, int lineNumber = 0)
        : Error_(err)
        , Filename_(fileName ? fileName : "")
        , LineNumber_(lineNumber) {
    }
    TError(const TError& rhs) = default;
    TError(const TProtoHwError& proto);

    // Get Details interface to write error details using << operator
    TIntrLogger Details() {
        TIntrLogger stream(Details_);
        return stream;
    }

    // Get pointer to details message
    const TString& GetDetails() const {
        return Details_;
    }

    // Collect additional information from yexception
    void CollectExceptionInfo(const yexception& exceptionInfo);
    // Collect additional information from any exception
    void CollectExceptionInfo();

    EErrorDefinition GetErrorCode() const {
        return Error_;
    }
    bool Defined() const {
        return Error_ != EErrorDefinition::Success;
    }

    TString Print(const TScenario& sc) const;

    void SetNodeStageInfo(const TString& nodeName, const TString& stageName) {
        NodeName_ = nodeName;
        StageName_ = stageName;
    }

    // Internal function bypass error message through apphost nodes
    void ExportToProto(TProtoHwError* proto) const;

private:
    EErrorDefinition Error_; // Error code
    TString Details_;        // Details message (printed to TError::Details() << or with HW_ERROR() macro)
    TString Backtrace_;      // Backtrace information (if present)
    TString What_;           // Exception message (if present)
    TString Filename_;
    int LineNumber_;
    TString NodeName_;       // Node name where error occured
    TString StageName_;
};

//
// Additional hardware exception handler
//
class TFrameworkException: public yexception {
public:
    explicit TFrameworkException(const TString& diags, const char* fileName = nullptr, int lineNumber = 0)
        : Diags_(diags)
        , Filename_(fileName ? fileName : "")
        , LineNumber_(lineNumber) {
    }
    const TString& GetDiags() const {
        return Diags_;
    }
    TStringBuf GetFilename() const {
        return Filename_;
    }
    int GetLineNumber() const {
        return LineNumber_;
    }
private:
    TString Diags_;
    TStringBuf Filename_;
    int LineNumber_;
};

//
// Common HW_ERROR definition for critical errors
//
#define HW_ERROR(diags)    do {                                                             \
        TString tempBuffer;                                                                 \
        TStringOutput output(tempBuffer);                                                   \
        output <<  diags;                                                                   \
        ythrow NAlice::NHollywoodFw::TFrameworkException(tempBuffer, __FILE__, __LINE__);   \
    } while(false)

} // namespace NAlice::NHollywoodFw
