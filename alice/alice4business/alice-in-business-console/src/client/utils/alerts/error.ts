import API, { ApiError } from '../../lib/api';

export const createErrorMessage = (error: ApiError, message: string = '') => {
    const { payload, fields } = error;
    if (payload) {
        const disc = payload.text;
        if (disc) {
            return `${disc}. ${message ? message : 'Повторите попытку позже'}`;
        }
        if (payload.unique) {
            return `Значения полей ${Object.keys(fields).join(', ')} должны быть уникальными`;
        }
        if (payload.invalid) {
            return `Исправьте поля ${Object.keys(fields).join(', ')}`;
        }
    }
    if (error.code === 403) {
        return `Forbidden: вы должны быть авторизованны`;
    }
    if (API.is4xxError(error)) {
        return `Request Error: \u00a0 ${message}`;
    }
    if (API.is5xxError(error)) {
        return `Server Error: \u00a0 ${message}`;
    }
    return `Unexpected Error: \u00a0 ${message}`;
};
