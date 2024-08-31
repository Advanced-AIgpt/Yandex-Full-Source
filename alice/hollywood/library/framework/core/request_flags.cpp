//
// HOLLYWOOD FRAMEWORK
// Base request information
//

#include "request.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/request/experiments.h>

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/string/cast.h>

namespace NAlice::NHollywoodFw {

namespace {

TMaybe<TString> ConvertExpValue(const google::protobuf::Value& val) {
    // if (!exp.second.string_value())
    if (val.has_null_value()) {
        return Nothing();
    }
    return val.string_value();
}

}

/*
    TRequest::TFlags class functions
    Read all flags and fill two arrays:
        Experiments_ -
        SubvalExperiments_
*/
TRequest::TFlags::TFlags(const NScenarios::TScenarioBaseRequest& baseRequestProto, TRTLogger& logger) {
    const google::protobuf::Struct& experiments = baseRequestProto.GetExperiments();
    for (const auto& exp : experiments.fields()) {
        // Check key=value:1 version
        const size_t pos = exp.first.find('=');
        if (pos != TString::npos) {
            // Save it to ShortExp_ list
            const auto val = ConvertExpValue(exp.second);
            if (val.Defined() && *val != "0" && *val != "1") {
                LOG_WARNING(logger) << "Experiment '" << exp.first << "' has unexpected key value '" << *val << "'.";
            }
            SubvalExperiments_.push_back(TExpWithSubvalues{exp.first.substr(0, pos), exp.first.substr(pos+1), val});
        }
        // Save key: value array
        // NormalExp_ will be saved always, even if this experiment was stored in ShortExp_
        Experiments_[exp.first] = ConvertExpValue(exp.second);
    }
};

/*
    Check experiment flags
    @param name name of experiment
    @return true of experiment flag was set
            flase if experiment is not found or value set to "0"
*/
bool TRequest::TFlags::IsExperimentEnabled(TStringBuf key) const {
    const auto& it = Experiments_.find(key);
    if (it == Experiments_.end()) {
        return false;
    }
    if (!it->second.Defined() || *(it->second) == "0") {
        return false;
    }
    return true;
}

/*
    Check is this key is a complex (i.e. has subvalue) or not
*/
bool TRequest::TFlags::IsKeyComplex(TStringBuf primaryKey) const {
    for (const auto& it : SubvalExperiments_) {
        if (it.MainKey == primaryKey) {
            return true;
        }
    }
    return false;
}

} // namespace NAlice::NHollywoodFw
