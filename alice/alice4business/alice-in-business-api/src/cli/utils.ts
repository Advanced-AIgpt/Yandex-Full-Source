import inquirer from 'inquirer';
import { getUserInfo } from '../services/blackbox';
import chalk from 'chalk';
import { CommanderStatic } from 'commander';
import { UserInstance } from '../db/tables/user';
import { ConnectOrganization, Device, Organization, User } from '../db';
import { OrganizationInstance } from '../db/tables/organization';
import inquirerAutocomplete from 'inquirer-autocomplete-prompt';
import Connect from '../services/connect';
import ip from 'ip';
import _commander = require('commander');

inquirer.registerPrompt('autocomplete', inquirerAutocomplete);

export function withErrors(command: (...args: any[]) => Promise<void>) {
    return async (...args: any[]) => {
        try {
            await command(...args);
            console.log('\n\n');
            const exit = await makeAConfirm('Exit CLI?');
            if (exit) {
                process.exit(0);
            }
            showCommandsList();
        } catch (e) {
            console.log(chalk.red(e.stack));
            process.exit(1);
        }
    };
}

export async function askForInput(
    label: string,
    required = true,
    additionalValidation?: (value: string, label: string) => string|boolean
) {
    const { input } = await inquirer.prompt<{ input: string }>([
        {
            type: 'input',
            name: 'input',
            message: label,
            validate: (value: string) => {
                if (required && value.length === 0) {
                    return `${label} is required`;
                }
                const additionalError = additionalValidation && additionalValidation(value, label);

                if(additionalError){
                    return additionalError;
                }

                return true;
            },
        },
    ]);

    return input;
}

export const showChoices = async (
    commandsList: string[],
    commander: CommanderStatic,
    args?: string[],
) => {
    const searchCommand = (answers: any[], input: string = '') => {
        return Promise.resolve(commandsList.filter((s) => s.indexOf(input) !== -1));
    };

    const { fullCommand } = await inquirer.prompt<{ fullCommand: string }>([
        {
            type: 'autocomplete',
            name: 'fullCommand',
            choices: commandsList,
            message: 'what full command do you want to run?',
            source: searchCommand,
        } as any,
    ]);

    commander._events[`command:${fullCommand}`](args);
};

export const makeAConfirm = async (message: string) => {
    const { answer } = await inquirer.prompt<{ answer: any }>([
        {
            type: 'confirm',
            name: 'answer',
            message,
        },
    ]);

    return answer;
};

export type inquirerType = 'checkbox' | 'list' | 'rawlist' | 'expand' | 'autocomplete';

export const _makeChoiceQuestion = (
    choiceList: inquirer.ChoiceType[],
    type: inquirerType = 'list',
) => {
    const search = (answers: any[], input: string = '') => {
        return Promise.resolve(
            choiceList.filter((item) => {
                if (item instanceof inquirer.Separator) {
                    return false;
                }
                if (typeof item === 'string') {
                    item = { name: item };
                }
                return (item as any).name.indexOf(input) !== -1;
            }),
        );
    };

    return {
        type,
        choices: choiceList,
        source: type === 'autocomplete' ? search : undefined,
    };
};

export const makeAChoice = async (
    choiceList: inquirer.ChoiceType[],
    message: string,
    type: inquirerType = 'list',
) => {
    const { action } = await inquirer.prompt<{ action: any }>([
        {
            ..._makeChoiceQuestion(choiceList, type),
            name: 'action',
            message,
        } as any,
    ]);

    return action;
};

export const commandsWithPrefix = (cmd: string, commander: CommanderStatic) => {
    const matchedCommands: string[] = [];
    const commands = commander
        .eventNames()
        .map((command: string | symbol) => command.toString().replace(/^command:/, ''))
        .filter((c: string) => c !== '*');
    for (const command of commands) {
        if (!cmd || command.indexOf(cmd) !== -1) {
            matchedCommands.push(command);
        }
    }
    return matchedCommands;
};

