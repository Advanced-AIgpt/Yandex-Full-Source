#include <alice/bass/libs/config/config.sc.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/metrics/place.h>
#include <alice/bass/libs/source_request/source_request.h>

#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/getopt/small/modchooser.h>
#include <library/cpp/neh/http_common.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/scheme/domscheme_traits.h>

#include <util/stream/file.h>
#include <util/string/subst.h>

using TBASSConfig = NBASSConfig::TConfig<TSchemeTraits>;

NSc::TValue GenerateAliceSourcesPanel() {
    constexpr TStringBuf base = TStringBuf(R"""(
{
    "title": "Alice sources",
    "type": "panel",
    "editors": [
        "shuster",
        "av-kotikov",
        "sgalustyan",
        "petrk",
        "leletko",
        "atsepeleva",
        "vi002",
        "yulika",
        "akhruslan"
    ],
    "charts": [
        {
            "id": "total",
            "type": "text",
            "width": 1,
            "height": 3,
            "row": 1,
            "col": 1,
            "text": "Total"
        },
        {
            "id": "unanswers_man",
            "type": "graphic",
            "width": 4,
            "height": 1,
            "row": 1,
            "col": 2,
            "title": "Unanswers, man",
            "stacked": false,
            "normalize": true,
            "yAxis": [
                {}
            ],
            "signals": [
                {
                    "title": "man",
                    "color": "#fa0000",
                    "tag": "itype=bass;ctype=prod,priemka;geo=man;prj=personal-cards",
                    "host": "ASEARCH",
                    "name": "unistat-source_*_requests_failure_*_dmmm"
                }
            ],
            "consts": []
        },
        {
            "id": "unanswers_sas",
            "type": "graphic",
            "width": 4,
            "height": 1,
            "row": 2,
            "col": 2,
            "title": "Unanswers, sas",
            "stacked": false,
            "normalize": true,
            "yAxis": [
                {}
            ],
            "signals": [
                {
                    "title": "sas",
                    "color": "#fa0000",
                    "tag": "itype=bass;ctype=prod,priemka;geo=sas;prj=personal-cards",
                    "host": "ASEARCH",
                    "name": "unistat-source_*_requests_failure_*_dmmm"
                }
            ],
            "consts": []
        },
        {
            "id": "unanswers_vla",
            "type": "graphic",
            "width": 4,
            "height": 1,
            "row": 3,
            "col": 2,
            "title": "Unanswers, vla",
            "stacked": false,
            "normalize": true,
            "yAxis": [
                {}
            ],
            "signals": [
                {
                    "title": "vla",
                    "color": "#fa0000",
                    "tag": "itype=bass;ctype=prod,priemka;geo=vla;prj=personal-cards",
                    "host": "ASEARCH",
                    "name": "unistat-source_*_requests_failure_*_dmmm"
                }
            ],
            "consts": []
        }
    ],
    "description": "Alice sources main panel"
}
)""");
    return NSc::TValue::FromJson(base);
}

const TString TEXT_BLOCK = R"""(
    {
        "id": "{{source}}",
        "type": "text",
        "width": 1,
        "height": 1,
        "row": {{row}},
        "col": 1,
        "text": "{{source}}"
    }
)""";
NSc::TValue GenerateTextBlock(TStringBuf source, ui32 row) {
    TString chart = TEXT_BLOCK;
    SubstGlobal(chart, "{{source}}", source);
    SubstGlobal(chart, "{{row}}", ToString(row));
    return NSc::TValue::FromJson(chart);
}

