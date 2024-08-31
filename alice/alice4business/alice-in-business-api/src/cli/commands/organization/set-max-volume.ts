import commander from 'commander';
import {
    askForInput,
    getOrganizationsChoiceList,
    makeAChoice,
    withErrors,
} from '../../utils';
import {Organization} from '../../../db';

export default (program: commander.Command) => {
    program
        .command('organizations:set-max-station-volume')
        .description('Set max station volume value')
        .action(
            withErrors(async () => {
                const organizationId = await makeAChoice(
                    await getOrganizationsChoiceList(),
                    'Choose organization',
                    'autocomplete',
                );

                const organization = await Organization.findByPk(organizationId);
                if (!organization) {
                    return;
                }

                const additionalValidation = (value: string, label: string) => {
                    const numValue = Number(value)
                    if(Number.isNaN(numValue)){
                        return `${label} is not a number`
                    } else if (numValue < 0 || numValue > 10) {
                        return 'New value should be in range 0-10'
                    }
                    return false
                }

                const newMaxStationVolume = await askForInput('New max station volume value', true, additionalValidation);

                await organization.update({maxStationVolume: newMaxStationVolume});
            }),
        );
};
