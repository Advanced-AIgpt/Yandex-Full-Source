import commander from 'commander';
import { askForInput, makeAChoice, withErrors } from '../../utils';
import Connect from '../../../services/connect';
import { Organization } from '../../../services/connect/organizations';
import { ConnectOrganization } from '../../../db';
import { ConnectOrganizationSchema } from '../../../db/tables/connectOrganization';

export default (program: commander.Command) => {
    program
        .command('connect:add-whois')
        .description('Add Connect organization via who-is')
        .action(
            withErrors(async () => {
                const type = await makeAChoice(
                    ['email', 'domain'],
                    'Choose query method',
                );
                const search = await askForInput('Value to search');

                const result = await Connect.whois(
                    { [type]: search },
                    { tvmSrc: 'connect-controller' },
                );

                const organizations = (await makeAChoice(
                    (
                        await Promise.all(
                            result.map((item) =>
                                Connect.getOrganization(item.org_id, ['id', 'name'], {
                                    tvmSrc: 'connect-controller',
                                }),
                            ),
                        )
                    ).map((org) => ({
                        name: `${org.id}: ${org.name}`,
                        value: org,
                    })),
                    'Choose Connect organizations to add',
                    'checkbox',
                )) as Pick<Organization, 'id' | 'name'>[];

                await ConnectOrganization.bulkCreate(
                    organizations.map((item) => ({
                        id: item.id,
                        name: item.name,
                    })) as ConnectOrganizationSchema[],
                    { ignoreDuplicates: true },
                );
            }),
        );
};
