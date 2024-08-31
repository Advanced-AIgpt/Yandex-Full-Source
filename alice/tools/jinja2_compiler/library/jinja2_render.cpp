#include "jinja2_render.h"
#include "google/protobuf/descriptor.h"
#include "jinja2cpp/value.h"
#include "string_convertions.h"
#include "util/string/ascii.h"
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/protos/extensions/extensions.pb.h>

#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/message.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/io/tokenizer.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/string/split.h>
#include <util/string/printf.h>
#include <util/stream/format.h>

namespace NAlice {

//
// Ctor
//
TJinja2Render::TJinja2Render() :
Environment_(&TemplateEnvironment_) {
    //TemplateEnvironment_.GetSettings().trimBlocks = true;
    //TemplateEnvironment_.GetSettings().lstripBlocks = true;
}

void TJinja2Render::AddConfigArray(const TStringBuf& name, const TVector<TString>& values) {
    static bool firstCall = true;
    jinja2::ValuesList valueList;

    for (const auto& it: values) {
        valueList.push_back(jinja2::Value(it.c_str()));
    }
    if (firstCall) {
        firstCall = false;
        // Add variable config with specified value
        jinja2::ValuesMap valuesMap;
        valuesMap[name.Data()] = valueList;
        TemplateEnvironment_.AddGlobal("config", valuesMap);
    } else {
        // Enumerate variables to find and modify "config" var
        auto fn = [valueList, name](jinja2::ValuesMap& map) {
            for (auto& it : map) {
                if (it.first == "config") {
                    it.second.asMap()[name.Data()] = valueList;
                    break;
                }
            }
        };
        TemplateEnvironment_.ApplyGlobals(fn);
    }
}

//
// Load template from string
// This function is used in unittests only
//
bool TJinja2Render::LoadTemplateString(const std::string& templateString) {
    const jinja2::Result<void> res = Environment_.Load(templateString);
    return res.has_value();
}

//
// Load template from file
//
bool TJinja2Render::LoadTemplateFile(const std::string& templateName) {
    const jinja2::Result<void> res = Environment_.LoadFromFile(templateName);
    return res.has_value();
}

//
// Render template and return destination string (whole file)
//
std::string TJinja2Render::Render() {
    try {
        jinja2::Result<std::string> res = Environment_.RenderAsString({});
        if (res.has_value()) {
            return res.value();
        }
        Cout << "Error while parsing" << Endl;
        const auto& errorFile = res.error().GetErrorLocation();

        Cout << "Error " << static_cast<int>(res.error().GetCode()) << Endl;
        Cout << "(" << errorFile.line << "#" << errorFile.col << "): " << errorFile.fileName << Endl;
        const auto& related = res.error().GetRelatedLocations();
        for (const auto& it : related) {
            Cout << "Called from: (" << it.line << "#" << it.col << "): " << it.fileName << Endl;
        }
        Cout << "Description: " << res.error().GetLocationDescr() << Endl;
    } catch (jinja2::ErrorInfo e) {
        Cout << "Error in parsing" << Endl;
        Cout << e.GetLocationDescr() << Endl;
    } catch (...) {
        Cout << "Error in parsing!!!" << Endl;
    }
    return "";
}

} // namespace NAlice
