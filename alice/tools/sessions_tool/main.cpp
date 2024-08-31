#include "parse_scenario_session.h"

#include <alice/megamind/library/session/session.h>

#include <alice/library/json/json.h>
#include <alice/library/proto/proto.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>
#include <library/cpp/json/json_writer.h>

using namespace NAlice;

int RunDeserialize(int /*argc*/, const char** /*argv*/) {
    TString serialized = Cin.ReadLine();
    const NJson::TJsonValue session = ParseRawJsonSession(serialized);

    Cout << NJson::WriteJson(session) << Endl;

    return 0;
}

int RunDeserializeMegamind(int /*argc*/, const char** /*argv*/) {
    TString serialized = Cin.ReadLine();
    const auto session = DeserializeSession(serialized);

    Cout << SerializeProtoText(session->Proto(), /* singleLineMode= */ false) << Endl;

    return 0;
}

int RunDeserializeMegamindMessage(int argc, const char** argv) {
    NLastGetopt::TOpts opts;
    TString scenarioName;
    bool singleLineMode;
    opts.AddLongOption('s', "scenario-name")
        .Help("Scenario name wich state to parse")
        .Optional()
        .StoreResult(&scenarioName);
    opts.AddLongOption("single-line-mode")
        .Help("Print proto in single line")
        .Optional()
        .SetFlag(&singleLineMode);

    opts.SetFreeArgsNum(0, 0);
    NLastGetopt::TOptsParseResult optsParseResult(&opts, argc, argv);
    TString message = Cin.ReadAll();

    auto json = JsonFromString(message);
    auto jsonMessage = JsonFromString(json["Message"].GetString());
    const auto session = DeserializeSession(jsonMessage["session"].GetString());

    if (scenarioName) {
        Cout << GetScenarioSessionString(session->Proto(), scenarioName, singleLineMode) << Endl;
    } else {
        Cout << NAlice::SerializeProtoText(session->Proto(), singleLineMode) << Endl;
    }

    return 0;
}

int main(int argc, const char** argv) {
    TModChooser modChooser;

    modChooser.AddMode("debug-print-json",
                       RunDeserialize,
                       "Deserialize VINS session from stdin and write json to stdout");
    modChooser.AddMode("debug-print-megamind-proto",
                       RunDeserializeMegamind,
                       "Deserialize Megamind session from stdin and write proto text to stdout");
    modChooser.AddMode("debug-print-megamind-message-proto",
                       RunDeserializeMegamindMessage,
                       "Deserialize session from Megamind request picked from eventlog message and passed to the tool from stdin and write proto text to stdoutut");

    try {
        return modChooser.Run(argc, argv);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
