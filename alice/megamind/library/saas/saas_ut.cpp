#include "saas.h"

#include <library/cpp/testing/unittest/registar.h>

#include <alice/library/unittest/fake_fetcher.h>

#include <search/idl/meta.pb.h>

using namespace NAlice::NSaasSearch;

namespace {

NAlice::TConfig::TSaasSourceOptions CreateSaasSourceConfig(int threshold = 0) {
    NAlice::TConfig::TSaasSourceOptions options;
    options.SetUnistatPrefix("recommender");
    options.SetServiceName("ServiceName");
    options.SetWordsCountMinInRequest(0);
    auto& params = *options.MutableSaasQueryParams();
    params.SetFormula("Fromula");
    params.SetKps("Kps");
    params.SetThreshold(threshold);
    params.SetSoftness("0");

    options.SetHost("127.0.0.0");
    options.SetPort(80);
    options.SetTimeoutMs(100);
    return options;
}

Y_UNIT_TEST_SUITE(Saas) {
    Y_UNIT_TEST(PrepareSaasRequest) {
        const NAlice::TConfig::TSaasSourceOptions emptyConfig;
        /* prepare with empty utterance */ {
            const TString utterance = "";
            NAlice::NTestingHelpers::TFakeRequestBuilder requestBuilder{};
            auto prepareStatus = PrepareSaasRequest("", emptyConfig, requestBuilder);
            UNIT_ASSERT(!prepareStatus.IsSuccess());
            UNIT_ASSERT(prepareStatus.Error()->ErrorMsg == "Empty utterance to call Saas");
        }
        /* correct prepare */ {
            NAlice::NTestingHelpers::TFakeRequestBuilder requestBuilder{};
            auto prepareStatus = PrepareSaasRequest("utterance", CreateSaasSourceConfig(), requestBuilder);
            UNIT_ASSERT(prepareStatus.IsSuccess());
            UNIT_ASSERT(requestBuilder.Cgi.Print() == "g=0..10.1.-1.0.0.-1.rlv.0..0.0&how=rlv&kps=Kps&ms=proto&numdoc=10"
                                                      "&relev=formula%3DFromula&relev=attr_limit%3D999999999"
                                                      "&service=ServiceName&text=utterance+softness%3A0&timeout=100000");
        }

    }
    Y_UNIT_TEST(ParseSaasReply) {
        /* test simple report */ {
            NMetaProtocol::TReport report;
            auto& grouping = *report.AddGrouping();
            for (int i = 0; i < 5; ++i) {
                auto& group = *grouping.AddGroup();
                auto& document = *group.AddDocument();
                document.SetRelevance(i);
                document.MutableArchiveInfo()->SetUrl(ToString(i));
            }
            TString content;
            UNIT_ASSERT(report.SerializeToString(&content));
            auto result = ParseSaasSkillDiscoveryReply(content, CreateSaasSourceConfig(2));
            UNIT_ASSERT_VALUES_EQUAL(result.SaasCandidateSize(), 2);
            UNIT_ASSERT_VALUES_EQUAL(result.GetSaasCandidate(0).GetSkillId(), "4");
            UNIT_ASSERT_VALUES_EQUAL(result.GetSaasCandidate(1).GetSkillId(), "3");
        }
        /* test empty report */ {
            NMetaProtocol::TReport report;
            TString content;
            UNIT_ASSERT(report.SerializeToString(&content));
            auto result = ParseSaasSkillDiscoveryReply(content, CreateSaasSourceConfig());
            UNIT_ASSERT_VALUES_EQUAL(result.SaasCandidateSize(), 0);
        }
        /* test remove all by threshold */ {
            NMetaProtocol::TReport report;
            auto& grouping = *report.AddGrouping();
            for (int i = 0; i < 5; ++i) {
                auto& group = *grouping.AddGroup();
                auto& document = *group.AddDocument();
                document.SetRelevance(i);
                document.MutableArchiveInfo()->SetUrl(ToString(i));
            }
            TString content;
            UNIT_ASSERT(report.SerializeToString(&content));
            auto result = ParseSaasSkillDiscoveryReply(content, CreateSaasSourceConfig(20));
            UNIT_ASSERT_VALUES_EQUAL(result.SaasCandidateSize(), 0);
        }
    }
}

} // namespace
