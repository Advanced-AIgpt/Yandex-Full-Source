import commander from 'commander';
import { askForInput, makeAChoice, makeAConfirm, withErrors } from '../../utils';
import config from '../../../lib/config';
import Connect from '../../../services/connect';
import chalk from 'chalk';

export default (program: commander.Command) => {
    program
        .command('connect:fix-webhook')
        .description(`Fix Connect webhooks for ${config.connect.selfSlug} service`)
        .action(
            withErrors(async () => {
                const hooks = await Connect.getWebhooks();

                if (hooks.length > 0) {
                    console.log(`\n${chalk.yellowBright('Existing webhooks:')}\n`);

                    for (const hook of hooks) {
                        console.log(
                            [
                                `id:\t\t${chalk.yellowBright(hook.id.toString())}`,
                                `url:\t\t${chalk.greenBright(hook.url)}`,
                                `events:\t\t${chalk.whiteBright(
                                    hook.event_names.join('\n\t\t'),
                                )}`,
                                `tvm_client_id:\t${chalk.whiteBright(
                                    (hook.tvm_client_id || 'null').toString(),
                                )}`,
                                `service_id:\t${chalk.whiteBright(hook.service_id)}\n`,
                            ].join('\n'),
                        );
                    }

                    const hooksToRemove = await makeAChoice(
                        hooks.map((hook) => ({
                            name: `id=${hook.id} URL=${hook.url}`,
                            value: hook.id,
                        })),
                        `Choose webhooks ${chalk.redBright('to delete')}`,
                        'checkbox',
                    );

                    for (const id of hooksToRemove) {
                        await Connect.deleteWebhook(id);
                    }

                    console.log(
                        `\n${chalk.yellowBright(
                            `Removed ${hooksToRemove.length} webhooks`,
                        )}\n`,
                    );
                } else {
                    console.log(
                        `\n${chalk.red('There are no webhooks registered in Connect')}\n`,
                    );
                }

                if (await makeAConfirm('Add webhook?')) {
                    console.log(
                        `Suggested hook URL: ${chalk.whiteBright(
                            `https://${chalk.cyan(config.connect.defaultHookHost)}${
                                config.app.urlRoot
                            }/internal/connect/sync`,
                        )}`,
                    );

                    const url =
                        (await askForInput('Hook URL', false)) ||
                        `https://${config.connect.defaultHookHost}${
                            config.app.urlRoot
                        }/internal/connect/sync`;

                    await Connect.createWebhook({
                        url,
                        event_names: [
                            'service_enabled',
                            'service_disabled',
                            'organization_deleted',
                            'organization_blocked',
                            'organization_unblocked',
                            'resource_added',
                            'resource_deleted',
                            'resource_modified',
                            'resource_relation_added',
                            'resource_relation_deleted',
                            'resource_grant_changed',
                            'group_membership_changed',
                            'group_group_added',
                            'group_group_deleted',
                            'department_user_added',
                            'department_user_deleted',
                            'department_group_added',
                            'department_group_deleted',
                            'department_department_added',
                            'department_department_deleted',
                        ],
                        tvm_client_id: config.tvmtool.selfId,
                    });
                }
            }),
        );
};
