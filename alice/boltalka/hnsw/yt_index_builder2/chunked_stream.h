#pragma once
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/generic/buffer.h>

#include <functional>

namespace NYtHnsw {

class TChunkedOutputStream : public IOutputStream {
public:
    using TCallback = std::function<void(const TBuffer&)>;

    TChunkedOutputStream(size_t chunkSize, const TCallback& chunkEndCallback);

    ~TChunkedOutputStream();

protected:
    void DoWrite(const void* buf, size_t len) override;
    void DoFlush() override;

private:
    const size_t ChunkSize;
    TCallback ChunkEndCallback;
    TBuffer Chunk;
};

class TChunkedInputStream : public IInputStream {
public:
    using TCallback = std::function<void(TBuffer&)>;

    TChunkedInputStream(const TCallback& nextChunkCallback);

protected:
    size_t DoRead(void* buf, size_t len) override;

private:
    TCallback NextChunkCallback;
    TBuffer Chunk;
    TBuffer::TConstIterator ChunkPos;
};

}

