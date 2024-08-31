import { Kolonkish } from './device';

export interface IOperation {
    id: string;
    date: string;
    kolonkish?: Kolonkish;
    type: 'activate' | 'reset' | 'promo-activate';
    status: 'resolved' | 'rejected' | 'pending';
    context?: 'ext' | 'int' | 'customer';
    deviceId: string;
    devicePk: string;
    userLogin?: string;
}
