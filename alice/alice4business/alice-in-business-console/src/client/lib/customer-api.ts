import { IDevice, IMyDevice } from '../model/device';
import API, { ApiParams } from './api';
import { rtrim } from './utils';
import { IOperation } from '../model/operation';

export interface ICustomerApi extends API {
    getDevices(): Promise<ICustomerDevice[]>;
    resetDevice(deviceId: string, platform: string): Promise<string>;
    activateDevice(activationCode?: string, activationId?: string): Promise<string>;
    getOperationStatus(operationId: string): Promise<IOperation['status']>;
    getDeviceByCode(activationCode: string): Promise<ICustomerDeviceInfo>;
    // TODO: remove return type on next release
    activateDeviceForGuest(activationCode?: string, activationId?: string): Promise<[string, string] | string>;
    applyPromocode(): Promise<'ok'>;
    getDevicesByActivationId(activationId: string): Promise<ICustomerActivation>;
}

export interface ICustomerDevice
    extends Pick<IDevice, 'platform' | 'deviceId' | 'pendingOperation'> {}

export interface ICustomerActivation {
    room: string | null;
    devices: ICustomerDevice[];
}

export interface ICustomerDeviceInfo
    extends Pick<IDevice, 'platform' | 'deviceId' | 'pendingOperation' | 'note'> {}

interface OperationResponse {
    status: 'ok';
    operation: Pick<IOperation, 'status'>;
}

export default class CustomerApi extends API implements ICustomerApi {
    public getDevices = () =>
        this.call<ICustomerDevice[]>('GET', `${this.rootUrl}/customer/devices`);

    public resetDevice = (deviceId: string, platform: string) =>
        this.call<string>('POST', `${this.rootUrl}/customer/reset`, {
            deviceId,
            platform,
        });

    public activateDevice = (activationCode?: string, activationId?: string) =>
        this.call<string>('POST', `${this.rootUrl}/customer/activate`, {
            code: activationCode,
            activationId,
        });

    public getOperationStatus = (operationId: string) =>
        this.call<OperationResponse>(
            'GET',
            `${this.rootUrl}/customer/guest/operations/${operationId}`,
        ).then((res) => res.operation.status);

    public getDeviceByCode = (activationCode: string) =>
        this.call<ICustomerDevice>(
            'GET',
            `${this.rootUrl}/customer/guest/devices/${activationCode}`,
        );

    public getDevicesByActivationId = (activationId: string) =>
        this.call<ICustomerActivation>(
            'GET',
            `${this.rootUrl}/customer/guest/activations/${activationId}`,
        );

    public activateDeviceForGuest = (activationCode?: string, activationId?: string) =>
        this.call<[string, string] | string>('POST', `${this.rootUrl}/customer/guest/activate`, {
            code: activationCode,
            activationId,
        });

    public applyPromocode = () => this.call<'ok'>('POST', `${this.rootUrl}/customer/promocode`);
}
