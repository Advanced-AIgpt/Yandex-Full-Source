#pragma once

#include <alice/cachalot/library/config/load.h>
#include <alice/cachalot/library/cachalot.h>

#include <voicetech/library/evlogdump/evlogdump.h>
#include <library/cpp/getopt/small/modchooser.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>


class TCachalotMainClass : public TMainClass {
public:
    explicit TCachalotMainClass(TString installation)
        : Installation(std::move(installation))
    {
    }

    int operator()(int argc, const char** argv);

    TString GetModeName() const;

    TString GetDefaultConfigResorce() const;

    TString GetHelpString() const;

private:
    TString Installation;
};

namespace NCachalot {
    static const TVector<TCachalotMainClass> ExecuteConfigOptions = {
        TCachalotMainClass("activation"),
        TCachalotMainClass("beta"),
        TCachalotMainClass("context"),
        TCachalotMainClass("gdpr"),
        TCachalotMainClass("mm"),
        TCachalotMainClass("tts")
    };
}
