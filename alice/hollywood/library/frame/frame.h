#pragma once

#include "slot.h"

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/hollywood/library/util/tptrwrapper.h>

#include <util/generic/list.h>
#include <util/generic/map.h>

#include <iterator>

namespace NAlice::NHollywood {

class TFrame {
public:
    using TSlotMap = TMap<TString, TList<TSlot>>;

    class TMapValuesWrapper {
    public:
        class const_iterator {
        public:
            using value_type = TSlot;
            using pointer = const value_type*;
            using reference = const value_type&;
            using difference_type = ptrdiff_t;
            using iterator_category = std::forward_iterator_tag;

        public:
            explicit const_iterator(const TSlotMap::const_iterator& iter, const TSlotMap::const_iterator& end)
                : Iter_(iter)
                , EndIter_(end)
            {
                ResetSlotIter();
            }

            bool operator==(const const_iterator& other) const {
                return Iter_ == other.Iter_ && SlotIter_ == other.SlotIter_;
            }

            bool operator!=(const const_iterator& other) const {
                return !(*this == other);
            }

            const_iterator& operator++() {
                ++(*SlotIter_);
                if (*SlotIter_ == Iter_->second.end()) {
                    ++Iter_;
                    ResetSlotIter();
                }
                return *this;
            }

            const_iterator operator++(int) {
                const_iterator copy = *this;
                ++(*this);
                return copy;
            }

            reference operator*() const {
                return **SlotIter_;
            }

            pointer operator->() const {
                return &**SlotIter_;
            }

        private:
            void ResetSlotIter() {
                if (Iter_ == EndIter_) {
                    SlotIter_ = Nothing();
                } else {
                    SlotIter_ = Iter_->second.begin();
                }
            }

        private:
            TSlotMap::const_iterator Iter_;
            TSlotMap::const_iterator EndIter_;  // always points past the end of the slot map
            TMaybe<TList<TSlot>::const_iterator> SlotIter_;  // == Nothing() iff we're past the last slot
        };

    public:
        explicit TMapValuesWrapper(const TSlotMap& map, const size_t numSlots)
            : Map_(map)
            , NumSlots_(numSlots)
        {}

        const_iterator begin() const {
            return const_iterator{Map_.begin(), Map_.end()};
        }

        const_iterator end() const {
            return const_iterator{Map_.end(), Map_.end()};
        }

        size_t GetSize() const {
            return NumSlots_;
        }

        size_t size() const {
            return NumSlots_;
        }

    private:
        const TSlotMap& Map_;
        const size_t NumSlots_;
    };

public:
    explicit TFrame(const TString& name)
        : Name_(name)
    {}

    explicit TFrame(TString&& name)
        : Name_(std::move(name))
    {}

    const TString& Name() const {
        return Name_;
    }

    void SetName(const TString& name) {
        Name_ = name;
    }

    TMapValuesWrapper Slots() const {
        return TMapValuesWrapper{SlotMap_, NumSlots_};
    }

    TList<TSlot>* FindSlots(const TStringBuf name) {
        return SlotMap_.FindPtr(name);
    }

    const TList<TSlot>* FindSlots(const TStringBuf name) const {
        return SlotMap_.FindPtr(name);
    }

    TPtrWrapper<TSlot> FindSlot(const TStringBuf name) const {
        if (const auto* slots = FindSlots(name)) {
            if (!slots->empty()) {
                return TPtrWrapper<TSlot>{&slots->front(), name};
            }
        }
        return TPtrWrapper<TSlot>{nullptr, name};
    }

    TFrame& AddSlot(const TSlot& slot) {
        SlotMap_[slot.Name].push_back(slot);
        NumSlots_++;
        return *this;
    }

    TFrame& RemoveSlots(const TStringBuf name) {
        if (const auto it = SlotMap_.find(name); it != SlotMap_.end()) {
            NumSlots_ -= it->second.size();
            SlotMap_.erase(it);
        }
        return *this;
    }

    TSemanticFrame ToProto() const;
    static TFrame FromProto(const TSemanticFrame& semanticFrame);

private:
    TString Name_;
    TSlotMap SlotMap_;  // multimap wouldn't preserve the order of equivalent elements
    size_t NumSlots_ = 0;
};

} // namespace NAlice::NHollywood
