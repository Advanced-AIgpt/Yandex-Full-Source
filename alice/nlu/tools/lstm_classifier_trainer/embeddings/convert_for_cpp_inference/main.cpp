#include <alice/nlu/libs/embedder/embedder.h>

#include <contrib/libs/cnpy/cnpy.h>
#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/folder/path.h>
#include <util/generic/map.h>
#include <util/generic/utility.h>
#include <util/memory/blob.h>
#include <util/stream/buffer.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/string/join.h>

namespace {
    constexpr int SANITY_VALUES_TO_PRINT = 10;

    using TVocab = TMap<TString, NAlice::TTokenIndex>; // use TMap for sortedness by key
    using TTrie = TCompactTrie<char, NAlice::TTokenIndex>;
    using TTrieBuilder = TCompactTrieBuilder<char, NAlice::TTokenIndex>;

    struct TEmbeddingsConverterOpts {
        TFsPath VocabPath;
        TFsPath EmbeddingsBlobPath;
        int Dimension = 300;
        TFsPath OutputPath;

        TEmbeddingsConverterOpts(int argc, const char** argv) {
            NLastGetopt::TOpts opts;

            opts.SetFreeArgsNum(0);

            opts.AddLongOption('v', "vocab", "Path to vocab")
                .StoreResult(&VocabPath)
                .Required();

            opts.AddLongOption('e', "embeddings", "Path to blob with embeddings")
                .StoreResult(&EmbeddingsBlobPath)
                .Required();

            opts.AddLongOption('d', "dimension", "Dimension of embeddings")
                .StoreResult(&Dimension)
                .DefaultValue(300);

            opts.AddLongOption('o', "output-path", "Output path with trie and embeddings blobs")
                .StoreResult(&OutputPath)
                .Required();

            NLastGetopt::TOptsParseResult parseOpts{&opts, argc, argv};
        }
    };

    TVocab LoadVocab(const TFsPath& path) {
        TVocab vocab;

        TFileInput input(path);
        TString line;
        while (input.ReadLine(line)) {
            const auto [it, is_inserted] = vocab.emplace(std::move(line), vocab.size());
            Y_ENSURE(is_inserted, "non-unique keys in vocab: " << it->first);
        }

        return vocab;
    }

    TTrie ConvertVocabToTrie(const TVocab& vocab) {
        TTrieBuilder trieBuilder(CTBF_PREFIX_GROUPED | CTBF_UNIQUE);

        for(const auto& [word, idx] : vocab) {
            trieBuilder.Add(word, idx);
        }

        TBufferOutput buffer;
        CompactTrieMakeFastLayout(buffer, trieBuilder);

        return TTrie(TBlob::FromBuffer(buffer.Buffer()));
    }

    void ValidateTrie(const TVocab& vocab, const TTrie& trie) {
        Y_ENSURE(vocab.size() == trie.Size());

        for(const auto& [word, idx] : vocab) {
            Y_ENSURE(idx == trie.Get(word));
        }
    }

    void ValidateEmbeddings(const TVocab& vocab, const int dimension, const cnpy::NpyArray& embeddings) {
        Y_ENSURE(!embeddings.fortran_order);
        Y_ENSURE(embeddings.shape.size() == 2);
        Y_ENSURE(embeddings.shape[0] == vocab.size());
        Y_ENSURE(embeddings.shape[1] == static_cast<unsigned long>(dimension));
        Y_ENSURE(embeddings.num_bytes() == vocab.size() * dimension * NAlice::TEmbeddingElementSize);
    }
} // namespace

int main(int argc, const char** argv) {
    const TEmbeddingsConverterOpts options(argc, argv);
    const TFsPath outputTriePath = options.OutputPath / "embeddings_dictionary.trie";
    const TFsPath outputEmbeddingsPath = options.OutputPath / "embeddings";

    const TVocab vocab = LoadVocab(options.VocabPath);
    Cerr << "Loaded vocab with " << vocab.size() << " words" << Endl;
    Y_ENSURE(vocab.size());

    {
        const TTrie trie = ConvertVocabToTrie(vocab);
        ValidateTrie(vocab, trie);
        TFileOutput(outputTriePath).Write(trie.Data().Data(), trie.Data().Size());
    }

    {
        const cnpy::NpyArray numpyEmbeddings = cnpy::npy_load(options.EmbeddingsBlobPath.GetPath());
        ValidateEmbeddings(vocab, options.Dimension, numpyEmbeddings);
        TFileOutput(outputEmbeddingsPath).Write(numpyEmbeddings.data_holder->data(), numpyEmbeddings.num_bytes());
    }

    {
        const NAlice::TTokenEmbedder embedder(
            TBlob::PrechargedFromFile(outputEmbeddingsPath), TBlob::PrechargedFromFile(outputTriePath), options.Dimension);

        const int valuesToPrint = Min(SANITY_VALUES_TO_PRINT, options.Dimension);
        Cout << "Brief view of the dumped embedder (for sanity check, first " << valuesToPrint << " values of embedding):" << Endl;

        const TString& firstWord = vocab.begin()->first;
        const NAlice::TEmbedding firstWordEmbed = embedder.EmbedToken(firstWord);

        Cout << firstWord << ' ';
        Cout << JoinRange(" ", firstWordEmbed.begin(), firstWordEmbed.begin() + valuesToPrint) << Endl;
        Cout << "..." << Endl;

        const TString& lastWord = vocab.rbegin()->first;
        const NAlice::TEmbedding lastWordEmbed = embedder.EmbedToken(lastWord);

        Cout << lastWord << ' ';
        Cout << JoinRange(" ", lastWordEmbed.begin(), lastWordEmbed.begin() + valuesToPrint) << Endl;
    }
}
