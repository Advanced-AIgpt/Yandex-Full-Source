#pragma once

#include <alice/matrix/library/service_runner/service_runner.h>

#include <library/cpp/getopt/modchooser.h>

#include <voicetech/library/evlogdump/evlogdump.h>

namespace NMatrix {

template <
    const char* ServiceName,
    typename TServicesCommonContextBuilder,
    typename ...TServices
>
int RunDaemon(int argc, const char* argv[]) {
    TModChooser modChooser;

    modChooser.AddMode(
        "run",
        NApplication::Run<
            ServiceName,
            TServicesCommonContextBuilder,
            TServices...
        >,
        TString::Join("Run ", ServiceName, " daemon.")
    );

    modChooser.AddMode(
        "evlogdump",
        EventLogDumpMain,
        "Dump eventlog."
    );

    try {
        return modChooser.Run(argc, argv);
    } catch (...) {
        Cerr << FormatCurrentException() << Endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

} // namespace NMatrix
