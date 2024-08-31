#include <robot/library/message_io/file_message_io.h>

#include <library/cpp/eventlog/logparser.h>
#include <library/cpp/digest/crc32c/crc32c.h>

#include <util/generic/vector.h>
#include <util/string/cast.h>
#include <util/stream/file.h>
#include <util/system/file.h>
#include <util/system/fs.h>

using namespace NRobot;

size_t GetStartFrameOffset(TFrameStreamer& frameStreamer) {
    const TFrame& frame = *frameStreamer;
    return frame.Address() - COMPRESSED_LOG_FRAME_SYNC_DATA.size();
}

TStringBuf GetTopFrameBuffer(TStringBuf chunk) {
    TMemoryInput input(chunk);
    TFrameStreamer frameStreamer(input, NEvClass::Factory());
    const auto frameLength = frameStreamer.Next() ? GetStartFrameOffset(frameStreamer) : chunk.size();
    return TStringBuf(chunk.begin(), frameLength);
}

int main(int argc, const char* argv[]) {
    if (argc != 4) {
        Cout << "eventlog_splitter input-file output-file frames-per-message" << Endl;
        return -1;
    }
    const auto inputFile = TString(argv[1]);
    const auto outputFile = TString(argv[2]);
    const auto framesPerMessage = FromString<ui64>(argv[3]);

    const auto content = TFileInput(inputFile).ReadAll();
    auto chunk = TStringBuf(content);
    auto writer = MakeFileMessageWriter(outputFile, EFileMessageFormat::BinaryLenval);

    TBuffer currentMessage;
    ui64 currentMessageFramesCount = 0;
    ui64 framesRead = 0;
    ui64 messagesWritten = 0;
    while (!chunk.empty()) {
        const auto frameChunk = GetTopFrameBuffer(chunk);
        ++framesRead;
        currentMessage.Append(frameChunk.data(), frameChunk.size());
        chunk.Skip(frameChunk.size());
        ++currentMessageFramesCount;
        if (currentMessageFramesCount == framesPerMessage) {
            writer->Write(TStringBuf(currentMessage.data(), currentMessage.size()));
            ++messagesWritten;
            currentMessage.Clear();
        }
    }
    if (currentMessageFramesCount > 0) {
        writer->Write(TStringBuf(currentMessage.data(), currentMessage.size()));
        ++messagesWritten;
    }

    Cout << "frames read: " << framesRead << Endl;
    Cout << "messages written: " << messagesWritten << Endl;
    return 0;
}
