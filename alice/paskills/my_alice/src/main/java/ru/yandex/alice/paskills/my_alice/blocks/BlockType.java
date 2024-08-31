package ru.yandex.alice.paskills.my_alice.blocks;

import java.util.List;

public enum BlockType {

    RECOMMENDER,
    STATION,
    MUSIC,
    SKILLS_GAME,
    SKILLS_NEW,
    COMPUTER_VISION;

    public static final List<BlockType> ORDERED = List.of(
            RECOMMENDER,
            STATION,
            MUSIC,
            SKILLS_GAME,
            COMPUTER_VISION,
            SKILLS_NEW
    );

    public static final List<BlockType> ORDERED_NO_PLUS = List.of(
            RECOMMENDER,
            STATION,
            SKILLS_GAME,
            COMPUTER_VISION,
            SKILLS_NEW,
            MUSIC
    );

}
