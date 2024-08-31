import { urlencoded, json, Router } from 'express';
import {
    tvmServiceTicketChecker,
    tvmUserTicketChecker,
    accessChecker
} from './lib/middlewares/authentication';
import config from './lib/config';
import * as ConnectOrganizations from './controllers/connect/organizations';
import * as ConnectResources from './controllers/connect/resources';
import * as ConsoleDevices from './controllers/console/devices';
import * as ConsoleRooms from './controllers/console/rooms';
import * as ConsoleOperations from './controllers/console/operations';
import * as ConsoleOrganizations from './controllers/console/organizations';
import * as Customer from './controllers/customer/customer';
import * as CustomerOperations from './controllers/customer/operations';
import * as Dialogovo from './controllers/dialogovo/dialogovo';
import * as Droideka from './controllers/droideka/droideka';
import * as PublicDevices from './controllers/public/devices';
import * as PublicRooms from './controllers/public/rooms';
import * as PublicOperations from './controllers/public/operations';
import * as Quasar from './controllers/quasar/quasar';
import * as SupportAccess from './controllers/support/access';
import * as SupportDevices from './controllers/support/devices';
import * as SupportOrganizations from './controllers/support/organizations';
import * as SupportPromocodes from './controllers/support/promocodes';
import * as SupportRooms from './controllers/support/rooms';
import * as SupportUsers from './controllers/support/users';
import IDM from './controllers/idm';
import { errorHandler } from './controllers/utils';

const consoleTvmMiddlewares = {
    serviceCheker: tvmServiceTicketChecker(
        new Set<number>([config.tvmtool.selfId]),
    ),
    userChecker: tvmUserTicketChecker(true),
    userGetter: tvmUserTicketChecker(false),
    accessChecker: accessChecker()
};