const TString UNANSWERS_CHART = R"""(
{
    "id": "unanswers_{{source}}",
    "type": "graphic",
    "width": 2,
    "height": 1,
    "row": {{row}},
    "col": 2,
    "title": "Unanswers %: man(red), sas(green), vla(blue) ",
    "stacked": false,
    "normalize": true,
    "yAxis": [
        {}
    ],
    "signals": [
        {
            "title": "man",
            "color": "#fa0000",
            "tag": "itype=bass;ctype=prod,priemka;geo=man;prj=personal-cards",
            "host": "ASEARCH",
            "name": "or(perc(unistat-source_{{source}}_requests_failure_dmmm, sum(unistat-source_{{source}}_requests_failure_dmmm, unistat-source_{{source}}_requests_success_dmmm)), 0)"
        },
        {
            "title": "vla",
            "color": "#37bff2",
            "tag": "itype=bass;ctype=prod,priemka;geo=vla;prj=personal-cards",
            "host": "ASEARCH",
            "name": "or(perc(unistat-source_{{source}}_requests_failure_dmmm, sum(unistat-source_{{source}}_requests_failure_dmmm, unistat-source_{{source}}_requests_success_dmmm)), 0)"
        },
        {
            "title": "sas",
            "color": "#169833",
            "tag": "itype=bass;ctype=prod,priemka;geo=sas;prj=personal-cards",
            "host": "ASEARCH",
            "name": "or(perc(unistat-source_{{source}}_requests_failure_dmmm, sum(unistat-source_{{source}}_requests_failure_dmmm, unistat-source_{{source}}_requests_success_dmmm)), 0)"
        }
    ],
    "consts": []
}
)""";

NSc::TValue GenerateUnanswersChart(TStringBuf source, ui32 row) {
    TString chart = UNANSWERS_CHART;
    SubstGlobal(chart, "{{source}}", source);
    SubstGlobal(chart, "{{row}}", ToString(row));
    return NSc::TValue::FromJson(chart);
}

const TString TIMINGS_CHART = R"""(
{
    "id": "timings_{{source}}",
    "type": "graphic",
    "width": 2,
    "height": 1,
    "row": {{row}},
    "col": 3,
    "title": "Timings",
    "signals": [
        {
            "color": "#37bff2",
            "tag": "itype=bass;ctype=prod;geo=man,sas,vla;prj=personal-cards",
            "host": "ASEARCH",
            "name": "quant(unistat-source_{{source}}_responseTime_hgram,50)"
        },
        {
            "color": "#169833",
            "tag": "itype=bass;ctype=prod;geo=man,sas,vla;prj=personal-cards",
            "host": "ASEARCH",
            "name": "quant(unistat-source_{{source}}_responseTime_hgram,75)"
        },
        {
            "color": "#f6ab31",
            "tag": "itype=bass;ctype=prod;geo=man,sas,vla;prj=personal-cards",
            "host": "ASEARCH",
            "name": "quant(unistat-source_{{source}}_responseTime_hgram,95)"
        },
        {
            "color": "#c95edd",
            "tag": "itype=bass;ctype=prod;geo=man,sas,vla;prj=personal-cards",
            "host": "ASEARCH",
            "name": "quant(unistat-source_{{source}}_responseTime_hgram,99)"
        }
    ],
    "consts": [
        {
            "title": "{{source}}_timeout",
            "value": {{timeout}},
            "color": "#7b917b",
            "width": 4
        },
        {
            "title": "{{source}}_SLA",
            "value": {{SLA}},
            "color": "#735184",
            "width": 4
        }
    ]
}
)""";

NSc::TValue GenerateTimingsChart(TStringBuf source, const NSc::TValue& config, ui32 row) {
    TStringBuf timeout;
    TStringBuf sla;
    if (source == "SupProvider" || source == "XivaProvider") {
        timeout = config["PushHandler"][source]["Source"]["Timeout"].GetString();
        sla = config["PushHandler"][source]["Source"]["SLATime"].GetString();
    } else {
        timeout = config["Vins"][source]["Timeout"].GetString();
        sla = config["Vins"][source]["SLATime"].GetString();
    }

    TString chart = TIMINGS_CHART;
    SubstGlobal(chart, "{{source}}", source);
    SubstGlobal(chart, "{{row}}", ToString(row));
    SubstGlobal(chart, "{{timeout}}", ToString(TDuration::Parse(timeout).MilliSeconds()));
    SubstGlobal(chart, "{{SLA}}", ToString(TDuration::Parse(sla).MilliSeconds()));
    return NSc::TValue::FromJson(chart);
}

class TPanel {
public:
    TPanel()
        : Json(GenerateAliceSourcesPanel())
        , Row(4)
    {
    }

