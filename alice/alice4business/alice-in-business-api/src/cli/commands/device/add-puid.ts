import commander from 'commander';
import {
    askForInput,
    getDevicesChoiceList,
    makeAChoice,
    withErrors,
} from '../../utils';
import { Device } from '../../../db';

export default (program: commander.Command) => {
    program
        .command('device:add-puid')
        .description('Add smart home uid to device')
        .action(
            withErrors(async () => {
                const deviceId = await makeAChoice(
                    await getDevicesChoiceList(),
                    'Choose a device',
                    'autocomplete'
                );

                const additionalValidation = (value: string, label: string) => {
                    if(Number.isNaN(Number(value))){
                        return `${label} is not a number`
                    }
                    return false
                }

                const puid = await askForInput('PUID', true, additionalValidation);

                if (deviceId && puid){
                    await Device.update({ smartHomeUid: puid }, { where: { id: deviceId } });
                } else {
                    throw Error
                }
            }),
        );
};
