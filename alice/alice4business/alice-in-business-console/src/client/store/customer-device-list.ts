import { ICustomerActivation, ICustomerApi, ICustomerDevice } from '../lib/customer-api';
import { action, computed, observable } from 'mobx';
import { sleep } from '../lib/utils';
import { IOperation } from '../model/operation';

export interface ICustomerDeviceListItem extends ICustomerDevice {
    busy?: boolean;
}

export class CustomerDeviceListStore {
    constructor(api: ICustomerApi) {
        this.api = api;
    }

    private readonly api: ICustomerApi;

    @observable private _devices: Map<string, ICustomerDeviceListItem> = new Map();
    @computed
    public get devices(): ICustomerDevice[] {
        return Array.from(this._devices.values()).sort((a, b) =>
            a.deviceId.toLowerCase().localeCompare(b.deviceId.toLowerCase()),
        );
    }

    @observable public isDeviceActivating: boolean = false;
    @action.bound
    public async activateDevice(code: string, forCurrentUser: boolean = true) {
        if (this.isDeviceActivating) {
            return false;
        }

        this.isDeviceActivating = true;

        try {
            const res = forCurrentUser
                ? await this.api.activateDevice(code)
                : await this.api.activateDeviceForGuest(code);

            // TODO: remove if condition on next release
            const operationId = Array.isArray(res) ? res[0] : res;
            await this.operationAwaiter(operationId);

            setImmediate(() => {
                this.update().catch(() => {});
            });

            return true;
        } finally {
            this.isDeviceActivating = false;
        }
    }

    @observable public isDeviceReseting: boolean = false;
    @action.bound
    public async reset(deviceId: string) {
        if (!this._devices.has(deviceId) || this.isDeviceReseting) {
            return;
        }

        const device = this._devices.get(deviceId)!;
        try {
            device.busy = true;
            this.isDeviceReseting = true;

            if (!device.pendingOperation) {
                device.pendingOperation = await this.api.resetDevice(
                    device.deviceId,
                    device.platform,
                );
            }
        } catch(e) {
            this.isDeviceReseting = false;
            throw e;
        } finally {
            device.busy = false;
        }

        try {
            await this.resetAwaiter(device);
        } finally {
            device.pendingOperation = undefined;
            this.isDeviceReseting = false;
        }
    }
    private _resetPromises: Map<string, Promise<void>> = new Map();
    private async resetAwaiter(device: ICustomerDevice) {
        if (!device.pendingOperation) {
            this._resetPromises.delete(device.deviceId);
            return;
        }

        if (!this._resetPromises.has(device.deviceId)) {
            this._resetPromises.set(
                device.deviceId,
                this.operationAwaiter(device.pendingOperation).finally(() => {
                    setImmediate(() => {
                        this.update().catch(() => {});
                    });
                }),
            );
        }

        await this._resetPromises.get(device.deviceId);
    }

    @observable public firstLoad: boolean = false;
    @observable public loading: boolean = false;
    @observable public loadError: boolean = false;
    private _pending: boolean = false;
    private _updateIsQueued: boolean = false;
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
            const devices = await this.api.getDevices();
            this._devices.clear();
            for (const device of devices) {
                this._devices.set(device.deviceId, device);

                this.resetAwaiter(device).catch(() => {});
            }

            this._resetPromises.forEach((_, deviceId) => {
                if (!this._devices.has(deviceId)) {
                    this._resetPromises.delete(deviceId);
                }
            });

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
