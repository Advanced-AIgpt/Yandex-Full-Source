#include <library/cpp/dot_product/dot_product.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/threading/local_executor/local_executor.h>

#include <util/generic/vector.h>
#include <util/generic/deque.h>
#include <util/generic/queue.h>
#include <util/generic/ymath.h>
#include <util/memory/blob.h>
#include <util/random/shuffle.h> // TODO(alipov): optional shuffle
#include <util/string/cast.h>
#include <util/system/hp_timer.h>
#include <util/stream/file.h>

struct TOptions {
    TString VectorFilename;
    size_t Dimension;
    size_t NumThreads;
    size_t MaxNeighbors;
    size_t BatchSize;
    size_t UpperLevelBatchSize;
    size_t SearchNeighborhoodSize;
    size_t NumExactCandidates;
    size_t LevelSizeDecay;
    bool Verbose = false;
    TString OutputFilename;

    TOptions(int argc, char **argv) {
        NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
        opts
            .AddHelpOption();
        opts
            .AddLongOption('v', "vectors")
            .RequiredArgument("FILE")
            .StoreResult(&VectorFilename)
            .Required();
        opts
            .AddLongOption('d', "dim")
            .RequiredArgument("INT")
            .StoreResult(&Dimension)
            .Required();
        opts.
            AddLongOption('o', "output")
            .RequiredArgument("FILE")
            .StoreResult(&OutputFilename)
            .Required();
        opts
            .AddLongOption('m', "max-neighbors")
            .RequiredArgument("INT")
            .StoreResult(&MaxNeighbors)
            .DefaultValue("32");
        opts
            .AddLongOption('t', "num-threads")
            .RequiredArgument("INT")
            .StoreResult(&NumThreads)
            .DefaultValue("32");
        opts
            .AddLongOption('b', "batch-size")
            .RequiredArgument("INT")
            .StoreResult(&BatchSize)
            .DefaultValue("1000")
            .Help("Batch size. Affects performance.");
        opts
            .AddLongOption('u', "upper-level-batch-size")
            .RequiredArgument("INT")
            .StoreResult(&UpperLevelBatchSize)
            .DefaultValue("40000")
            .Help("Batch size for building upper levels. Affects accuracy.");
        opts
            .AddLongOption('s', "search-neighborhood-size")
            .RequiredArgument("INT")
            .StoreResult(&SearchNeighborhoodSize)
            .DefaultValue("300")
            .Help("Search neighborhood size for ANN-search");
        opts
            .AddLongOption('e', "num-exact-candidates")
            .RequiredArgument("INT")
            .StoreResult(&NumExactCandidates)
            .DefaultValue("100")
            .Help("Number of nearest vectors in batch.");
        opts
            .AddLongOption('l', "level-size-decay")
            .RequiredArgument("INT")
            .DefaultValue("max-neighbors / 2")
            .Help("Base of exponent for decaying level sizes.");
        opts
            .AddLongOption("verbose")
            .NoArgument()
            .SetFlag(&Verbose);

        opts.SetFreeArgsNum(0);
        opts.AddHelpOption('h');

        NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);

        if (!TryFromString<size_t>(parsedOpts.Get("level-size-decay"), LevelSizeDecay)) {
            LevelSizeDecay = MaxNeighbors / 2;
        }

        Y_VERIFY(1 <= MaxNeighbors && MaxNeighbors <= SearchNeighborhoodSize);
        Y_VERIFY(1 <= MaxNeighbors && MaxNeighbors <= NumExactCandidates);
        Y_VERIFY(BatchSize > MaxNeighbors);
        Y_VERIFY(LevelSizeDecay > 1);

        UpperLevelBatchSize = Max(UpperLevelBatchSize, BatchSize);
    }
};

struct TNeighbor {
    float Dist;
    size_t Id;

    bool operator<(const TNeighbor& other) const {
        return Dist > other.Dist || Dist == other.Dist && Id < other.Id;
    }
    bool operator>(const TNeighbor& other) const {
        return other < *this;
    }
};
using TNeighbors = TVector<TNeighbor>;
using TGraph = TVector<TNeighbors>;

template<class T>
using TMinQueue = TPriorityQueue<T, TVector<T>, TGreater<T>>;
template<class T>
using TMaxQueue = TPriorityQueue<T>;

void TrimNeighbors(const TOptions& opts, const float* vectors, TNeighbors* neighbors) {
    TMinQueue<TNeighbor> candidates(neighbors->begin(), neighbors->end());
    TNeighbors nearestNotAdded;
    neighbors->clear();
    while (!candidates.empty() && neighbors->size() < opts.MaxNeighbors) {
        auto cur = candidates.top();
        candidates.pop();
        const float* curVector = vectors + cur.Id * opts.Dimension;
        bool add = true;
        for (const auto& n : *neighbors) {
            float distToNeighbor = DotProduct(curVector, vectors + n.Id * opts.Dimension, opts.Dimension);
            if (distToNeighbor > cur.Dist) {
                add = false;
                break;
            }
        }
        if (add) {
            neighbors->push_back(cur);
        } else if (nearestNotAdded.size() + neighbors->size() < opts.MaxNeighbors) {
            nearestNotAdded.push_back(cur);
        }
    }
    for (size_t i = 0; i < nearestNotAdded.size() && neighbors->size() < opts.MaxNeighbors; ++i) {
        neighbors->push_back(nearestNotAdded[i]);
    }
    neighbors->shrink_to_fit();
}

