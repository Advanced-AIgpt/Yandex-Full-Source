import commander from 'commander';
import { commandsWithPrefix, showChoices, showCommandsList } from './utils';

// tslint:disable:no-var-requires
require('./commands/promoCode/add').default(commander);
require('./commands/promoCode/add-to-organization').default(commander);

require('./commands/organization/create').default(commander);
require('./commands/organization/change').default(commander);
require('./commands/organization/set-max-volume').default(commander);

require('./commands/connect/sync-organization-list').default(commander);
require('./commands/connect/enable').default(commander);
require('./commands/connect/disable').default(commander);
require('./commands/connect/sync').default(commander);
require('./commands/connect/permissions').default(commander);
require('./commands/connect/domain').default(commander);
require('./commands/connect/add-whois').default(commander);
require('./commands/connect/add-by-id').default(commander);
require('./commands/connect/fix-webhook').default(commander);

require('./commands/users/create').default(commander);
require('./commands/users/bind').default(commander);

require('./commands/device/create').default(commander);
require('./commands/device/change').default(commander);
require('./commands/device/add-puid').default(commander);

require('./commands/room/create').default(commander);

commander.action(async (command) => {
    const args = process.argv;
    const commandsList = commandsWithPrefix(command, commander);
    if (commandsList.length) {
        await showChoices(commandsList, commander, args);
    } else {
        console.error('Unknown command');
    }
});

commander.parse(process.argv);

if (!commander.args.length) {
    showCommandsList();
}
