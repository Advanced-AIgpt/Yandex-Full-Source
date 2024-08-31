import commander from 'commander';
import { getConnectOrganizationsChoiceList, makeAChoice, withErrors } from '../../utils';
import config from '../../../lib/config';
import Connect from '../../../services/connect';
import inquirer from 'inquirer';
import ip from 'ip';

export default (program: commander.Command) => {
    program
        .command('connect:domain')
        .description('Add technical domain to Connect organization')
        .action(
            withErrors(async () => {
                const orgId = (await makeAChoice(
                    await getConnectOrganizationsChoiceList(),
                    'Choose Connect organization to add technical domain',
                    'autocomplete',
                )) as number;

                const { admin_id: uid, name } = await Connect.getOrganization(
                    orgId,
                    ['name', 'admin_id'],
                    { tvmSrc: 'connect-controller' },
                );

                const techDomain = (await Connect.getDomains(orgId, ['name'], {
                    tvmSrc: 'connect-controller',
                })).find((item) => item.name.endsWith(config.connect.techDomainSuffix));
                if (techDomain) {
                    console.log(
                        `Connect organization "${name}" (${orgId}) has technical domain: ${
                            techDomain.name
                        }`,
                    );

                    return;
                }

                const { newDomainName } = await inquirer.prompt([
                    {
                        type: 'string',
                        message: `OK, you can add technical domain for Connect organization "${name}" (${orgId}). Technical domain must have suffix ${
                            config.connect.techDomainSuffix
                        }.\nInput domain name:`,
                        name: 'newDomainName',
                        filter(input) {
                            input = input.replace(/\s+/g, '');
                            input = input.replace(/\.$/, '');
                            input = input.endsWith(config.connect.techDomainSuffix)
                                ? input
                                : input + config.connect.techDomainSuffix;
                            input = input.replace(/\.{2,}/g, '.');
                            input = input.replace(/^\./, '');
                            input = input.replace(/\.$/, '');

                            return input;
                        },
                        validate(input) {
                            return input.trim().length > 0;
                        },
                    },
                ]);

                await Connect.createDomain(
                    orgId,
                    { uid: uid!, ip: ip.address() },
                    { name: newDomainName },
                    { tvmSrc: 'connect-controller' },
                );
            }),
        );
};
