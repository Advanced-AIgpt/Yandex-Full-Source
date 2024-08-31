#include "chunked_stream.h"

namespace NYtHnsw {

TChunkedOutputStream::TChunkedOutputStream(size_t chunkSize, const TCallback& chunkEndCallback)
    : ChunkSize(chunkSize)
    , ChunkEndCallback(chunkEndCallback)
    , Chunk(ChunkSize)
{
}

TChunkedOutputStream::~TChunkedOutputStream() {
    DoFlush();
}

void TChunkedOutputStream::DoWrite(const void* buf, size_t len) {
    const char* ptr = reinterpret_cast<const char*>(buf);
    while (len > 0) {
        const size_t writeBytes = Min(len, Chunk.Avail());
        Chunk.Append(ptr, writeBytes);
        ptr += writeBytes;
        len -= writeBytes;
        if (Chunk.Avail() == 0) {
            ChunkEndCallback(Chunk);
            Chunk.Clear();
        }
    }
}

void TChunkedOutputStream::DoFlush() {
    if (Chunk.Size() > 0) {
        ChunkEndCallback(Chunk);
        Chunk.Clear();
    }
}

TChunkedInputStream::TChunkedInputStream(const TCallback& nextChunkCallback)
    : NextChunkCallback(nextChunkCallback)
    , ChunkPos(Chunk.Begin())
{
}

size_t TChunkedInputStream::DoRead(void* buf, size_t len) {
    const size_t res = len;
    char* ptr = reinterpret_cast<char*>(buf);
    while (len > 0) {
        if (ChunkPos == Chunk.End()) {
            NextChunkCallback(Chunk);
            Y_VERIFY(Chunk.Begin() < Chunk.End());
            ChunkPos = Chunk.Begin();
        }
        const size_t readBytes = Min<size_t>(len, Chunk.End() - ChunkPos);
        std::memcpy(ptr, ChunkPos, readBytes);
        ChunkPos += readBytes;
        ptr += readBytes;
        len -= readBytes;
    }
    return res;
}

}

