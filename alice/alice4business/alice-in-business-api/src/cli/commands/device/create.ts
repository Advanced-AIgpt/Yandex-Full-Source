import commander from 'commander';
import {
    askForInput,
    getOrganizationsChoiceList,
    makeAChoice,
    withErrors,
} from '../../utils';
import { Device } from '../../../db';
import { DeviceSchema, Platform, Status } from '../../../db/tables/device';

export default (program: commander.Command) => {
    program
        .command('devices:create')
        .description('Create device')
        .action(
            withErrors(async () => {
                const platform = (await makeAChoice(
                    Object.values(Platform),
                    'Platform',
                    'autocomplete',
                )) as Platform;
                const deviceId = await askForInput('Device id');
                const externalDeviceId = await askForInput('External device Id', false);
                const note = await askForInput('Note', false);
                const status = (await makeAChoice(
                    ['reset', 'inactive', 'active'],
                    'Device status',
                    'autocomplete',
                )) as Status;
                const organizationId = await makeAChoice(
                    await getOrganizationsChoiceList(),
                    'Device owner organization',
                    'autocomplete',
                );
                const kolonkishId =
                    (await askForInput('Kolonkish Uid (null)', false)) || undefined;
                const kolonkishLogin =
                    (await askForInput('Kolonkish Login (null)', false)) || undefined;
                const d = await Device.create({
                    externalDeviceId,
                    note,
                    status,
                    organizationId,
                    deviceId,
                    platform,
                    kolonkishId,
                    kolonkishLogin,
                } as DeviceSchema);
                console.log(`Device id - ${d.id}`);
            }),
        );
};
