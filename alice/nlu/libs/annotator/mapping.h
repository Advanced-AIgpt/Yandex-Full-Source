#pragma once

#include <util/system/types.h>
#include <util/stream/output.h>
#include <util/generic/vector.h>
#include <util/generic/buffer.h>
#include <util/memory/blob.h>
#include <util/stream/buffer.h>
#include <util/ysaveload.h>


template <class TItem>
class TIdToItemMappingBuilder {
public:
    TIdToItemMappingBuilder() = default;

    void AddItem(const TItem& item) {
        Items.push_back(item);
    }

    void Save(IOutputStream* output) const {
        ui64 numItems = Items.size();
        output->Write(&numItems, sizeof(ui64));

        TVector<TBufferOutput> serializedData;
        serializedData.reserve(numItems);
        ui64 offset = 0;
        for (const TItem& item : Items) {
            serializedData.emplace_back();
            auto& serializedItem = serializedData.back();
            output->Write(&offset, sizeof(ui64));
            ::Save(&serializedItem, item);
            offset += serializedItem.Buffer().Size();
        }
        output->Write(&offset, sizeof(ui64)); // dummy offset

        for (const auto& serializedItem : serializedData) {
            const auto& buffer = serializedItem.Buffer();
            output->Write(buffer.Data(), buffer.Size());
        }
    }

private:
    TVector<TItem> Items;
};


template <class TItem>
class TIdToItemMapping {
public:
    TIdToItemMapping(const TBlob& blob)
        : DataHolder(blob)
    {
        const char* dataStart = DataHolder.AsCharPtr();
        DataEnd = dataStart + DataHolder.Length();
        NumItems = *reinterpret_cast<const ui64*>(dataStart);
        Offsets = reinterpret_cast<const ui64*>(dataStart + sizeof(ui64));
        Items = reinterpret_cast<const char*>(Offsets + NumItems + /*dummy offset*/1);
    }

    TItem GetItem(const size_t id) const {
        Y_ASSERT(id < NumItems);
        const char* itemPos = Items + Offsets[id];
        TBuffer buffer(itemPos, Offsets[id + 1] - Offsets[id]);
        TBufferInput serializedData(buffer);
        TItem item;
        ::Load(&serializedData, item);
        return item;
    }

private:
    TBlob DataHolder;
    size_t NumItems;
    const ui64* Offsets;
    const char* Items;
    const char* DataEnd;
};
