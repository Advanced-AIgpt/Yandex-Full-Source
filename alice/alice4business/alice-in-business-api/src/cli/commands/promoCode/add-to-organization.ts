import { Op } from 'sequelize';
import commander from 'commander';
import chalk from 'chalk';
import { askForInput, getOrganizationsChoiceList, makeAChoice, withErrors } from '../../utils';
import { PromoCode } from '../../../db';

const pullOrganizationId = 'c8cba10c-97fb-40f1-8c36-a950ceed14bd';

export default (program: commander.Command) => {
    program
        .command('promocodes:add-to-organization')
        .description('Add promocodes to organization')
        .action(
            withErrors(async () => {
                const organizationId = await makeAChoice(
                    await getOrganizationsChoiceList(),
                    'Organization owner',
                    'autocomplete',
                );

                const additionalValidation = (value: string, label: string) => {
                    if(Number.isNaN(Number(value))){
                        return `${label} is not a number`;
                    }
                    return false;
                }

                const codesNum = Number(await askForInput('Number of codes', true, additionalValidation))

                const pullPromocodes = (await PromoCode.findAll(
                    {
                        where: { organizationId: pullOrganizationId },
                        attributes: ['id'],
                        limit: codesNum,
                    })).map((pullPromocode) => pullPromocode.id);

                if (codesNum > pullPromocodes.length) {
                    console.log(`\n${chalk.yellowBright(`Only ${pullPromocodes.length} was added.`)}\n`);
                }

                await PromoCode.update({ organizationId }, { where: {
                        id: {[Op.in]: pullPromocodes}
                    }})
            }),
        );
};
