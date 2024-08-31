import { mediaGotWithTvm } from './utils';

export interface PlusRequest {
    userId: string;
    clientIp: string;
    promoCode: string;
    language?: 'ru' | 'en';
}

// https://wiki.yandex-team.ru/passport/python/api/bundle/plus/#statusaktivaciipodarochnogokodaconsume-status
export enum PlusResponseStatus {
    Success = 'success',
    CodeNotExists = 'code-not-exists',
    CodeAlreadyConsumed = 'code-already-consumed',
    CodeExpired = 'code-expired',
    UserTemporaryBanned = 'user-temporary-banned',
    FailedToCreatePayment = 'failed-to-create-payment',
    UserHasTemporaryCampaignRestrictions = 'user-has-temporary-campaign-restrictions',
    CodeOnlyForNewUsers = 'code-only-for-new-users',
    CodeOnlyForWeb = 'code-only-for-web',
    CodeOnlyForMobile = 'code-only-for-mobile',
    AllowBillingProduct = 'allow-billing-product',
    CodeNotAllowedInCurrentRegion = 'code-not-allowed-in-current-region',
    CodeCantBeConsumed = 'code-cant-be-consumed',
}

export interface PlusResponse {
    result: {
        status: PlusResponseStatus;
        orderId?: string;
        statusDesc?: string;
        givenDays?: string;
        accountStatus?: object;
    };
    invocationInfo?: object;
}

export const plusRequest = async (options: PlusRequest) => {
    return mediaGotWithTvm<PlusResponse>('/billing/promo-code/consume', {
        method: 'post',
        query: {
            __uid: options.userId,
            ip: options.clientIp,
            code: options.promoCode,
            language: options.language || 'en',
        },
    });
};
