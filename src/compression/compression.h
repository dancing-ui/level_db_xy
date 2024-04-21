#ifndef _LEVEL_DB_XY_COMPRESSION_H_
#define _LEVEL_DB_XY_COMPRESSION_H_

namespace ns_compression {

enum CompressionType {
    kNoCompression = 0x0,
    kSnappyCompression = 0x1,
    kZstdCompression = 0x2,
};

} // ns_compression

#endif