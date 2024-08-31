import { action, observable } from 'mobx';
import { ICustomerActivation, ICustomerApi } from '../lib/customer-api';
import { sleep } from '../lib/utils';
import { IOperation } from '../model/operation';
import { CustomerDeviceListStore } from './customer-device-list';

export class ActivationDataStore {
    constructor(api: ICustomerApi, activationId?: string, deviceStore?: CustomerDeviceListStore) {
        this.api = api;
        this.activationId = activationId;
        this.deviceStore = deviceStore;
        this.update().catch(() => {});
    }

    private readonly api: ICustomerApi;
    private readonly activationId?: string;
    private readonly deviceStore?: CustomerDeviceListStore;

    @observable public activationData?: ICustomerActivation;
    @observable public firstLoad: boolean = false;
    @observable public loading: boolean = false;
    @observable public loadError: boolean = false;
    private _pending: boolean = false;
    private _updateIsQueued: boolean = false;

    @observable public isActivating: boolean = false;
    @action.bound
    public async activate(code: string, forCurrentUser: boolean = true) : Promise<[boolean, string]> {
        if (this.isActivating) {
            return [false, ''];
        }

        this.isActivating = true;

        try {
            let operationId;
            let kolonkishId = '';

            if (forCurrentUser) {
                operationId = await this.api.activateDevice(this.activationId ? undefined : code, this.activationId)
            } else {
                // TODO: remove if condition on next release
                const res = await this.api.activateDeviceForGuest(this.activationId ? undefined : code, this.activationId);
                if(Array.isArray(res)) {
                    [operationId, kolonkishId] = res;
                } else {
                    operationId = res
                }
            }
            await this.operationAwaiter(operationId);

            setImmediate(() => {
                this.update().catch(() => {});
                this.deviceStore?.update().catch(() => {});
            });

            return [true, kolonkishId];
        } finally {
            this.isActivating = false;
        }
    }

    @action.bound
    public async update() {
        if (this._pending) {
            this._updateIsQueued = true;
            return;
        }

        this.loading = true;
        this._pending = true;
        this._updateIsQueued = false;

        try {
            if (this.activationId) {
                this.activationData = await this.api.getDevicesByActivationId(this.activationId);
            }
            if (!this.firstLoad) {
                this.firstLoad = true;
            }

            if (this.loadError) {
                this.loadError = false;
            }
        } catch (e) {
            if (!this.loadError) {
                this.loadError = true;
            }
        }
        this.loading = false;

        setTimeout(() => {
            this._pending = false;

            if (this._updateIsQueued) {
                this.update().catch(() => {});
            }
        }, this.api.pollingInterval);
    }

    private async operationAwaiter(operationId: string) {
        let status: IOperation['status'];
        do {
            await sleep(this.api.pollingInterval);
            status = await this.api.getOperationStatus(operationId);
        } while (status === 'pending');

        if (status === 'rejected') {
            throw new Error('Operation failed');
        }
    }
}
