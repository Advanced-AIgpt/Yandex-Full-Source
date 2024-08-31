#include <cstddef>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>

#pragma once

namespace NAlice::NHollywood {

//
// TPtrWrapper: like a TMayBe class but used for pointers only
// Used with FindSlot() / FindSemanticFrame() functions
// Throw an exception in case if slot or frame doesn't exist and it's not checked in the source code
//
template <class T> class TPtrWrapper {
public:
    explicit TPtrWrapper(const T* ptr, const TStringBuf name)
    : Ptr_(ptr)
    , Name_(name)
    {}

    explicit TPtrWrapper(std::nullptr_t, const TStringBuf name)
    : Ptr_(nullptr)
    , Name_(name)
    {}

    inline TPtrWrapper<T>& operator =(const T* ptr) {
       Ptr_ = ptr;
       return *this;
    }

    inline bool operator ==(const T* ptr) const {
        return Ptr_ == ptr;
    }

    operator bool() const {
        return IsValid();
    }

    inline bool operator !=(const T* ptr) const {
        return Ptr_ != ptr;
    }

    inline const T* operator ->() const {
        ValidateAndThrow();
        return Ptr_;
    }
    inline const T& operator *() const {
        ValidateAndThrow();
        return *Ptr_;
    }
    inline const T* Get() const {
        ValidateAndThrow();
        return Ptr_;
    }
    inline const T* GetRaw() const {
        return Ptr_;
    }

    inline bool IsValid() const {
        return Ptr_ != nullptr;
    }

    inline void ValidateAndThrow() const {
        if (!IsValid()) {
            ythrow yexception() << "Trying to access to null pointer for object '" << Name_ << "'";
        }
    }

private:
    const T* Ptr_;
    TString Name_;
};

} // namespace NAlice
