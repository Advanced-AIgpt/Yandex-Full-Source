#pragma once

#include <util/system/defaults.h>


namespace NTL {

namespace NMurmurPrivate {

    static constexpr const ui32 M32 = 0x5bd1e995;
    static constexpr const ui32 R32 = 24;


    static constexpr ui32 MurmurHashImpl32(const char* key, size_t len, ui32 acc) {
        Y_UNUSED(key, len, acc);

        if (len >= 4) {
            const ui32 a = (key[3] << 24) | (key[2] << 16) | (key[1] << 8) | key[0];
            const ui32 b = a * M32;
            const ui32 c = b ^ (b >> R32);
            const ui32 d = c * M32;

            return MurmurHashImpl32(key + 4, len - 4, (acc * M32) ^ d);
        }

        ui32 e = acc;

        switch (len) {
            case 3:
                e ^= key[2] << 16;

            case 2:
                e ^= key[1] << 8;

            case 1:
                e ^= key[0];
                e *= M32;
        }

        return acc;
    }

    static constexpr ui32 MurmurHash32(const char* key, size_t len, ui32 seed = 0) {
        const ui32 h = MurmurHashImpl32(key, len, ui32(seed ^ len));

        const ui32 a = h ^ (h >> 13);
        const ui32 b = a * M32;
        const ui32 c = b ^ (b >> 15);

        return c;
    }


    static constexpr const ui64 M64 = ULL(0xc6a4a7935bd1e995);

    static constexpr const ui64 R64 = ULL(47);


    static constexpr ui64 MurmurHashImpl64(const char* key, size_t len, ui64 acc) {
        if (len >= 8) {
            const ui64 a = (((ui64)key[7]) << 56) | (((ui64)key[6]) << 48) | (((ui64)key[5]) << 40) | (((ui64)key[4]) << 32) | (((ui64)key[3]) << 24) | (((ui64)key[2]) << 16) | (((ui64)key[1]) << 8) | ((ui64)key[0]);
            const ui64 b = a * M64;
            const ui64 c = b ^ (b >> R64);
            const ui64 d = c * M64;

            return MurmurHashImpl64(key + 8, len - 8, (acc ^ d) * M64);
        }

        switch (len) {
            case 7:
                acc ^= ((ui64)key[6]) << 48;

            case 6:
                acc ^= ((ui64)key[5]) << 40;

            case 5:
                acc ^= ((ui64)key[4]) << 32;

            case 4:
                acc ^= ((ui64)key[3]) << 24;

            case 3:
                acc ^= ((ui64)key[2]) << 16;

            case 2:
                acc ^= ((ui64)key[1]) << 8;

            case 1:
                acc ^= ((ui64)key[0]);
                acc *= M64;
        }

        return acc;
    }


    static constexpr ui64 MurmurHash64(const char* key, size_t len, ui64 seed = 0) {
        const ui64 h = MurmurHashImpl64(key, len, seed ^ ui64(M64 * len));

        const ui64 a = h ^ (h >> R64);
        const ui64 b = a * M64;
        const ui64 c = b ^ (b >> R64);

        return c;
    }


    template <unsigned N>
    struct TMurHelper;


#define DEF_MUR(t)                                                                         \
    template <>                                                                            \
    struct TMurHelper<t> {                                                                 \
        static constexpr ui##t MurmurHash(const char* buf, size_t len, ui##t init) {       \
            return MurmurHash##t(buf, len, init);                                          \
        }                                                                                  \
    };

    DEF_MUR(32)
    DEF_MUR(64)

#undef DEF_MUR


static constexpr size_t StrLen(const char *ch, size_t acc = 0) {
    if (!ch || !*ch) return acc;
    return StrLen(ch + 1, acc + 1);
}

}   // namespace NMurMurPrivate


template <class T>
static constexpr T MurmurHash(const char* buf, size_t len, T init) {
    return (T)NMurmurPrivate::TMurHelper<8 * sizeof(T)>::MurmurHash(buf, len, init);
}


template <class T>
static constexpr T MurmurHash(const char* buf, size_t len) {
    return MurmurHash<T>(buf, len, (T)0);
}


template <class T>
static constexpr T MurmurHash(const char *buf) {
    return MurmurHash<T>(reinterpret_cast<const void*>(buf), NMurmurPrivate::StrLen(buf), (T)0);
}


static constexpr ui32 MurmurHash32(const char *buf) {
    return NMurmurPrivate::MurmurHash32(buf, NMurmurPrivate::StrLen(buf), 0);
}


static constexpr ui64 MurmurHash64(const char *buf) {
    return NMurmurPrivate::MurmurHash64(buf, NMurmurPrivate::StrLen(buf), 0);
}


}   // namespace NTL
