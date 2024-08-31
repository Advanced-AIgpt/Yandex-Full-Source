import Sequelize from 'sequelize';
import multer from 'multer';

export const renameValidationErrorFields = (
    error: Sequelize.ValidationError,
    fieldsMap: { [key: string]: string },
): Sequelize.ValidationError => {
    for (const errorItem of error.errors) {
        if (fieldsMap[errorItem.path]) {
            // TODO
            // errorItem.path = fieldsMap[errorItem.path];
        }
    }

    const fields: object = (error as any).fields || {};
    for (const [from, to] of Object.entries(fieldsMap)) {
        if (fields.hasOwnProperty(from)) {
            // @ts-ignore
            fields[to] = fields[from];
            // @ts-ignore
            delete fields[from];
        }
    }

    return error;
};
export const trimPrivateFields = (value: any) => {
    if (typeof value === 'object' && value !== null) {
        const res: typeof value = Array.isArray(value) ? [] : {};

        for (const [key, v] of Object.entries(value)) {
            if (key[0] === '_') {
                continue;
            }

            res[key] = trimPrivateFields(v);
        }

        return res;
    }

    return value;
};

export const isSequelizeError = (error: Error): error is Sequelize.Error => {
    return error instanceof Sequelize.Error;
};
export const isEmptyResultError = (error: Error): error is Sequelize.EmptyResultError => {
    return error instanceof Sequelize.EmptyResultError;
};
export const isSequelizeDatabaseError = (
    error: Error,
): error is Sequelize.DatabaseError => {
    return error instanceof Sequelize.DatabaseError;
};
export const isSequelizeValidationError = (
    error: Error,
): error is Sequelize.ValidationError => {
    return error instanceof Sequelize.ValidationError;
};
export const isSequelizeUniqueConstraintError = (
    error: Error,
): error is Sequelize.UniqueConstraintError => {
    return error instanceof Sequelize.UniqueConstraintError;
};
export const isSequelizeForeignKeyConstraintError = (
    error: Error,
): error is Sequelize.ForeignKeyConstraintError => {
    return error instanceof Sequelize.ForeignKeyConstraintError;
};

export const isMulterError = (error: Error) => {
    return (
        error instanceof (multer as any).MulterError ||
        error.message === 'Multipart: Boundary not found'
    );
};
