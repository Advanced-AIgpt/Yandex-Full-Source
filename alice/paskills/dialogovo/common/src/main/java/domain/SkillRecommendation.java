package ru.yandex.alice.paskill.dialogovo.domain;

public class SkillRecommendation {
    private final SkillInfo skill;
    private final String activation;
    private final String logoAvatarUrl;

    public SkillRecommendation(SkillInfo skill, String activation, String logoAvatarUrl) {
        this.skill = skill;
        this.activation = activation;
        this.logoAvatarUrl = logoAvatarUrl;
    }


    public SkillInfo getSkill() {
        return this.skill;
    }

    public String getActivation() {
        return this.activation;
    }

    public String getLogoAvatarUrl() {
        return this.logoAvatarUrl;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        SkillRecommendation that = (SkillRecommendation) o;

        if (!skill.equals(that.skill)) {
            return false;
        }
        if (!activation.equals(that.activation)) {
            return false;
        }
        return logoAvatarUrl.equals(that.logoAvatarUrl);
    }

    @Override
    public int hashCode() {
        int result = skill.hashCode();
        result = 31 * result + activation.hashCode();
        result = 31 * result + logoAvatarUrl.hashCode();
        return result;
    }

    @Override
    public String toString() {
        return "SkillRecommendation{" +
                "skill=" + skill +
                ", activation='" + activation + '\'' +
                ", logoAvatarUrl='" + logoAvatarUrl + '\'' +
                '}';
    }
}
