package ru.yandex.alice.paskill.dialogovo.domain;

public class AvatarsNamespace {

    /**
     * Avatar namespace name (i.e. dialogs, dialogs-skill-card)
     */
    private final String name;

    public static final AvatarsNamespace DIALOGS = new AvatarsNamespace("dialogs");
    public static final AvatarsNamespace DIALOGS_SKILL_CARD = new AvatarsNamespace("dialogs-skill-card");

    private AvatarsNamespace(String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

    public String getUrlPath() {
        return "get-" + name;
    }

    public static AvatarsNamespace createCustom(String name) {
        return new AvatarsNamespace(name);
    }

}