export const askForUserWithId = async () => {
    const { userId } = await inquirer.prompt<{ userId: string }>([
        {
            type: 'input',
            name: 'userId',
            message: 'User id',
        },
    ]);
    return await User.findByPk(userId);
};

export async function askForUser() {
    const { login } = await inquirer.prompt<{ login: string }>([
        {
            type: 'input',
            name: 'login',
            message: "User's yandex login",
        },
    ]);
    const user = (await getUserInfo({ login, userip: ip.address() })).body.users[0];

    return {
        login,
        id: user.uid.value as string,
    } as UserInstance;
}

export async function fetchUid(login: string) {
    const response = await getUserInfo({ login, userip: ip.address() });

    try {
        const user = response.body.users[0];
        return user.uid.value;
    } catch (err) {
        console.error(err);

        return undefined;
    }
}

export const showCommandsList = () => {
    const allCommandsList = commandsWithPrefix('', _commander);
    showChoices(allCommandsList, _commander).catch(console.error);
};

export const getOrganizationsChoiceList = async (group = false) => {
    const organizations = await Organization.findAll();

    const connectOrgs = {} as Record<string, string>;
    const byConnectOrgId = {} as Record<string, OrganizationInstance[]>;

    for (const organization of organizations) {
        const orgId = organization.connectOrgId || '';

        if (byConnectOrgId[orgId]) {
            byConnectOrgId[orgId].push(organization);
        } else {
            byConnectOrgId[orgId] = [organization];

            if (orgId) {
                const connectOrganization = await Connect.getOrganization(
                    orgId,
                    ['name'],
                    { tvmSrc: 'connect-controller' },
                );

                connectOrgs[orgId] = `${connectOrganization.name} (${orgId})`;
            }
        }
    }

    const result = [] as inquirer.ChoiceType[];
    for (const orgId of Object.keys(byConnectOrgId).sort(
        (a, b) => parseInt(a, 10) - parseInt(b, 10),
    )) {
        const connectHeader = (connectOrgs[orgId] || 'Not Connect`ed') + ' >>>';

        if (group) {
            result.push(new inquirer.Separator(connectHeader));
        }

        result.push(
            ...byConnectOrgId[orgId]
                .sort((a, b) => a.name.localeCompare(b.name))
                .map((org) => ({
                    value: org.id,
                    name: group
                        ? `${org.name} (${org.id})`
                        : `${connectHeader} ${org.name} (${org.id})`,
                })),
        );
    }

    return result;
};

export const getOrganizationDevicesChoiceList = async (organizationId: string) => {

    const organization = (await Organization.findOne({
        where: { id: organizationId },
        include: [
            {
                model: Device,
                where: {
                    roomId: null
                }
            },
        ],
        rejectOnEmpty: true,
    }))!;
    const result = [] as inquirer.ChoiceType[];
    for (const device of organization.devices!) {
        result.push({
            value: device,
            name: `${device.deviceId} (${device.note})`
        });
    }
    return result;
}

export const getUserChoiceList = async () => {
    const users = await User.findAll();
    return users.map((user) => ({
        value: user.id,
        name: `${user.login} (${user.id})`,
    })) as inquirer.ChoiceType[];
};

export const getDevicesChoiceList = async () => {
    const devices = await Device.findAll();
    return devices.map((device) => ({
        value: device.id,
        name: `${device.platform} (${device.id})`,
    })) as inquirer.ChoiceType[];
};

export const getConnectOrganizationsChoiceList = async () => {
    const organizations = await ConnectOrganization.findAll();

    if (organizations.length === 0) {
        console.log(
            chalk.cyan('There is no Connect organizations in DB; add them first'),
        );
    }

    return organizations
        .sort((a, b) => a.id - b.id)
        .map((org) => ({
            name: `${org.id}: ${org.name}${org.active ? ' (active)' : ''}`,
            value: org.id,
        })) as inquirer.ChoiceType[];
};
