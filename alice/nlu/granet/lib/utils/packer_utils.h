#pragma once

#include <library/cpp/packers/packers.h>
#include <util/generic/va_args.h>

// Macro DEFINE_PACKER creates specialization of NPackers::TPacker<> for your structure.
// Using:
//     struct TMy {
//         int X;
//         TMyOther Y;
//     };
//     DEFINE_PACKER(TMy, X, Y);

namespace NPackerUtils {

    template<typename TData, typename TPacker = NPackers::TPacker<TData>>
    inline void Unpack(const char** buffer, TData* data) {
        TPacker().UnpackLeaf(*buffer, *data);
        *buffer += TPacker().SkipLeaf(*buffer);
    }

    template<typename TData, typename TPacker = NPackers::TPacker<TData>>
    inline void Pack(char** buffer, const TData& data) {
        const size_t size = TPacker().MeasureLeaf(data);
        TPacker().PackLeaf(*buffer, data, size);
        *buffer += size;
    }

    template<typename TData, typename TPacker = NPackers::TPacker<TData>>
    inline size_t Measure(const TData& data) {
        return TPacker().MeasureLeaf(data);
    }

} // namespace NPackerUtils

#define DEFINE_PACKER(classType, ...)                                                       \
    namespace NPackers {                                                                    \
        template <>                                                                         \
        class TPacker<classType> {                                                          \
        public:                                                                             \
            using TClassType = classType;                                                   \
        public:                                                                             \
            void UnpackLeaf(const char* buffer, TClassType& data) const {                   \
                const char* ptr = buffer;                                                   \
                Y_PASS_VA_ARGS(Y_MAP_ARGS(__DEFINE_PACKER_UNPACK_IMPL__, __VA_ARGS__))      \
            }                                                                               \
            void PackLeaf(char* buffer, const TClassType& data, size_t size) const {        \
                char* ptr = buffer;                                                         \
                Y_PASS_VA_ARGS(Y_MAP_ARGS(__DEFINE_PACKER_PACK_IMPL__, __VA_ARGS__))        \
                Y_ASSERT(size == static_cast<size_t>(ptr - buffer));                        \
            }                                                                               \
            size_t MeasureLeaf(const TClassType& data) const {                              \
                size_t size = 0;                                                            \
                Y_PASS_VA_ARGS(Y_MAP_ARGS(__DEFINE_PACKER_MEASURE_IMPL__, __VA_ARGS__))     \
                return size;                                                                \
            }                                                                               \
            size_t SkipLeaf(const char* buffer) const {                                     \
                const char* ptr = buffer;                                                   \
                Y_PASS_VA_ARGS(Y_MAP_ARGS(__DEFINE_PACKER_SKIP_IMPL__, __VA_ARGS__))        \
                return static_cast<size_t>(ptr - buffer);                                   \
            }                                                                               \
        };                                                                                  \
    }                                                                                       \

#define __DEFINE_PACKER_UNPACK_IMPL__(field) NPackerUtils::Unpack(&ptr, &data.field);
#define __DEFINE_PACKER_PACK_IMPL__(field) NPackerUtils::Pack(&ptr, data.field);
#define __DEFINE_PACKER_MEASURE_IMPL__(field) size += NPackerUtils::Measure(data.field);
#define __DEFINE_PACKER_SKIP_IMPL__(field) ptr += NPackers::TPacker<decltype(static_cast<TClassType*>(nullptr)->field)>().SkipLeaf(ptr);
