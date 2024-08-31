import { action, computed, observable } from 'mobx';
import { IConsoleApi } from '../lib/console-api';
import { sleep } from '../lib/utils';
import { IDevice, Platform, Status } from '../model/device';
import { IDeviceRoom } from '../model/room';
import { RoomStore } from './room';
import { RoomListStore } from './room-list';

export class DeviceStore implements IDevice {
    constructor(device: IDevice, api: IConsoleApi) {
        this.id = device.id;
        this.organizationId = device.organizationId;
        this.platform = device.platform;
        this.deviceId = device.deviceId;
        this.externalDeviceId = device.externalDeviceId || '';
        this.note = device.note || '';
        this.status = device.status;
        this.agreementAccepted = device.agreementAccepted;
        this.online = device.online;
        this.isActivatedByCustomer = device.isActivatedByCustomer;
        this.hasPromo = device.hasPromo;
        this.pendingOperation = device.pendingOperation;
        this.room = device.room;
        this.api = api;
    }

    public id: string;
    public organizationId: string;
    public platform: Platform;
    public deviceId: string;
    @observable public externalDeviceId: string;
    @observable public note: string;
    @observable public status: Status;
    @observable public agreementAccepted: boolean;
    @observable public online: boolean;
    @observable public isActivatedByCustomer?: boolean;
    @observable public hasPromo: boolean;
    @observable public pendingOperation?: string;
    @observable public room?: IDeviceRoom;

    private readonly api: IConsoleApi;
    @observable private _pending: boolean = false;
    @computed
    public get pending() {
        return this._pending || Boolean(this.pendingOperation);
    }

    @action.bound
    public async activatePromo() {
        if (this._pending) {
            throw new Error('Устройство занято');
        }
        this._pending = true;
        try {
            await this.api.activatePromoForDevice(this.id);
            this.hasPromo = true;
        } finally {
            this._pending = false;
        }
    }

    @action.bound
    public async activateDevice() {
        if (this._pending) {
            throw new Error('Устройство занято');
        }
        this._pending = true;
        try {
            await this.api.activateDevice(this.id);
            this.status = Status.Active;
            await sleep(this.api.pollingInterval);
        } finally {
            this._pending = false;
        }
    }

    @action.bound
    public async resetDevice() {
        if (this._pending) {
            throw new Error('Устройство занято');
        }
        this._pending = true;
        try {
            const res = await this.api.resetDevice(this.id);
            this.hasPromo = false;
            this.status = Status.Reset;
            await this.operationAwaiter(res.operationId);
            this.status = Status.Inactive;
            await sleep(this.api.pollingInterval);
        } finally {
            this._pending = false;
        }
    }

    @action.bound
    public async editDeviceNote(note: string) {
        if (this._pending) {
            throw new Error('Устройство занято');
        }
        this._pending = true;
        try {
            await this.api.editDeviceNote(this.id, note);
            this.note = note;
        } finally {
            this._pending = false;
        }
    }

    @action.bound
    public async editDeviceExternalId(DeviceExternalId: string) {
        if (this._pending) {
            throw new Error('Устройство занято');
        }
        this._pending = true;
        try {
            await this.api.editDeviceExternalId(this.id, DeviceExternalId);
            this.externalDeviceId = DeviceExternalId;
        } finally {
            this._pending = false;
        }
    }

    @action.bound
    public async editDeviceRoom(DeviceRoom: IDeviceRoom | null) {
        if (this._pending) {
            throw new Error('Устройство занято');
        }
        this._pending = true;
        try {
            await this.api.editDeviceRoom(this.id, DeviceRoom?.id || null);
            this.room = DeviceRoom || undefined;
        } finally {
            this._pending = false;
        }
    }

    private async operationAwaiter(operationId: string) {
        let status: string;
        do {
            await sleep(this.api.pollingInterval);
            status = await this.api
                .getOperationStatus(operationId)
                .then(({ operation }) => operation.status);
        } while (status === 'pending');

        if (status === 'rejected') {
            throw new Error('Не удалось активировать устройство');
        }
    }
}
