//
// HOLLYWOOD FRAMEWORK
// Scenario error definition class
//

#include "error.h"

#include "scenario.h"

#include <alice/hollywood/library/framework/proto/framework_state.pb.h>

namespace NAlice::NHollywoodFw {

TError::TError(const TProtoHwError& proto)
    : Error_(static_cast<EErrorDefinition>(proto.GetErrorCode()))
    , Details_(proto.GetDetails())
    , Backtrace_(proto.GetBacktrace())
    , What_(proto.GetWhat())
    , Filename_(proto.GetFilename())
    , LineNumber_(proto.GetLineNumber())
    , NodeName_(proto.GetNodeName())
    , StageName_(proto.GetStageName())
{
}

void TError::ExportToProto(TProtoHwError* proto) const {
    proto->SetErrorCode(static_cast<int>(Error_));
    proto->SetDetails(Details_);
    proto->SetBacktrace(Backtrace_);
    proto->SetWhat(What_);
    proto->SetFilename(TString(Filename_));
    proto->SetLineNumber(LineNumber_);
    proto->SetNodeName(NodeName_);
    proto->SetStageName(StageName_);
}

/*
    Collect extra information from exception information
    Internal function: called automalically from hwscenario.cpp
*/
void TError::CollectExceptionInfo(const yexception& exceptionInfo) {
    Backtrace_ = FormatCurrentException();
    What_ = exceptionInfo.what();
}

/*
    Collect extra information from exception information
    Internal function: called automalically from hwscenario.cpp
*/
void TError::CollectExceptionInfo() {
    Backtrace_ = FormatCurrentException();
}

TString TError::Print(const TScenario& sc) const {
    TString str;
    TStringOutput out(str);
    out << "An error occured during scenario '" << sc.GetName() << "' processing. " <<
            "Error: " << Error_ << "\n" <<
            "File: " << Filename_ << "(#" << LineNumber_ << ")\n" <<
            "Details: " << GetDetails() << "\n" <<
            "What: " << What_ << "\n" <<
            "NodeName: " << NodeName_ << "\n" <<
            "StageName: " << StageName_ << "\n" <<
            "Backtrace: " << Backtrace_;
    return str;
}

} // namespace NAlice::NHollywoodFw
