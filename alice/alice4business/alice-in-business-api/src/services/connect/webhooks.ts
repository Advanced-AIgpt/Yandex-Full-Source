import { connectGotWithTvm } from './utils';

export interface Webhook {
    service_id: any;
    id: number;
    url: string;
    event_names: string[];
    tvm_client_id: number | null;
}

export const getWebhook = async (hookId: number, options: object = {}) =>
    connectGotWithTvm<Webhook>(`/v11/webhooks/${hookId}/`, {
        ...options,
        method: 'get',
    });

export const getWebhooks = async (options: object = {}) =>
    connectGotWithTvm.paginatedResult<Webhook>('/v11/webhooks/', {
        ...options,
        method: 'get',
    });

export const createWebhook = async (webhook: Partial<Webhook>, options: object = {}) =>
    connectGotWithTvm<Webhook>(`/v11/webhooks/`, {
        ...options,
        method: 'post',
        body: webhook,
    });

export const deleteWebhook = async (hookId: number, options: object = {}) =>
    connectGotWithTvm<void>(`/v11/webhooks/${hookId}/`, {
        ...options,
        method: 'delete',
    });
