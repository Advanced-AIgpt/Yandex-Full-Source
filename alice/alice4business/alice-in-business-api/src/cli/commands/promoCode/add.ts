import commander from 'commander';
import { askForInput, getOrganizationsChoiceList, makeAChoice, withErrors } from '../../utils';
import inquirer from 'inquirer';
import { PromoCode } from '../../../db';
import { sendPush } from '../../../services/push/send';
import { PromoCodeSchema } from '../../../db/tables/promoCode';

export default (program: commander.Command) => {
    program
        .command('promocodes:add')
        .description('Add promocode')
        .action(
            withErrors(async () => {
                const organizationId = await makeAChoice(
                    await getOrganizationsChoiceList(),
                    'Organization owner',
                    'autocomplete',
                );

                const ticketKey = await askForInput('Ticket Key');

                const { input } = await inquirer.prompt<{ input: string }>([
                    {
                        type: 'editor',
                        name: 'input',
                        message: 'Put promo codes separated with [\\s\\n;,] into editor',
                    },
                ]);

                const codes = input
                    .split(/[\s\n;,]+/)
                    .filter(Boolean)
                    .map((code) => ({ code, organizationId, ticketKey }));

                await PromoCode.bulkCreate(codes as PromoCodeSchema[], {
                    ignoreDuplicates: true,
                });

                sendPush({
                    topic: organizationId,
                    event: 'organization-info',
                    payload: null,
                });
            }),
        );
};
