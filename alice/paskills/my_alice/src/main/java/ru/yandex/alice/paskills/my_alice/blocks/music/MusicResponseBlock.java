package ru.yandex.alice.paskills.my_alice.blocks.music;


import java.util.List;

import lombok.Data;

@Data
class MusicResponseBlock {

    private final String id;
    private final String title;
    private final String description;
    private List<BlockEntity> entities;

}
