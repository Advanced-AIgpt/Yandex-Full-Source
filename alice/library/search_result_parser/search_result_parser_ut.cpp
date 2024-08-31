#include "search_result_parser.h"

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/file.h>

#include <google/protobuf/text_format.h>

namespace NAlice {

namespace {

TString FullPath(TStringBuf file) {
    return SRC_(TString::Join("ut/", file, ".pb.txt"));
}

NScenarios::TWebSearchDocs LoadSearchDoc(const TString& name) {
    NScenarios::TWebSearchDocs req;
    const auto in = TFileInput{FullPath(name)}.ReadAll();
    UNIT_ASSERT_C(NProtoBuf::TextFormat::ParseFromString(in, &req), "pb.txt file parse failed");
    return req;
}

} // anonymous namespace

Y_UNIT_TEST_SUITE(ProtoEval) {
    Y_UNIT_TEST(QueryAvatar) {
        NScenarios::TDataSource dataSrc;
        *dataSrc.MutableWebSearchDocs() = LoadSearchDoc("tolyatti_docs");

        TSearchResultParser se(TRTLogger::NullLogger());
        UNIT_ASSERT(se.AttachDataSource(&dataSrc));

        auto snippet =se.FindSnippetByType({TSearchResultParser::EUseDatasource::Docs}, "entity_search");
        auto ret = se.GetMainAvatar(snippet);
        UNIT_ASSERT(ret);
        UNIT_ASSERT_STRINGS_EQUAL(ret->GetUrlAvatar().GetUrl(),
            "https://avatars.mds.yandex.net/get-entity_search/2340476/I18pzR5KOSub/S120x120");
    } // Y_UNIT_TEST(QueryAvatar)

    Y_UNIT_TEST(QueryGallery) {
        NScenarios::TDataSource dataSrc;
        *dataSrc.MutableWebSearchDocs() = LoadSearchDoc("tolyatti_docs");

        TSearchResultParser se(TRTLogger::NullLogger());
        UNIT_ASSERT(se.AttachDataSource(&dataSrc));

        auto snippet =se.FindSnippetByType({TSearchResultParser::EUseDatasource::Docs}, "entity_search");
        const auto ret = se.GetImagesGallery(snippet);
        UNIT_ASSERT_EQUAL(ret->GetImages().size(), 15);
        UNIT_ASSERT_STRINGS_EQUAL(ret->GetImages()[0].GetUrlAvatar().GetUrl(),
            "https://avatars.mds.yandex.net/i?id=251528047d1a96c4f669b278d66b138e-5887670-images-thumbs&ref=oo_serp");

    } // Y_UNIT_TEST(QueryGallery)

    Y_UNIT_TEST(QueryPersons) {
        NScenarios::TDataSource dataSrc;
        *dataSrc.MutableWebSearchDocs() = LoadSearchDoc("tolyatti_docs");

        TSearchResultParser se(TRTLogger::NullLogger());
        UNIT_ASSERT(se.AttachDataSource(&dataSrc));

        auto snippet =se.FindSnippetByType({TSearchResultParser::EUseDatasource::Docs}, "entity_search");
        const auto ret = se.GetRelatedPersons(snippet);
        UNIT_ASSERT_EQUAL(ret->GetPersones().size(), 0);
    } // Y_UNIT_TEST(QueryPersons)

    Y_UNIT_TEST(QueryRequest) {
        NScenarios::TDataSource dataSrc;
        *dataSrc.MutableWebSearchDocs() = LoadSearchDoc("tolyatti_docs");

        TSearchResultParser se(TRTLogger::NullLogger());
        UNIT_ASSERT(se.AttachDataSource(&dataSrc));

        TVector<TSearchResultParser::TEntryDescr> descr = se.CollectSnippets({TSearchResultParser::EUseDatasource::DocsRight});
        UNIT_ASSERT(descr.size() == 0);
        descr = se.CollectSnippets({TSearchResultParser::EUseDatasource::Docs});
        UNIT_ASSERT_EQUAL(descr.size(), 13);

    }
}

} // namespace NAlice
