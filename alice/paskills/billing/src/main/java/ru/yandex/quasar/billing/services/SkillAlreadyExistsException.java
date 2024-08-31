package ru.yandex.quasar.billing.services;

import ru.yandex.quasar.billing.exception.ConflictException;

public class SkillAlreadyExistsException extends ConflictException {
    public SkillAlreadyExistsException() {
        super("Skill already exists");
    }

    public SkillAlreadyExistsException(Throwable cause) {
        super("Skill already exists", cause);
    }
}
