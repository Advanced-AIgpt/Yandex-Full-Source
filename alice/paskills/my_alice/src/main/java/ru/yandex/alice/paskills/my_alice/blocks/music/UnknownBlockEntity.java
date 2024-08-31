package ru.yandex.alice.paskills.my_alice.blocks.music;

import lombok.Data;

@Data
class UnknownBlockEntity implements BlockEntity {

    @Override
    public String getId() {
        throw new UnsupportedOperationException();
    }

    @Override
    public String getImageUri() {
        throw new UnsupportedOperationException();
    }

    @Override
    public String getSuggest() {
        throw new UnsupportedOperationException();
    }

    @Override
    public String getTitle() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean isValid() {
        return false;
    }

}
