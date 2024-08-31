#include <alice/nlu/libs/embedder/embedder.h>

#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

using namespace NAlice;

namespace {
    TTokenEmbedder CreateTestEmbedder(const TMap<TString, TEmbedding>& embeddings, const size_t dimension) {
        TCompactTrieBuilder<char, TTokenIndex> trieBuilder(CTBF_PREFIX_GROUPED | CTBF_UNIQUE);
        TEmbedding embeddingMatrix(Reserve(dimension * embeddings.size()));

        for (const auto& [word, vector] : embeddings) {
            trieBuilder.Add(word, trieBuilder.GetEntryCount());
            embeddingMatrix.insert(embeddingMatrix.end(), vector.begin(), vector.end());
        }

        TBufferOutput buffer;
        CompactTrieMakeFastLayout(buffer, trieBuilder);

        return TTokenEmbedder(
            TBlob::Copy(embeddingMatrix.data(), embeddingMatrix.size() * sizeof(TEmbedding::value_type)),
            TBlob::FromBuffer(buffer.Buffer()),
            dimension
        );
    }
} // namespace


Y_UNIT_TEST_SUITE(TTokenEmbedderTestSuite) {
    Y_UNIT_TEST(InVocabWords) {
        const TEmbedding helloEmbedding{1.0f, 2.0f};
        const TEmbedding worldEmbedding{0.0f, 1.0f};

        const TTokenEmbedder embedder = CreateTestEmbedder(
            {
                {"hello", helloEmbedding},
                {"world", worldEmbedding}
            },
            2
        );

        const TEmbedding defaultEmbed{-7.0f, 9.0f};

        UNIT_ASSERT_EQUAL(embedder.EmbedToken("hello"), helloEmbedding);
        UNIT_ASSERT_EQUAL(embedder.EmbedToken("hello", defaultEmbed), helloEmbedding);

        UNIT_ASSERT_EQUAL(embedder.EmbedToken("world"), worldEmbedding);
        UNIT_ASSERT_EQUAL(embedder.EmbedToken("world", defaultEmbed), worldEmbedding);
    }

    Y_UNIT_TEST(OufOfVocabWords) {
        const TEmbedding helloEmbedding{1.0f, 2.0f};
        const TEmbedding worldEmbedding{0.0f, 1.0f};

        const TTokenEmbedder embedder = CreateTestEmbedder(
            {
                {"hello", helloEmbedding},
                {"world", worldEmbedding}
            },
            2
        );

        const TEmbedding defaultEmbed{-7.0f, 9.0f};
        const TEmbedding zeroEmbed{0.0f, 0.0f};

        UNIT_ASSERT_EQUAL(embedder.EmbedToken("OOV"), zeroEmbed);
        UNIT_ASSERT_EQUAL(embedder.EmbedToken("OOV", defaultEmbed), defaultEmbed);
    }
}
