package ru.yandex.alice.paskills.my_alice.blocks;

import lombok.Data;

import ru.yandex.alice.paskills.my_alice.layout.PageLayout;

@Data
public class Block {

    private final BlockType type;
    private final PageLayout.Group group;

}
