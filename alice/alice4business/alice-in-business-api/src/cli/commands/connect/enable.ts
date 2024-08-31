import commander from 'commander';
import ip from 'ip';
import { getConnectOrganizationsChoiceList, makeAChoice, withErrors } from '../../utils';
import config from '../../../lib/config';
import Connect from '../../../services/connect';

export default (program: commander.Command) => {
    program
        .command('connect:enable')
        .description(`Enable ${config.connect.selfSlug} service for Connect organization`)
        .action(
            withErrors(async () => {
                const orgIds = (await makeAChoice(
                    await getConnectOrganizationsChoiceList(),
                    'Choose Connect organizations to enable',
                    'checkbox',
                )) as number[];

                for (const orgId of orgIds) {
                    const { admin_id } = await Connect.getOrganization(
                        orgId,
                        ['admin_id'],
                        { tvmSrc: 'connect-controller' },
                    );

                    await Connect.enableService(
                        orgId,
                        { uid: admin_id!, ip: ip.address() },
                        { tvmSrc: 'connect-controller' },
                    );
                }
            }),
        );
};
