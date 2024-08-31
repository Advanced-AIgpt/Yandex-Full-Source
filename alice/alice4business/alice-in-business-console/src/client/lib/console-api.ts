import { History, IDevice, IDeviceCreation, IMyDevice } from '../model/device';
import { IOrganization } from '../model/organization';
import { serializeQueryParams } from './utils';
import { IOperation } from '../model/operation';
import API from './api';
import { IRoom, IRoomCreation } from '../model/room';

export interface IConsoleApi extends API {
    getMyDevices(): Promise<IMyDevice[]>;
    fetchDeviceList(organizationId: string): Promise<IDevice[]>;
    fetchDevice(id: string): Promise<IDevice>;
    createDevice(device: IDeviceCreation, organizationId: string): Promise<IDevice>;
    removeDevice(id: string): Promise<void>;
    activateDevice(id: string): Promise<SuccessResponse>;
    resetDevice(id: string): Promise<SuccessResponse>;
    editDeviceNote(id: string, note: string): Promise<void>;
    editDeviceExternalId(id: string, externalDeviceId: string): Promise<void>;
    editDeviceRoom(id: string, roomId: string | null): Promise<void>;
    getOperationStatus(operationId: string): Promise<OperationResponse>;
    getAllOrganizations(): Promise<IOrganization[]>;
    getOrganization(organizationId: string): Promise<IOrganization>;
    activatePromoForDevice(id: string): Promise<void>;
    getOrganizationHistory(id: string, next?: string, devicePk?: string): Promise<History>;
    getOrganizationRooms(organizationId: string): Promise<IRoom[]>;
    fetchRoom(id: string): Promise<IRoom>;
    resetRoom(id: string): Promise<SuccessResponse>;
    activateRoom(id: string): Promise<SuccessResponse>;
    createRoom(room: IRoomCreation, organizationId: string): Promise<IRoom>;
    removeRoom(id: string): Promise<SuccessResponse>;
    editRoomName(id: string, name: string): Promise<SuccessResponse>;
    editRoomExternalRoomId(id: string, externalRoomId: string): Promise<SuccessResponse>;
    activatePromoForRoom(id: string): Promise<void>;
}

interface SuccessResponse {
    status: 'ok';
    operationId: string;
}

interface OperationResponse {
    status: 'ok';
    operation: Pick<IOperation, 'status'>;
}

export default class ConsoleAPI extends API implements IConsoleApi {
    public getMyDevices = () => this.call<IMyDevice[]>('GET', `${this.rootUrl}/devices/my`);

    public fetchDeviceList = (organizationId: string) =>
        this.call<IDevice[]>('GET', `${this.rootUrl}/devices/list/${organizationId}`);

    public fetchDevice = (id: string) => this.call<IDevice>('GET', `${this.rootUrl}/devices/${id}`);

    public createDevice = (device: IDeviceCreation, organizationId: string) =>
        this.call<IDevice>('POST', `${this.rootUrl}/devices`, { device, organizationId });

    public removeDevice = (id: string) => this.call('DELETE', `${this.rootUrl}/devices/${id}`);

    public activateDevice = (id: string) =>
        this.call<SuccessResponse>('POST', `${this.rootUrl}/devices/${id}/activate`);

    public resetDevice = (id: string) =>
        this.call<SuccessResponse>('POST', `${this.rootUrl}/devices/${id}/reset`);

    public editDeviceNote = (id: string, note: string) =>
        this.call('POST', `${this.rootUrl}/devices/${id}/edit/note`, { note });

    public editDeviceExternalId = (id: string, externalDeviceId: string) =>
        this.call('POST', `${this.rootUrl}/devices/${id}/edit/external_device_id`, {
            externalDeviceId,
        });

    public editDeviceRoom = (id: string, roomId: string) =>
        this.call('POST', `${this.rootUrl}/devices/${id}/edit/room_id`, { roomId });

    public getOperationStatus = (operationId: string) =>
        this.call<OperationResponse>('GET', `${this.rootUrl}/operations/${operationId}`);

    public getAllOrganizations = () =>
        this.call<IOrganization[]>('GET', `${this.rootUrl}/organizations`);

    public getOrganization = (organizationId: string) =>
        this.call<IOrganization>('GET', `${this.rootUrl}/organizations/${organizationId}`);

    public activatePromoForDevice = (id: string) =>
        this.call('POST', `${this.rootUrl}/devices/${id}/promocode`);

    public getOrganizationHistory = (id: string, next?: string, devicePk?: string) =>
        this.call<History>(
            'GET',
            `${this.rootUrl}/organizations/${id}/history?${serializeQueryParams({
                next,
                devicePk,
            })}`,
        );

    public getOrganizationRooms = (organizationId: string) =>
        this.call<IRoom[]>('GET', `${this.rootUrl}/rooms/list/${organizationId}`);

    public fetchRoom = (id: string) => this.call<IRoom>('GET', `${this.rootUrl}/rooms/${id}`);

    public resetRoom = (id: string) =>
        this.call<SuccessResponse>('POST', `${this.rootUrl}/rooms/${id}/reset`);

    public activateRoom = (id: string) =>
        this.call<SuccessResponse>('POST', `${this.rootUrl}/rooms/${id}/activate`);

    public createRoom = (room: IRoomCreation, organizationId: string) =>
        this.call<IRoom>('POST', `${this.rootUrl}/rooms`, { room, organizationId });

    public removeRoom = (id: string) =>
        this.call<SuccessResponse>('DELETE', `${this.rootUrl}/rooms/${id}`);

    public editRoomExternalRoomId = (id: string, externalRoomId: string) =>
        this.call<SuccessResponse>('POST', `${this.rootUrl}/rooms/${id}/edit/external_room_id`, {
            externalRoomId,
        });

    public editRoomName = (id: string, name: string) =>
        this.call<SuccessResponse>('POST', `${this.rootUrl}/rooms/${id}/edit/name`, { name });

    public activatePromoForRoom = (id: string) =>
        this.call('POST', `${this.rootUrl}/rooms/${id}/promocode`);
}
