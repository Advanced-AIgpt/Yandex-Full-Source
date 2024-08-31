import { action, computed, observable } from 'mobx';
import { IConsoleApi } from '../lib/console-api';
import { sleep } from '../lib/utils';
import { Status } from '../model/device';
import { IRoom, OnlineStatus, PromoStatus } from '../model/room';

export class RoomStore implements IRoom {
    constructor(room: IRoom, api: IConsoleApi) {
        this.id = room.id;
        this.organizationId = room.organizationId;
        this.name = room.name;
        this.externalRoomId = room.externalRoomId || '';
        this.status = room.status;
        this.onlineStatus = room.onlineStatus;
        this.numDevices = room.numDevices;
        this.promoStatus = room.promoStatus;
        this.pendingOperation = room.pendingOperation;

        this.api = api;
    }

    public id: string;
    public organizationId: string;
    @observable public name: string;
    @observable public externalRoomId?: string | undefined;
    @observable public status: Status;
    @observable public onlineStatus: OnlineStatus;
    @observable public numDevices: number;
    @observable public promoStatus: PromoStatus;
    @observable public pendingOperation?: string;

    private readonly api: IConsoleApi;
    @observable private _pending: boolean = false;
    @computed
    public get pending() {
        return this._pending || Boolean(this.pendingOperation);
    }

    @action.bound
    public async resetRoom() {
        if (this._pending) {
            throw new Error('Комната занята');
        }
        this._pending = true;
        try {
            const res = await this.api.resetRoom(this.id);
            this.status = Status.Reset;
            await this.operationAwaiter(res.operationId);
            this.status = Status.Inactive;
            await sleep(this.api.pollingInterval);
        } finally {
            this._pending = false;
        }
    }

    @action.bound
    public async activateRoom() {
        if (this._pending) {
            throw new Error('Комната занята');
        }
        this._pending = true;
        try {
            const res = await this.api.activateRoom(this.id);
            await this.operationAwaiter(res.operationId);
            this.status = Status.Active;
            await sleep(this.api.pollingInterval);
        } finally {
            this._pending = false;
        }
    }

    @action.bound
    public async renameRoom(name: string) {
        if (this._pending) {
            throw new Error('Комната занята');
        }
        this._pending = true;
        try {
            await this.api.editRoomName(this.id, name);
            this.name = name;
        } finally {
            this._pending = false;
        }
    }

    @action.bound
    public async editExternalRoomId(externalRoomId: string) {
        if (this._pending) {
            throw new Error('Комната занята');
        }
        this._pending = true;
        try {
            await this.api.editRoomExternalRoomId(this.id, externalRoomId);
            this.externalRoomId = externalRoomId;
        } finally {
            this._pending = false;
        }
    }

    @action.bound
    public async activatePromo() {
        if (this._pending) {
            throw new Error('Комната занята');
        }
        this._pending = true;
        try {
            await this.api.activatePromoForRoom(this.id);
            this.promoStatus = PromoStatus.Applied;
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
            throw new Error('Не удалось сбросить устройства комнаты');
        }
    }
}