TNeighbors FindApproximateNeighbors(const TOptions& opts, const float* vectors, const TDeque<TGraph>& levels, const float* query) {
    size_t entryId = 0;
    float entryDist = DotProduct(query, vectors, opts.Dimension);
    for (size_t level = levels.size(); level-- > 1; ) {
        for (bool entryChanged = true; entryChanged; ) {
            entryChanged = false;
            const auto& neighbors = levels[level][entryId];
            for (size_t i = 0; i < neighbors.size(); ++i) {
                size_t id = neighbors[i].Id;
                float distToQuery = DotProduct(query, vectors + id * opts.Dimension, opts.Dimension);
                if (distToQuery > entryDist) {
                    entryDist = distToQuery;
                    entryId = id;
                    entryChanged = true;
                }
            }
        }
    }

    TMaxQueue<TNeighbor> nearest;
    TMinQueue<TNeighbor> candidates;
    THashSet<size_t> visited;
    nearest.push({entryDist, entryId});
    candidates.push({entryDist, entryId});
    visited.insert(entryId);

    while (!candidates.empty()) {
        auto cur = candidates.top();
        candidates.pop();
        if (cur.Dist < nearest.top().Dist) {
            break;
        }
        const auto& neighbors = levels[0][cur.Id];
        for (size_t i = 0; i < neighbors.size(); ++i) {
            size_t id = neighbors[i].Id;
            if (visited.contains(id)) {
                continue;
            }
            float distToQuery = DotProduct(query, vectors + id * opts.Dimension, opts.Dimension);
            if (nearest.size() < opts.SearchNeighborhoodSize || distToQuery > nearest.top().Dist) {
                nearest.push({distToQuery, id});
                candidates.push({distToQuery, id});
                visited.insert(id);
                if (nearest.size() > opts.SearchNeighborhoodSize) {
                    nearest.pop();
                }
            }
        }
    }

    TNeighbors result;
    for (; !nearest.empty(); nearest.pop()) {
        result.push_back(nearest.top());
    }
    std::reverse(result.begin(), result.end());
    return result;
}

void BuildApproximateNeighbors(const TOptions& opts, const float* vectors, const TDeque<TGraph>& levels, size_t batchBegin, size_t batchEnd, TGraph* level) {
    auto task = [&](int id) {
        (*level)[id] = FindApproximateNeighbors(opts, vectors, levels, vectors + id * opts.Dimension);
    };
    NPar::LocalExecutor().ExecRange(task, batchBegin, batchEnd, NPar::TLocalExecutor::WAIT_COMPLETE);
}

void AddExactNeighborsInBatch(const TOptions& opts, const float* vectors, size_t batchBegin, size_t batchEnd, TGraph* level) {
    auto task = [&](int id_) {
        const size_t id = id_;
        const float* curVector = vectors + id * opts.Dimension;
        TMaxQueue<TNeighbor> nearest;
        for (size_t otherId = batchBegin; otherId < batchEnd; ++otherId) {
            if (otherId == id) {
                continue;
            }
            float dist = DotProduct(curVector, vectors + otherId * opts.Dimension, opts.Dimension);
            nearest.push({dist, otherId});
            if (nearest.size() > opts.NumExactCandidates) {
                nearest.pop();
            }
        }
        auto& neighbors = (*level)[id];
        for (; !nearest.empty(); nearest.pop()) {
            neighbors.push_back(nearest.top());
        }
        TrimNeighbors(opts, vectors, &neighbors);
    };
    NPar::LocalExecutor().ExecRange(task, batchBegin, batchEnd, NPar::TLocalExecutor::WAIT_COMPLETE);
}

void UpdatePreviousNeighbors(const TOptions& opts, const float* vectors, size_t batchBegin, size_t batchEnd, TGraph* level) {
    THashSet<size_t> toTrimSet;
    for (size_t id = batchBegin; id < batchEnd; ++id) {
        const auto& neighbors = (*level)[id];
        for (const auto& n : neighbors) {
            if (n.Id < batchBegin) {
                (*level)[n.Id].push_back({n.Dist, id});
                toTrimSet.insert(n.Id);
            }
        }
    }
    TVector<size_t> toTrim(toTrimSet.begin(), toTrimSet.end());
    auto task = [&](int id) {
        auto& neighbors = (*level)[toTrim[id]];
        TrimNeighbors(opts, vectors, &neighbors);
    };
    NPar::LocalExecutor().ExecRange(task, 0, toTrim.size(), NPar::TLocalExecutor::WAIT_COMPLETE);
}

