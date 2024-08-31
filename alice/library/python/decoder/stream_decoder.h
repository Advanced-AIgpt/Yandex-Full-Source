#pragma once

// Pure C API to be used in .pyx

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef void* StreamDecoderPtr;

StreamDecoderPtr createStreamDecoder(unsigned sample_rate);
void destroyStreamDecoder(StreamDecoderPtr);
void streamDecoderWrite(StreamDecoderPtr, const char *buf, size_t size);
size_t streamDecoderRead(StreamDecoderPtr, char *buf, size_t size);
int streamDecoderEof(StreamDecoderPtr);
int streamDecoderGetError(StreamDecoderPtr, char* err, size_t err_limit, size_t* err_len);

#ifdef __cplusplus
}  // extern "C"
#endif
