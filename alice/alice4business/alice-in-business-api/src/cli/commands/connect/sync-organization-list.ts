import commander from 'commander';
import chalk from 'chalk';
import { withErrors } from '../../utils';
import Connect from '../../../services/connect';
import { ConnectOrganization } from '../../../db';

export default (program: commander.Command) => {
    program
        .command('connect:sync-organization-list')
        .description(`Sync Connect organization list`)
        .action(
            withErrors(async () => {
                const organizations = await Connect.getOrganizations(['id', 'name']);

                console.log(
                    chalk.white(`Got ${organizations.length} enabled organizations`),
                );

                let createCounter = 0;
                for (const organizationInfo of organizations) {
                    const [
                        organization,
                        created,
                    ] = await ConnectOrganization.findOrCreate({
                        where: { id: organizationInfo.id! },
                        defaults: { id: organizationInfo.id!, updatedAt: new Date(0) },
                    });

                    await organization.update(
                        {
                            name: organizationInfo.name,
                            active: true,
                        },
                        {
                            silent: true,
                        },
                    );

                    if (created) {
                        ++createCounter;
                    }
                }

                console.log(
                    chalk.white(
                        `ConnectOrganization list synced. Created: ${chalk.whiteBright(
                            createCounter.toString(),
                        )}, updated: ${organizations.length - createCounter}`,
                    ),
                );
            }),
        );
};