void FinalizeNeighbors(const TOptions& opts, TGraph* level) {
    auto task = [&](int id) {
        auto& neighbors = (*level)[id];
        Y_VERIFY(neighbors.size() == Min(opts.MaxNeighbors, level->size() - 1));
    };
    NPar::LocalExecutor().ExecRange(task, 0, level->size(), NPar::TLocalExecutor::WAIT_COMPLETE);
}

TGraph BuildLevel(const TOptions& opts, const float* vectors, TDeque<TGraph>* levels, size_t levelSize, size_t batchSize) {
    levels->push_front(TGraph(levelSize));
    auto& level = levels->front();
    size_t builtLevelSize = 0;
    if (levels->size() > 1) {
        const auto& previousLevel = (*levels)[1];
        if (previousLevel.size() >= batchSize) {
            std::copy(previousLevel.begin(), previousLevel.end(), level.begin());
            builtLevelSize = previousLevel.size();
        }
    }

    THPTimer globalWatch;
    for (size_t batchBegin = builtLevelSize; batchBegin < levelSize; ) {
        const size_t curBatchSize = Min(levelSize - batchBegin, batchSize);
        const size_t batchEnd = batchBegin + curBatchSize;

        THPTimer watch;
        if (batchBegin > 0) {
            BuildApproximateNeighbors(opts, vectors, *levels, batchBegin, batchEnd, &level);
            if (opts.Verbose) {
                Cerr << "\tbuild ann " << watch.PassedReset() / curBatchSize << Endl;
            }
        }
        AddExactNeighborsInBatch(opts, vectors, batchBegin, batchEnd, &level);
        if (opts.Verbose) {
            Cerr << "\tbuild exact " << watch.PassedReset() / curBatchSize << Endl;
        }
        UpdatePreviousNeighbors(opts, vectors, batchBegin, batchEnd, &level);
        if (opts.Verbose) {
            Cerr << "\tbuild prev " << watch.PassedReset() / curBatchSize << Endl;
        }
        batchBegin = batchEnd;

        Cerr << batchEnd * 100 / levelSize << "% done\r";
        if (opts.Verbose) {
            size_t numProcessed = batchEnd - builtLevelSize;
            Cerr << Endl << batchEnd << '\t' << globalWatch.Passed() / numProcessed << '\t' << numProcessed / globalWatch.Passed() << Endl;
        }
    }
    FinalizeNeighbors(opts, &level);
    return level;
}

TVector<size_t> GetLevelSizes(size_t numVectors, size_t levelSizeDecay) {
    TVector<size_t> levelSizes;
    for (; numVectors > 1; numVectors /= levelSizeDecay) {
        levelSizes.push_back(numVectors);
    }
    return levelSizes;
}

TDeque<TGraph> BuildIndex(const TOptions& opts, const float* vectors, size_t numVectors) {
    auto levelSizes = GetLevelSizes(numVectors, opts.LevelSizeDecay);

    TDeque<TGraph> levels;
    for (size_t level = levelSizes.size(); level-- > 0; ) {
        Cerr << "Building level " << level << " size " << levelSizes[level] << Endl;
        size_t batchSize = level == 0 ? opts.BatchSize : opts.UpperLevelBatchSize;
        BuildLevel(opts, vectors, &levels, levelSizes[level], batchSize);
    }
    Cerr << Endl << "Done" << Endl;
    return levels;
}

void SaveIndex(const TOptions& opts, const TDeque<TGraph>& levels) {
    Cerr << "Saving index..." << Endl;
    TFixedBufferFileOutput out(opts.OutputFilename);

    ui32 numVectors = levels[0].size();
    out.Write(&numVectors, sizeof(numVectors));

    ui32 maxNeighbors = opts.MaxNeighbors;
    out.Write(&maxNeighbors, sizeof(maxNeighbors));

    ui32 levelSizeDecay = opts.LevelSizeDecay;
    out.Write(&levelSizeDecay, sizeof(levelSizeDecay));

    for (const auto& level : levels) {
        for (const auto& neighbors : level) {
            TVector<ui32> ids;
            ids.reserve(opts.MaxNeighbors);
            for (const auto& n : neighbors) {
                ids.push_back(n.Id);
            }
            out.Write(ids.data(), ids.size() * sizeof(ids[0]));
        }
    }
    Cerr << "Done" << Endl;
}

int main(int argc, char **argv) {
    TOptions opts(argc, argv);

    NPar::LocalExecutor().RunAdditionalThreads(opts.NumThreads - 1);

    TBlob blob = TBlob::PrechargedFromFile(opts.VectorFilename);
    const float* vectors = reinterpret_cast<const float*>(blob.Begin());
    size_t numVectors = blob.Size() / sizeof(float) / opts.Dimension;

    Y_VERIFY(numVectors > opts.MaxNeighbors);

    auto levels = BuildIndex(opts, vectors, numVectors);
    SaveIndex(opts, levels);

    return 0;
}
