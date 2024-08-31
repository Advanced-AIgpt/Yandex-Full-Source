package ru.yandex.alice.paskills.my_alice.blocks.skills;

enum SkillTag {
    GAME("game"),
    NEW("new");

    private final String value;

    SkillTag(String value) {
        this.value = value;
    }

    public String getValue() {
        return value;
    }

}
