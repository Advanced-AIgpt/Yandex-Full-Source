export enum Message {
    invalidProps = 'Неверные входные параметры.',
    nonexistRole = 'Нет такой роли.',
    nonexistPassportLogin = 'Неизвестный passport login.',
    blackboxAccessProblems = 'Проблемы с доступом к blackbox.',
    databaseProblems = 'Нет связи с базой данных.',
    alreadyHaveRole = 'Пользователь уже имеет эту роль.',
    alreadyRemovedRole = 'У сотрудника уже нет такого доступа.',
}

export interface PostRequest {
    login: string;
    role: string;
    fields: string;
}

export class RoleError extends Error {
    constructor(public status: number, public message: Message, public errorType: 'error' | 'fatal' | 'warning') {
        super();
    }
}
