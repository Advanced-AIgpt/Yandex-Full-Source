import { v4 as uuidv4 } from 'uuid';
import uniqueString from 'unique-string';
import { IncomingHttpHeaders } from 'http';
import { DeviceSchema, Platform } from '../../db/tables/device';
import {
    ActivatePayload,
    OperationSchema,
    Scope as OperationScope,
    Status as OperationStatus,
} from '../../db/tables/operation';
import { KolonkishResponse } from '../../services/passport/kolonkish';
import { DeviceInfo } from '../../services/quasar/info';
import { PlusResponse, PlusResponseStatus } from '../../services/media/ya-plus';
import * as ACL from '../../lib/acl';
import { PromoCodeSchema } from '../../db/tables/promoCode';
import { OrganizationSchema } from '../../db/tables/organization';
import { RoomSchema } from '../../db/tables/room';

const data = {
    uniqueString,
    randomUid: () => 4000000000 + Math.ceil(Math.random() * 4000000000),
    uuid: uuidv4,
    headers: {} as IncomingHttpHeaders,
    user: {
        ip: '0.0.0.127',
        uid: 4000242097,
        login: Promise.resolve(uniqueString()),
    } as ACL.User,
    userOAuthToken: 'AQAAAADubtmxAAAO7LsvJKeqJEGFki60RJmPqxE',
    organization: {
        id: uuidv4(),
        name: 'TEST_ORG',
    } as OrganizationSchema,
    promocode: {
        id: uuidv4(),
        code: uniqueString(),
        get organizationId(): string {
            return data.organization.id;
        },
        get userId(): string {
            return data.kolonkish.uid;
        },
    } as PromoCodeSchema,
    kolonkish: {
        login: uniqueString(),
        uid: uniqueString(),
    },
    device: {
        get id() {
            return uuidv4();
        },
        get organizationId(): string {
            return data.organization.id;
        },
        get externalDeviceId() {
            return uniqueString();
        },

        get kolonkishLogin() {
            return uniqueString();
        },
        get kolonkishId() {
            return uniqueString();
        },

        platform: Platform.YandexStation,
        get deviceId() {
            return uniqueString();
        },

        lastSyncUpdate: new Date(),
    } as DeviceSchema,
    room: {
        get id() {
            return uuidv4();
        },
        get organizationId(): string {
            return data.organization.id;
        },
        get externalRoomId() : string {
            return uniqueString();
        },
        get name() : string {
            return uniqueString();
        }
    } as RoomSchema,
    operation: {
        id: uuidv4(),
        status: OperationStatus.Pending,
        payload: {
            // TODO: payload only for reset
            get kolonkishUid(): string {
                return data.kolonkish.uid;
            },
            get kolonkishLogin(): string {
                return data.kolonkish.login;
            },
            clientIp: '0.0.0.127',
            shouldActivatePromoCode: false,
        } as ActivatePayload,
        scope: {
            context: 'ext' as 'ext',
            headers: {} as IncomingHttpHeaders,
            userLogin: uniqueString(),
            userId: uniqueString(),
        } as OperationScope,
        get devicePk(): string {
            return data.device.id!;
        },
    } as OperationSchema,
    response: {
        registryKolonkish(props: Partial<KolonkishResponse> = {}) {
            return {
                status: 'ok',
                code: uniqueString(),
                get uid() {
                    return data.kolonkish.uid;
                },
                get login() {
                    return data.kolonkish.login;
                },
                ...props,
            } as KolonkishResponse;
        },
        getDeviceInfo(props: Partial<DeviceInfo> = {}) {
            return {
                status: 'ok',
                device: {
                    status: 'online',
                    get owner_uid() {
                        return data.device.kolonkishId;
                    },
                },
                ...props,
            } as DeviceInfo;
        },
        plusRequest(resStatus?: PlusResponseStatus) {
            return {
                result: {
                    status: resStatus || PlusResponseStatus.Success,
                },
            } as PlusResponse;
        },
    },
};

export default data;
