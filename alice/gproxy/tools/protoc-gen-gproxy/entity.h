#pragma once

#include <util/string/subst.h>

#include <google/protobuf/descriptor.h>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>


namespace NGProxyTraits {


template <class Accessor, class Container, class ItemType, class ItemContainerType>
struct TBaseAccessor {
    using ValueType = ItemContainerType;

    static inline int Next(const Container *p, int index) {
        if (Accessor::Count(p) > index) {
            return index + 1;
        }
        return index;
    }

    static inline int Prev(const Container *p, int index) {
        if (index > 0) {
            return index - 1;
        }
        return Accessor::Count(p);
    }

    static inline bool IsValid(const Container *p, int index) {
        return (index >= 0) && (index < Accessor::Count(p));
    }

    static inline ValueType Get(const Container *p, int index) {
        if (!IsValid(p, index)) return nullptr;
        return ItemContainerType(Accessor::Get(p, index));
    }
};


template <class T>
struct TNameGetter {
    static TProtoStringType GetName(const T *d) {
        return d->name();
    }

    static TProtoStringType GetFullName(const T *d) {
        return "::" + SubstGlobalCopy(d->full_name(), ".", "::");
    }
};

template <>
struct TNameGetter<::google::protobuf::FieldDescriptor> {
    static TProtoStringType GetName(const ::google::protobuf::FieldDescriptor *d) {
        return d->name();
    }

    static TProtoStringType GetFullName(const ::google::protobuf::FieldDescriptor *d) {
        return "::" + SubstGlobalCopy(d->full_name(), ".", "::");
    }

    static TProtoStringType GetJsonName(const ::google::protobuf::FieldDescriptor *d) {
        if (d->has_json_name()) {
            return d->json_name();
        } else {
            return d->name();
        }
    }
};


template <>
struct TNameGetter<::google::protobuf::FileDescriptor> {
    static TProtoStringType GetName(const ::google::protobuf::FileDescriptor *d) {
        return d->name();
    }

    static TProtoStringType GetFullName(const ::google::protobuf::FileDescriptor *d) {
        return d->name();
    }
};


template <typename T = google::protobuf::Descriptor>
class TEntity {
public:
    TEntity(const T *descriptor)
        : Descriptor(descriptor)
    { }

    TEntity(const TEntity&) = default;

    TEntity(TEntity&&) = default;

    inline TProtoStringType Name() const {
        return TNameGetter<T>::GetName(Descriptor);
    }

    inline TProtoStringType NameLower() const {
        TProtoStringType lower = TNameGetter<T>::GetName(Descriptor);
        lower.to_lower();
        return lower;
    }

    inline TProtoStringType FullName() const {
        return TNameGetter<T>::GetFullName(Descriptor);
    }

    inline TProtoStringType JsonName() const {
        return TNameGetter<T>::GetJsonName(Descriptor);
    }

    inline TProtoStringType ProtobufPath() const {
        return Descriptor->file()->name();
    }

    inline TProtoStringType HeaderPath(const TProtoStringType& suffix = "") const {
        if (suffix.empty()) {
            return google::protobuf::compiler::cpp::StripProto(ProtobufPath()) + ".pb.h";
        } else {
            return google::protobuf::compiler::cpp::StripProto(ProtobufPath()) + "." + suffix + ".pb.h";
        }
    }

    inline const T* Get() const {
        return Descriptor;
    }

    inline operator bool() const {
        return  Descriptor != nullptr;
    }

    template <typename A>
    struct TBaseIterator {
        using ValueType = typename A::ValueType;

        TBaseIterator(const T *d, bool end = false)
            : D(d)
            , I(end ? A::Count(d) : 0)
        { }

        TBaseIterator(const TBaseIterator<A>&) = default;

        TBaseIterator(TBaseIterator<A>&&) = default;

        TBaseIterator& operator=(const TBaseIterator<A>&) = default;

        TBaseIterator& operator=(TBaseIterator<A>&&) = default;


        TBaseIterator& operator++() {
            I = A::Next(D, I);
            return *this;
        }

        TBaseIterator& operator--() {
            I = A::Prev(D, I);
            return *this;
        }


        TBaseIterator operator++(int) {
            TBaseIterator tmp = *this;
            I = A::Next(D, I);
            return tmp;
        }

        TBaseIterator operator--(int) {
            TBaseIterator tmp = *this;
            I = A::Prev(D, I);
            return tmp;
        }

        operator bool() const {
            return A::IsValid(D, I);
        }


        inline bool operator==(const TBaseIterator<A>& other) const {
            return D == other.D
                && I == other.I
            ;
        }

        inline bool operator!=(const TBaseIterator<A>& other) const {
            return D != other.D
                || I != other.I
            ;
        }

        ValueType operator*() const {
            return A::Get(D, I);
        }

        const T *D { nullptr };
        int      I = 0;
    };

private:
    const T *Descriptor { nullptr };
};  // class TEntity

}   // namespace NGProxyTraits