    void AddSource(TStringBuf source, const NSc::TValue& config) {
        Json["charts"].GetArrayMutable().AppendAll(GenerateSourceRow(source, config, Row).GetArrayMutable());
    }

    const NSc::TValue& GetJson() const {
        return Json;
    }

private:
    NSc::TValue GenerateSourceRow(TStringBuf source, const NSc::TValue& config, ui32& row) {
        NSc::TValue v;
        v.Push(GenerateTextBlock(source, row));
        v.Push(GenerateUnanswersChart(source, row));
        v.Push(GenerateTimingsChart(source, config, row++));
        return v;
    }

private:
    NSc::TValue Json;
    ui32 Row;
};

struct TFakeCounters final: NBASS::NMetrics::ICountersPlace {
    TFakeCounters()
        : FakeBassCounters()
    {
    }

    NMonitoring::TMetricRegistry& Sensors() override {
        return FakeSensors;
    }

    NMonitoring::TMetricRegistry& SkillSensors() override {
        return FakeSensors;
    }

    NMonitoring::TBassCounters& BassCounters() override {
        return FakeBassCounters;
    }

    const NBASS::NMetrics::TSignals& Signals() const override {
        return FakeSignals;
    }

    void Register(TMonService& /*service*/) override {
        return;
    }

private:
    NMonitoring::TMetricRegistry FakeSensors;
    NMonitoring::TBassCounters FakeBassCounters;
    NBASS::NMetrics::TSignals FakeSignals;

};

NSc::TValue GeneratePanel(const TString& configFile) {
    TFakeCounters counters{};

    TDummySourcesRegistryDelegate delegate;
    TSourcesRegistry sourceRegistry(counters, delegate);
    auto sources = sourceRegistry.GetRegistryList();
    NSc::TValue config = NSc::TValue::FromJsonThrow(TFileInput(configFile).ReadAll());

    TPanel panel;
    for (auto& source : sources) {
        panel.AddSource(source, config);
    }
    return panel.GetJson();
}

int Print(int argc, const char** argv) {
    NLastGetopt::TOpts options;

    options.SetFreeArgsNum(1);
    options.SetFreeArgTitle(0, "<config.json>", "configuration file in json");
    NLastGetopt::TOptsParseResult optParsing(&options, argc, argv);

    Cout << GeneratePanel(optParsing.GetFreeArgs()[0]).ToJsonPretty() << Endl;
    return 0;
}

int Commit(int argc, const char** argv) {
    NLastGetopt::TOpts options;

    TString user;
    options
        .AddLongOption("user", "Panel's owner")
        .Optional()
        .DefaultValue("osado")
        .StoreResult(&user);
    TString key;
    options
        .AddLongOption("key", "Panel's alias")
        .Optional()
        .DefaultValue("alice_sources")
        .StoreResult(&key);
    bool debug = false;
    options
        .AddCharOption('d', "Debug mode")
        .AddLongName("debug")
        .NoArgument()
        .SetFlag(&debug);
    options.SetFreeArgsNum(1);
    options.SetFreeArgTitle(0, "<config.json>", "configuration file in json");
    NLastGetopt::TOptsParseResult optParsing(&options, argc, argv);

    NNeh::TMessage msg = NNeh::TMessage::FromString("http://yasm.yandex-team.ru/srvambry/upsert");
    TString keys = "{\"user\" : \"" + user + "\", \"key\" : \"" + key + "\"}";
    TString result = GeneratePanel(optParsing.GetFreeArgs()[0]).ToJsonPretty();

    if (!NNeh::NHttp::MakeFullRequest(msg, "", "{\"keys\" : " + keys + ", \"values\" : " + result + "}")) {
        ythrow yexception() << "Could not create a request.\nUrl: " << msg.Addr << "\nKeys: " << keys + "}}";
    }

    auto response = NNeh::Request(msg)->Wait();

    if (debug) {
        Cout << response->Data << Endl;
    }

    return 0;
}

int main(int argc, const char** argv) {
    TModChooser modChooser;

    modChooser.AddMode("Print", Print, "Prints config to Cout");
    modChooser.AddMode("Commit", Commit, "Commits config to https://yasm.yandex-team.ru/panel/<panel's name>");

    return modChooser.Run(argc, argv);
}
