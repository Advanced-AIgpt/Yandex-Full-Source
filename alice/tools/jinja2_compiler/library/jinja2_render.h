#pragma once

#include "jinja2cpp/value.h"
#include <contrib/libs/jinja2cpp/include/jinja2cpp/template.h>
#include <contrib/libs/jinja2cpp/include/jinja2cpp/template_env.h>

#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/message.h>

#include <util/generic/map.h>

namespace NAlice {

class TJinja2Render {
public:
    TJinja2Render();

    inline jinja2::TemplateEnv& GetTemplateEnv() {
        return TemplateEnvironment_;
    }
    void AddConfigArray(const TStringBuf& name, const TVector<TString>& values);
    bool LoadTemplateString(const std::string& templateString);
    bool LoadTemplateFile(const std::string& templateName);
    std::string Render();

private:
    jinja2::TemplateEnv TemplateEnvironment_;
    jinja2::Template Environment_;
};

} // namespace NAlice
