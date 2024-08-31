import { action, computed, observable } from 'mobx';
import API, { ApiError } from '../lib/api';
import { IConsoleApi } from '../lib/console-api';
import { IDeviceRoom, IRoomCreation } from '../model/room';
import { RoomStore } from './room';

export class RoomListStore {
    constructor(organizationId: string, api: IConsoleApi) {
        this.organizationId = organizationId;
        this.api = api;
    }

    public readonly organizationId: string;

    @observable public rooms: Map<string, RoomStore> = new Map();
    @observable public loaded: boolean = false;
    @observable public loadError?: string;

    private readonly api: IConsoleApi;
    private pending: boolean = false;
    private updateIsQueued: boolean = false;

    @action.bound
    public async update() {
        if (this.pending) {
            this.updateIsQueued = true;
            return;
        }

        this.pending = true;
        this.updateIsQueued = false;

        try {
            const rooms = await this.api.getOrganizationRooms(this.organizationId);
            this.rooms.clear();
            rooms.forEach((room) => {
                this.rooms.set(room.id, new RoomStore(room, this.api));
            });
            this.loaded = true;
            this.loadError = undefined;
        } catch (e) {
            console.warn('Failed to get rooms', e);
            if (e instanceof ApiError && e.code === 403) {
                this.loadError = 'Нет доступа к комнатам';
            } else {
                this.loadError = 'Произошла ошибка';
            }
        }

        setTimeout(() => {
            this.pending = false;
            if (this.updateIsQueued) {
                this.update().catch(() => {});
            }
        }, this.api.pollingInterval);
    }

    @action.bound
    public async updateRoom(id: string) {
        try {
            const room = await this.api.fetchRoom(id);
            this.rooms.set(room.id, new RoomStore(room, this.api));
        } catch (e) {
            if (API.isError(e) && e.code === 404) {
                this.rooms.delete(id);
            } else {
                throw e;
            }
        }
    }

    @action.bound
    public async createRoom(form: IRoomCreation) {
        form.externalRoomId || (form.externalRoomId = undefined);
        const room = await this.api.createRoom(form, this.organizationId);
        this.rooms.set(room.id, new RoomStore(room, this.api));
    }

    @action.bound
    public async removeRoom(id: string) {
        await this.api.removeRoom(id);
        this.rooms.delete(id);
    }

    @computed
    public get visibleList(): RoomStore[] {
        return Array.from(this.rooms.values());
    }

    public get deviceRooms(): IDeviceRoom[] {
        return Array.from(this.rooms.values(), (m) => ({ id: m.id, name: m.name }));
    }
}
