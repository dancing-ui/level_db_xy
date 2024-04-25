#ifndef _LEVEL_DB_XY_COMPRESSION_H_
#define _LEVEL_DB_XY_COMPRESSION_H_

#include <cstdint>
#include <string>

#if defined(HAVE_SNAPPY)
#include <snappy.h>
#endif // HAVE_SNAPPY

#if defined(HAVE_ZSTD)
#define ZSTD_STATIC_LINKING_ONLY  // For ZSTD_compressionParameters.
#include <zstd.h>
#endif // HAVE_ZSTD


namespace ns_compression {

enum CompressionType {
    kNoCompression = 0x0,
    kSnappyCompression = 0x1,
    kZstdCompression = 0x2,
};

inline bool Snappy_Compress(char const *input, uint64_t length, std::string *output) {
#if defined(HAVE_SNAPPY)
    output->resize(snappy::MaxCompressedLength(length));
    uint64_t outlen;
    snappy::RawCompress(input, length, &(*output)[0], &outlen);
    output->resize(outlen);
    return true;
#endif // HAVE_SNAPPY
    static_cast<void>(input);
    static_cast<void>(length);
    static_cast<void>(output);
    return false;
}

inline bool Snappy_GetUncompressedLength(char const *input, uint64_t length, uint64_t *result) {
#if defined(HAVE_SNAPPY)
    return snappy::GetUncompressedLength(input, length, result);
#endif // HAVE_SNAPPY
    static_cast<void>(input);
    static_cast<void>(length);
    static_cast<void>(result);
    return false;
}

inline bool Snappy_Uncompress(char const *input, uint64_t length, char *output) {
#if defined(HAVE_SNAPPY)
    return snappy::RawUncompress(input, length, output);
#endif // HAVE_SNAPPY
    static_cast<void>(input);
    static_cast<void>(length);
    static_cast<void>(output);
    return false;
}

inline bool Zstd_Compress(int32_t level, char const* input, uint64_t length, std::string* output) {
#if defined(HAVE_ZSTD)
    uint64_t outlen = ZSTD_compressBound(length);
    if(ZSTD_isError(outlen)) {
        return false;
    }
    output->resize(outlen);
    ZSTD_CCtx* ctx = ZSTD_createCCtx();
    ZSTD_compressionParameters parameters = ZSTD_getCParams(level, std::max(length, static_cast<uint64_t>(1)), 0);
    ZSTD_CCtx_setCParams(ctx, parameters);
    outlen = ZSTD_compress2(ctx, &(*output)[0], output->size(), input, length);
    ZSTD_freeCCtx(ctx);
    if(ZSTD_isError(outlen)) {
        return false;
    }
    output->resize(outlen);
    return true;
#endif // HAVE_ZSTD
    static_cast<void>(level);
    static_cast<void>(input);
    static_cast<void>(length);
    static_cast<void>(output);
    return false;
}

inline bool Zstd_GetUncompressedLength(char const* input, uint64_t length, uint64_t* result) {
#if defined(HAVE_ZSTD)
    uint64_t size = ZSTD_getFrameContentSize(input, length);
    if(size == 0) return false;
    *result = size;
    return true;
#endif // HAVE_ZSTD
    static_cast<void>(input);
    static_cast<void>(length);
    static_cast<void>(result);
    return false;
}

inline bool Zstd_Uncompress(char const* input, uint64_t length, char* output) {
#if defined(HAVE_ZSTD)
    uint64_t outlen;
    if(!Zstd_GetUncompressedLength(input, length, &outlen)) {
        return false;
    }
    ZSTD_DCtx* ctx = ZSTD_createDCtx();
    outlen = ZSTD_decompressDCtx(ctx, output, outlen, input, length);
    ZSTD_freeDCtx(ctx);
    if(ZSTD_isError(outlen)) {
        return false;
    }
    return true;
#endif // HAVE_ZSTD
    static_cast<void>(input);
    static_cast<void>(length);
    static_cast<void>(output);
    return false;
}



} // ns_compression

#endif