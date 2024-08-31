import { action, computed, observable } from 'mobx';
import API, { ApiError } from '../lib/api';
import { IConsoleApi } from '../lib/console-api';
import { searchSubStr } from '../lib/utils';
import { IDevice, IDeviceCreation, Status } from '../model/device';
import { DeviceStore } from './device';
import bind from 'bind-decorator';

export class DeviceListStore {
    constructor(organizationId: string, api: IConsoleApi) {
        this.organizationId = organizationId;
        this.api = api;
    }

    public readonly organizationId: string;

    @observable public devices: Map<string, DeviceStore> = new Map();
    @observable public searchFilter: string = '';
    @observable public categoryFilter: Status | 'offline' | null = null;
    @observable public orderAttr: keyof IDevice = 'status';
    @observable public orderType: 'ascending' | 'descending' = 'ascending';

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
            const devices = await this.api.fetchDeviceList(this.organizationId);
            this.devices.clear();
            for (const device of devices) {
                this.devices.set(device.id, new DeviceStore(device, this.api));
            }
            this.loaded = true;
            this.loadError = undefined;
        } catch (e) {
            console.warn('Failed to fetch devices', e);

            if (e instanceof ApiError && e.code === 403) {
                this.loadError = 'Нет доступа к устройствам';
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

    @bind
    private sortFn(d1: IDevice, d2: IDevice) {
        const { orderAttr, orderType } = this;
        if (orderType === 'ascending') {
            return d1[orderAttr]! > d2[orderAttr]! ? -1 : 1;
        }
        return d1[orderAttr]! > d2[orderAttr]! ? 1 : -1;
    }

    @bind
    private searchFilterFn({ note, externalDeviceId, deviceId, id, room }: IDevice) {
        return (
            searchSubStr(id, this.searchFilter) ||
            searchSubStr(deviceId, this.searchFilter) ||
            searchSubStr(externalDeviceId || '', this.searchFilter) ||
            searchSubStr(note || '', this.searchFilter) ||
            searchSubStr(room?.id || '', this.searchFilter)
        );
    }

    @bind
    private categoryFilterFn(device: IDevice) {
        return (
            this.categoryFilter === null ||
            device.status === this.categoryFilter ||
            (this.categoryFilter === Status.Inactive && device.status === Status.Reset) ||
            (this.categoryFilter === 'offline' && !device.online)
        );
    }

    @computed
    public get visibleList(): DeviceStore[] {
        return Array.from(this.devices.values())
            .filter(this.searchFilterFn)
            .filter(this.categoryFilterFn)
            .sort(this.sortFn);
    }

    @action.bound
    public async createDevice(form: IDeviceCreation) {
        form.externalDeviceId || (form.externalDeviceId = undefined);
        form.note || (form.note = undefined);

        const device = await this.api.createDevice(form, this.organizationId);
        this.devices.set(device.id, new DeviceStore(device, this.api));
    }

    @action.bound
    public async removeDevice(id: string) {
        await this.api.removeDevice(id);
        this.devices.delete(id);
    }

    @action.bound
    public async updateDevice(id: string) {
        try {
            const device = await this.api.fetchDevice(id);
            this.devices.set(device.id, new DeviceStore(device, this.api));
        } catch (e) {
            if (API.isError(e) && e.code === 404) {
                this.devices.delete(id);
            } else {
                throw e;
            }
        }
    }
}
