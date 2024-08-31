import commander from 'commander';
import { getConnectOrganizationsChoiceList, makeAChoice, withErrors } from '../../utils';
import { syncOrganization } from '../../../lib/sync/connect';

export default (program: commander.Command) => {
    program
        .command('connect:sync')
        .description(`Sync Connect organization`)
        .action(
            withErrors(async () => {
                const orgIds = (await makeAChoice(
                    await getConnectOrganizationsChoiceList(),
                    'Choose Connect organizations to sync',
                    'checkbox',
                )) as number[];

                for (const orgId of orgIds) {
                    await syncOrganization(orgId);
                }
            }),
        );
};
