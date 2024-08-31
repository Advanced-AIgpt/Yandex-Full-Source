#pragma once

#include <google/protobuf/repeated_field.h>

namespace NAlice {

    // Add elements of Container to protobuf repeated field
    template<class Container, class Element>
    void ExtendRepeatedField(const Container& src, ::google::protobuf::RepeatedPtrField<Element>* dest) {
        Y_ASSERT(dest);
        for (const auto& item : src) {
            *dest->Add() = item;
        }
    }

} // namespace NAlice