export default Router()
    .use(
        '/console',
        Router()
            .use(json())
            .use(
                // Апи для анонимного пользователя
                '/customer/guest',
                Router()
                    .use(consoleTvmMiddlewares.serviceCheker)
                    .use(consoleTvmMiddlewares.userGetter)
                    .get('/operations/:operationId', CustomerOperations.getOperation)
                    .get('/devices/:code', Customer.getDeviceByCodeHandler)
                    .get('/activations/:id', Customer.getDevicesByActivationIdHandler)
                    .post('/activate', Customer.activateForGuestHandler),
            )
            .use(
                '/customer',
                Router()
                    .use(consoleTvmMiddlewares.serviceCheker)
                    .use(consoleTvmMiddlewares.userChecker)
                    .get('/devices', Customer.getDevicesHandler)
                    .post('/reset', Customer.resetDeviceHandler)
                    .post('/activate', Customer.activateForCustomerHandler)
                    .post('/promocode', Customer.applyPromocodeForUser),
            )
            .use(
                '/devices',
                Router()
                    .use(consoleTvmMiddlewares.serviceCheker)
                    .use(consoleTvmMiddlewares.userChecker)
                    .get('/my', ConsoleDevices.getMyDeviceList)
                    .post('/', ConsoleDevices.createDeviceHandler)
                    .get('/:id', ConsoleDevices.getDeviceHandler)
                    .delete('/:id', ConsoleDevices.deleteDeviceHandler)
                    .get('/:id/agreement', ConsoleDevices.getDeviceAgreement)
                    .post('/:id/activate', ConsoleDevices.activateDeviceHandler)
                    .post('/:id/reset', ConsoleDevices.resetDeviceHandler)
                    .post('/:id/promocode', ConsoleDevices.activatePromoCodeHandler)
                    .post('/:id/edit/note', ConsoleDevices.editDeviceNote)
                    .post(
                        '/:id/edit/external_device_id',
                        ConsoleDevices.editDeviceExternalDeviceId,
                    )
                    .post('/:id/edit/room_id', ConsoleDevices.editDeviceRoomId,
                    )
                    .get('/list/:organizationId', ConsoleDevices.getDeviceList),
            )
            .use(
                '/rooms',
                Router()
                    .use(consoleTvmMiddlewares.serviceCheker)
                    .use(consoleTvmMiddlewares.userChecker)
                    .post('/', ConsoleRooms.createRoomHandler)
                    .get('/list/:organizationId', ConsoleRooms.getRoomList)
                    .get('/:id', ConsoleRooms.getRoomHandler)
                    .delete('/:id', ConsoleRooms.deleteRoomHandler)
                    .post('/:id/reset', ConsoleRooms.resetRoomHandler)
                    .post('/:id/promocode', ConsoleRooms.activatePromoCodeHandler)
                    .post('/:id/edit/name', ConsoleRooms.renameRoomHandler)
                    .post('/:id/edit/external_room_id', ConsoleRooms.editExternalRoomIdHandler)
                    .post('/:id/activate', ConsoleRooms.activateRoomHandler),
            )
            .use(
                '/operations',
                Router()
                    .use(consoleTvmMiddlewares.serviceCheker)
                    .use(consoleTvmMiddlewares.userChecker)
                    .get('/:id', ConsoleOperations.getOperationInfo),
            )
            .use(
                '/organizations',
                Router()
                    .use(consoleTvmMiddlewares.serviceCheker)
                    .use(consoleTvmMiddlewares.userChecker)
                    .get('/', ConsoleOrganizations.getAllOrganizations)
                    .get('/:id', ConsoleOrganizations.getOrganization)
                    .get('/:id/history', ConsoleOrganizations.getOrganizationHistory),
            )
            .use(errorHandler({ mode: 'wrap+payload' })),
    )
    .use(
        '/support',
        Router()
            .use(json())
            .use(consoleTvmMiddlewares.serviceCheker)
            .use(consoleTvmMiddlewares.userChecker)
            .use(consoleTvmMiddlewares.accessChecker)
            .use(
                '/access',
                Router()
                    .get('/', SupportAccess.getAccess) // Если доступа нет, то middleware вернёт ошибку
            )
            .use(
                '/devices',
                Router()
                    .get('/all', SupportDevices.getAllDevice)
                    .post('/addPuid', SupportDevices.addPuid)
                    .post('/create', SupportDevices.createDevice)
                    .post('/change', SupportDevices.changeDevice)
                    .get('/:id/defaults', SupportDevices.getDefaults)
            )
            .use(
                '/organizations',
                Router()
                    .get('/all', SupportOrganizations.getAllOrganizations)
                    .get('/:organizationId/devices', SupportOrganizations.getDevices)
                    .get('/:id/defaults', SupportOrganizations.getDefaults)
                    .get('/getConnect', SupportOrganizations.getConnectOrganizationsChoiceList)
                    .post('/create', SupportOrganizations.createOrganization)
                    .post('/change', SupportOrganizations.changeOrganization)
                    .post('/setMaxVolume', SupportOrganizations.setMaxVolume)
            )
            .use(
                '/promocodes',
                Router()
                    .post('/add', SupportPromocodes.addPromocode)
                    .post('/addToOrganization', SupportPromocodes.addPromocodeToOrganization)
            )
            .use(
                '/rooms',
                Router()
                    .post('/create', SupportRooms.createRoom)
            )
            .use(
                '/users',
                Router()
                    .get('/all', SupportUsers.getAllUsers)
                    .post('/create', SupportUsers.createUser)
                    .post('/bind', SupportUsers.bindUsers)
            )
    )
    .use(
        '/internal',
        Router()
            .use(
                '/connect',
                Router()
                    .use(
                        tvmServiceTicketChecker(
                            new Set<number>([config.connect.tvmId]),
                        ),
                    )

                    .get('/resources/', ConnectResources.getResourcesHandler)
                    // .post('/resources', ConnectResources.createResourceHandler)
                    // .get('/resources/:id', ConnectResources.createResourceHandler)
                    // .patch('/resources/:id', ConnectResources.updateResourceHandler)
                    // .delete('/resources/:id', ConnectResources.deleteResourceHandler)
                    .post('/sync', ConnectOrganizations.syncOrganizationHandler)

                    .use(errorHandler({ mode: 'wrap' })),
            )
            .use(
                '/dialogovo',
                Router()
                    .use(tvmServiceTicketChecker(new Set<number>(config.dialogovo.tvmId)))

                    .get('/devices', Dialogovo.getControlledDevices)
                    .get('/device_state', Dialogovo.getDeviceState)

                    .use(errorHandler({ mode: 'wrap' })),
            )
            .use(
                '/droideka',
                Router()
                    .use(tvmServiceTicketChecker(new Set<number>(config.droideka.tvmId)))
                    .get('/device_info', Droideka.getTVInfo)
                    .get('/devices', Droideka.getTVList)

                    .use(errorHandler({ mode: 'wrap' })),
            )
            .use(
                '/quasar',
                Router()
                    .get('/sub_state', Quasar.getSubscriptionState)
                    .get('/aux_config', Quasar.getAuxiliaryConfig)

                    .use(errorHandler({ mode: 'wrap' })),
            ),
    )
    .use(
        '/public',
        Router()
            .use(tvmServiceTicketChecker(null))
            .use(tvmUserTicketChecker(true))
            .use(json())
            .use(
                '/devices',
                Router()
                    .get('/info', PublicDevices.getDeviceInfoHandler)
                    .post('/activate', PublicDevices.activateDeviceHandler)
                    .post('/reset', PublicDevices.resetDeviceHandler)
                    .post('/promocode', PublicDevices.activatePromoCodeHandler)
                    .post('/activation', PublicDevices.createDeviceActivationHandler),
            )
            .use(
                '/rooms',
                Router()
                    .get('/info', PublicRooms.getRoomInfoHandler)
                    .post('/activate', PublicRooms.activateRoomHandler)
                    .post('/reset', PublicRooms.resetRoomHandler)
                    .post('/promocode', PublicRooms.activatePromoCodeHandler)
                    .post('/activation', PublicRooms.createRoomActivationHandler),
            )
            .use('/operations', Router().get('/', PublicOperations.getOperationInfo))

            .use(errorHandler({ mode: 'wrap' })),
    )
    .use(
        '/idm',
        Router()
            .use(urlencoded({
                extended: true
            }))
            .use(tvmServiceTicketChecker(new Set<number>(config.idm.tvmId)))
            .get('/info', IDM.getInfo)
            .get('/get-all-roles', IDM.getAllRoles)
            .post('/add-role', IDM.addRole)
            .post('/remove-role', IDM.removeRole),
    );
