import { action, observable } from 'mobx';
import { IOrganization } from '../model/organization';
import { IConnectOrganization } from '../model/connectOrganization';
import { IConsoleApi } from '../lib/console-api';

export class OrganizationStore implements IOrganization {
    constructor(id: string, api: IConsoleApi) {
        this.id = id;
        this.api = api;
    }

    public readonly id: string;
    @observable public name: string = '';
    @observable public promoCount: number = 0;
    @observable public connectOrganization?: IConnectOrganization;
    @observable public usesRooms: boolean = false;

    @observable public loaded: boolean = false;
    @observable public loadError?: Error;

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
            Object.assign(this, await this.api.getOrganization(this.id));
            this.loaded = true;
            this.loadError = undefined;
        } catch (e) {
            this.loadError = e.message;
            console.warn('Failed to fetch organization', e);
        }

        setTimeout(() => {
            this.pending = false;
            if (this.updateIsQueued) {
                this.update().catch(() => {});
            }
        }, this.api.pollingInterval);
    }
}
