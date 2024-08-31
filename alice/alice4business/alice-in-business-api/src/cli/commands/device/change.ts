import commander from 'commander';
import {
    askForInput,
    getOrganizationsChoiceList,
    makeAChoice,
    withErrors,
} from '../../utils';
import { Device } from '../../../db';
import { DeviceSchema, Status } from '../../../db/tables/device';

export default (program: commander.Command) => {
    program
        .command('device:change')
        .description('Change device fields')
        .action(
            withErrors(async () => {
                const primaryId = await askForInput('id (primary id)');
                const deviceAttr = await Device.findOne().then(
                    (i) => (i as any).attributes,
                );
                const device = await Device.findByPk(primaryId, {
                    rejectOnEmpty: true,
                });

                const field = await makeAChoice(
                    deviceAttr,
                    'Choice device field',
                    'autocomplete',
                );

                let value: any;
                switch (field) {
                    case 'status':
                        value = (await makeAChoice(
                            ['active', 'inactive', 'offline'],
                            'Device status',
                        )) as Status;
                        break;
                    case 'organizationId':
                        value = await makeAChoice(
                            await getOrganizationsChoiceList(),
                            'Device owner (Organization)',
                            'autocomplete',
                        );
                        break;
                    default:
                        value = await askForInput('Input ' + field);
                        break;
                }
                await device!.update({ [field]: value } as DeviceSchema);
            }),
        );
};
