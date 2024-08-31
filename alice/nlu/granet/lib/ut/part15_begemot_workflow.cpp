#include <alice/nlu/granet/lib/granet.h>
#include <alice/nlu/granet/lib/parser/multi_parser.h>
#include <alice/nlu/libs/ut_utils/ut_utils.h>
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <library/cpp/iterator/zip.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetPart15_Experiments) {

    TGrammar::TConstRef CompileTestGrammar(TStringBuf grammarStr) {
        return CompileGrammarFromString(NAlice::NUtUtils::NormalizeText(grammarStr), {});
    }

    TGrammar::TConstRef CompileTestGrammarOptional(TStringBuf grammarStr) {
        return !grammarStr.empty() ? CompileTestGrammar(grammarStr) : nullptr;
    }

    TVector<TGrammar::TConstRef> CompileTestGrammarsOptional(TStringBuf grammarStr) {
        if (grammarStr.empty()) {
            return {};
        }
        return {CompileTestGrammar(grammarStr)};
    }

    THashSet<TParserTaskKey> MakeTaskSet(TStringBuf forms) {
        THashSet<TParserTaskKey> result;
        for (const TString& name : SplitAndStrip(forms, ',')) {
            result.insert(TParserTaskKey{PTT_FORM, name});
        }
        return result;
    }

    TMultiGrammar::TConstRef Compile(
        TStringBuf staticGrammar,
        TStringBuf freshGrammar,
        TStringBuf externalGrammar,
        const TFreshForcingOptions& freshOptions,
        TStringBuf experiments)
    {
        return TMultiGrammar::CreateForBegemot(
            CompileTestGrammarOptional(staticGrammar),
            CompileTestGrammarOptional(freshGrammar),
            CompileTestGrammarsOptional(externalGrammar),
            freshOptions,
            StringSplitter(experiments).Split(','),
            MakeTaskSet(""));
    }

    void Test(const TMultiGrammar::TConstRef& grammar, TStringBuf request, TStringBuf expectedResponse) {
        TSample::TRef sample = TSample::Create(request, LANG_RUS);
        TMultiParser::TRef parser = TMultiParser::Create(grammar, sample, true);

        TSet<TString> forms;
        for (const TParserFormResult::TConstRef& form : parser->ParseForms()) {
            if (form->IsPositive()) {
                forms.insert(form->GetName());
            }
        }
        const TString actualResponse = JoinSeq(",", forms);

        UNIT_ASSERT_C(actualResponse == expectedResponse, request);
    }

    void Test(const TMultiGrammar::TConstRef& grammar, const TMap<TStringBuf, TStringBuf>& requestToResponse) {
        for (const auto& [request, response] : requestToResponse) {
            Test(grammar, request, response);
        }
    }

    void TestExperiments(
        TStringBuf staticGrammar,
        TStringBuf freshGrammar,
        TStringBuf externalGrammar,
        const TFreshForcingOptions& freshOptions,
        const TVector<TStringBuf>& setsOfExperiments,
        const TMap<TStringBuf, TVector<TStringBuf>>& requestToResponses)
    {
        for (const auto& [request, responses] : requestToResponses) {
            Y_ENSURE(responses.size() == setsOfExperiments.size());
            for (const auto& [response, experiments] : Zip(responses, setsOfExperiments)) {
                TMultiGrammar::TConstRef grammar = Compile(staticGrammar, freshGrammar, externalGrammar,
                    freshOptions, experiments);
                Test(grammar, request, response);
            }
        }
    }

    const TStringBuf STATIC = R"(
        form fa:    root: a0
        form fb:    root: b0
        form fpa:   root: pa0
        form fpb:   root: pb0
    )";

    const TStringBuf FRESH = R"(
        form fb:    root: b1
        form fc:    root: c1
        form fpb:   root: pb1
        form fpc:   root: pc1
    )";

    const TStringBuf EXTERNAL = R"(
        form fa:    root: a2
        form fc:    root: c2
    )";

    Y_UNIT_TEST(Empty) {
        Test(Compile("", "", "", {}, ""), {
            {"a0", ""  }, {"a1", ""  }, {"a2", ""  },
            {"b0", ""  }, {"b1", ""  }, {"b2", ""  },
            {"c0", ""  }, {"c1", ""  }, {"c2", ""  },
        });
    }

    Y_UNIT_TEST(Static) {
        Test(Compile(STATIC, "", "", {}, ""), {
            {"a0", "fa"}, {"a1", ""  }, {"a2", ""  },
            {"b0", "fb"}, {"b1", ""  }, {"b2", ""  },
            {"c0", ""  }, {"c1", ""  }, {"c2", ""  },
        });
        Test(Compile(STATIC, "", "", {.ForceForForms = {"fa", "fb", "fc"}}, ""), {
            {"a0", "fa"}, {"a1", ""  }, {"a2", ""  },
            {"b0", "fb"}, {"b1", ""  }, {"b2", ""  },
            {"c0", ""  }, {"c1", ""  }, {"c2", ""  },
        });
        Test(Compile(STATIC, "", "", {.ForceEntireFresh = true}, ""), {
            {"a0", "fa"}, {"a1", ""  }, {"a2", ""  },
            {"b0", "fb"}, {"b1", ""  }, {"b2", ""  },
            {"c0", ""  }, {"c1", ""  }, {"c2", ""  },
        });
    }

    Y_UNIT_TEST(Fresh) {
        Test(Compile(STATIC, FRESH, "", {}, ""), {
            {"a0",  "fa" }, {"a1",  ""   }, {"a2",  ""   },
            {"b0",  "fb" }, {"b1",  ""   }, {"b2",  ""   },
            {"c0",  ""   }, {"c1",  ""   }, {"c2",  ""   },
            {"pa0", "fpa"}, {"pa1", ""   }, {"pa2", ""   },
            {"pb0", "fpb"}, {"pb1", ""   }, {"pb2", ""   },
            {"pc0", ""   }, {"pc1", ""   }, {"pc2", ""   },
        });
        Test(Compile(STATIC, FRESH, "", {.ForceForForms = {"fb"}}, ""), {
            {"a0",  "fa" }, {"a1",  ""   }, {"a2",  ""   },
            {"b0",  ""   }, {"b1",  "fb" }, {"b2",  ""   },
            {"c0",  ""   }, {"c1",  ""   }, {"c2",  ""   },
            {"pa0", "fpa"}, {"pa1", ""   }, {"pa2", ""   },
            {"pb0", "fpb"}, {"pb1", ""   }, {"pb2", ""   },
            {"pc0", ""   }, {"pc1", ""   }, {"pc2", ""   },
        });
        Test(Compile(STATIC, FRESH, "", {.ForceForForms = {"fa", "fb", "fc"}}, ""), {
            {"a0",  ""   }, {"a1",  ""   }, {"a2",  ""   },
            {"b0",  ""   }, {"b1",  "fb" }, {"b2",  ""   },
            {"c0",  ""   }, {"c1",  "fc" }, {"c2",  ""   },
            {"pa0", "fpa"}, {"pa1", ""   }, {"pa2", ""   },
            {"pb0", "fpb"}, {"pb1", ""   }, {"pb2", ""   },
            {"pc0", ""   }, {"pc1", ""   }, {"pc2", ""   },
        });
        Test(Compile(STATIC, FRESH, "", {.ForceForPrefixes = {"fp"}}, ""), {
            {"a0",  "fa" }, {"a1",  ""   }, {"a2",  ""   },
            {"b0",  "fb" }, {"b1",  ""   }, {"b2",  ""   },
            {"c0",  ""   }, {"c1",  ""   }, {"c2",  ""   },
            {"pa0", ""   }, {"pa1", ""   }, {"pa2", ""   },
            {"pb0", ""   }, {"pb1", "fpb"}, {"pb2", ""   },
            {"pc0", ""   }, {"pc1", "fpc"}, {"pc2", ""   },
        });
        Test(Compile(STATIC, FRESH, "", {.ForceEntireFresh = true}, ""), {
            {"a0",  ""   }, {"a1",  ""   }, {"a2",  ""   },
            {"b0",  ""   }, {"b1",  "fb" }, {"b2",  ""   },
            {"c0",  ""   }, {"c1",  "fc" }, {"c2",  ""   },
            {"pa0", ""   }, {"pa1", ""   }, {"pa2", ""   },
            {"pb0", ""   }, {"pb1", "fpb"}, {"pb2", ""   },
            {"pc0", ""   }, {"pc1", "fpc"}, {"pc2", ""   },
        });
    }

    Y_UNIT_TEST(External) {
        Test(Compile(STATIC, "", EXTERNAL, {}, ""), {
            {"a0", ""  }, {"a1", ""  }, {"a2", "fa"},
            {"b0", "fb"}, {"b1", ""  }, {"b2", ""  },
            {"c0", ""  }, {"c1", ""  }, {"c2", "fc"},
        });
        Test(Compile(STATIC, "", EXTERNAL, {.ForceForForms = {"fa", "fb", "fc"}}, ""), {
            {"a0", ""  }, {"a1", ""  }, {"a2", "fa"},
            {"b0", "fb"}, {"b1", ""  }, {"b2", ""  },
            {"c0", ""  }, {"c1", ""  }, {"c2", "fc"},
        });
        Test(Compile(STATIC, "", EXTERNAL, {.ForceEntireFresh = true}, ""), {
            {"a0", ""  }, {"a1", ""  }, {"a2", "fa"},
            {"b0", "fb"}, {"b1", ""  }, {"b2", ""  },
            {"c0", ""  }, {"c1", ""  }, {"c2", "fc"},
        });
    }

    Y_UNIT_TEST(FreshWithExternal) {
        Test(Compile(STATIC, FRESH, EXTERNAL, {}, ""), {
            {"a0", ""  }, {"a1", ""  }, {"a2", "fa"},
            {"b0", "fb"}, {"b1", ""  }, {"b2", ""  },
            {"c0", ""  }, {"c1", ""  }, {"c2", "fc"},
        });
        Test(Compile(STATIC, FRESH, EXTERNAL, {.ForceForForms = {"fb"}}, ""), {
            {"a0", ""  }, {"a1", ""  }, {"a2", "fa"},
            {"b0", ""  }, {"b1", "fb"}, {"b2", ""  },
            {"c0", ""  }, {"c1", ""  }, {"c2", "fc"},
        });
        Test(Compile(STATIC, FRESH, EXTERNAL, {.ForceForForms = {"fa", "fb", "fc"}}, ""), {
            {"a0", ""  }, {"a1", ""  }, {"a2", "fa"},
            {"b0", ""  }, {"b1", "fb"}, {"b2", ""  },
            {"c0", ""  }, {"c1", ""  }, {"c2", "fc"},
        });
        Test(Compile(STATIC, FRESH, EXTERNAL, {.ForceEntireFresh = true}, ""), {
            {"a0", ""  }, {"a1", ""  }, {"a2", "fa"},
            {"b0", ""  }, {"b1", "fb"}, {"b2", ""  },
            {"c0", ""  }, {"c1", ""  }, {"c2", "fc"},
        });
    }

    const TStringBuf EXP_STATIC = R"(
        form fa:            root: a0
        form fa.ifexp.e1:   root: a1
        form fa.ifexp.e2:   root: a2
        form fb:            root: b0
    )";

    const TStringBuf EXP_FRESH = R"(
        form fa:            root: a0
        form fa.ifexp.e1:   root: a1
        form fa.ifexp.e2:   root: a2
        form fa.ifexp.e3:   root: a3
        form fb:            root: b0
        form fb.ifexp.e1:   root: b1
    )";

    Y_UNIT_TEST(ExperimentsSimple) {
        Test(Compile(EXP_STATIC, EXP_FRESH, "", {}, ""), {
            {"a0", "fa"}, {"a1", ""  }, {"a2", ""  }, {"a3", ""  },
            {"b0", "fb"}, {"b1", ""  }, {"b2", ""  }, {"b3", ""  },
        });
        Test(Compile(EXP_STATIC, EXP_FRESH, "", {}, "e1"), {
            {"a0", ""  }, {"a1", "fa"}, {"a2", ""  }, {"a3", ""  },
            {"b0", "fb"}, {"b1", ""  }, {"b2", ""  }, {"b3", ""  },
        });
        Test(Compile(EXP_STATIC, EXP_FRESH, "", {.ForceForExperiments = {"e1"}}, "e1"), {
            {"a0", ""  }, {"a1", "fa"}, {"a2", ""  }, {"a3", ""  },
            {"b0", ""  }, {"b1", "fb"}, {"b2", ""  }, {"b3", ""  },
        });
    }

    Y_UNIT_TEST(ExperimentTableEmpty) {
        TestExperiments("", "", "", {},
            {            "",   "e1", "e2", "e3", "e1,e3"},
            {
                {"a0",  {"",   "",   "",   "",   "",    }},
                {"a1",  {"",   "",   "",   "",   "",    }},
                {"a2",  {"",   "",   "",   "",   "",    }},
                {"a3",  {"",   "",   "",   "",   "",    }},
                {"ae",  {"",   "",   "",   "",   "",    }},
                {"b0",  {"",   "",   "",   "",   "",    }},
                {"b1",  {"",   "",   "",   "",   "",    }},
            }
        );
    }

    Y_UNIT_TEST(ExperimentTableStatic) {
        TestExperiments(EXP_STATIC, EXP_FRESH, "", {},
            {            "",   "e1", "e2", "e3", "e1,e3"},
            {
                {"a0",  {"fa", "",   "",   "fa", "",    }},
                {"a1",  {"",   "fa", "",   "",   "fa",  }},
                {"a2",  {"",   "",   "fa", "",   "",    }},
                {"a3",  {"",   "",   "",   "",   "",    }},
                {"ae",  {"",   "",   "",   "",   "",    }},
                {"b0",  {"fb", "fb", "fb", "fb", "fb",  }},
                {"b1",  {"",   "",   "",   "",   "",    }},
            }
        );
    }

    Y_UNIT_TEST(ExperimentTableFresh) {
        TestExperiments(EXP_STATIC, EXP_FRESH, "", {.ForceForForms = {"fa.ifexp.e3", "fb.ifexp.e1"}},
            {            "",   "e1", "e2", "e3", "e1,e3"},
            {
                {"a0",  {"fa", "",   "",   "",   "",    }},
                {"a1",  {"",   "fa", "",   "",   "",    }},
                {"a2",  {"",   "",   "fa", "",   "",    }},
                {"a3",  {"",   "",   "",   "fa", "fa",  }},
                {"ae",  {"",   "",   "",   "",   "",    }},
                {"b0",  {"fb", "",   "fb", "fb", "",    }},
                {"b1",  {"",   "fb", "",   "",   "fb",  }},
            }
        );
    }
}
