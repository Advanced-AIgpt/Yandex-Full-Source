import commander from 'commander';
import { getConnectOrganizationsChoiceList, makeAChoice, withErrors } from '../../utils';
import config from '../../../lib/config';
import Connect from '../../../services/connect';
import { Relation, Resource } from '../../../services/connect/resources';
import { User } from '../../../services/connect/users';
import { Group } from '../../../services/connect/groups';
import { Department } from '../../../services/connect/departments';
import { Types as PermissionTypes } from '../../../db/tables/permission';
import ip from 'ip';

export default (program: commander.Command) => {
    program
        .command('connect:permissions')
        .description(
            `Edit ${
                config.connect.selfSlug
            } service resource relations (access rules) for Connect organization`,
        )
        .action(
            withErrors(async () => {
                const orgId = (await makeAChoice(
                    await getConnectOrganizationsChoiceList(),
                    'Choose Connect organization',
                    'autocomplete',
                )) as number;

                const resource: Resource = await makeAChoice(
                    (await Connect.getResources(orgId))
                        .sort((a, b) => a.id!.localeCompare(b.id!))
                        .map((item) => ({
                            name: item.id,
                            value: item,
                        })),
                    'Choose resource:',
                    'autocomplete',
                );

                const { admin_id } = await Connect.getOrganization(orgId, ['admin_id'], {
                    tvmSrc: 'connect-controller',
                });

                const departmentChoices = (await Connect.getDepartments(
                    orgId,
                    undefined,
                    {
                        tvmSrc: 'connect-controller',
                    },
                )).map((item) => ({
                    name: formatDepartment(item),
                    value: item,
                }));

                const groupChoices = (await Connect.getGroups(orgId, undefined, {
                    tvmSrc: 'connect-controller',
                })).map((item) => ({
                    name: formatGroup(item),
                    value: item,
                }));

                const userChoices = (await Connect.getUsers(
                    orgId,
                    ['id', 'name', 'nickname'],
                    {
                        tvmSrc: 'connect-controller',
                    },
                )).map((item) => ({
                    name: formatUser(item),
                    value: item,
                }));

                // repeat actions
                actions: while (true) {
                    const relations = resource.relations!.sort(
                        (a: any, b: any) =>
                            a.name.localeCompare(b.name) ||
                            a.object_type.localeCompare(b.object_type) ||
                            a.object_id - b.object_id,
                    );

                    const action = await makeAChoice(
                        [
                            {
                                name: 'Add permission',
                                value: 'add',
                            },
                            ...(relations.length > 0
                                ? [
                                      {
                                          name: 'Remove permission',
                                          value: 'remove',
                                      },
                                  ]
                                : []),
                            {
                                name: 'Finish and save',
                                value: 'done',
                            },
                        ],
                        `Current permissions:\n${
                            relations.length > 0
                                ? relations
                                      .map(formatRelation)
                                      .map(
                                          (item) =>
                                              `  ${item.access} =>\t${item.objectType}\t${
                                                  item.label
                                              }`,
                                      )
                                      .join('\n')
                                : '  *** no entries'
                        }\nChoose action:`,
                    );

                    switch (action) {
                        case 'add':
                            const name = (await makeAChoice(
                                Object.entries(PermissionTypes).map(
                                    ([type, description]) => ({
                                        name: `${type} (${description})`,
                                        value: type,
                                    }),
                                ),
                                'Choose permission type:',
                            )).toLowerCase();

                            const objectType = (await makeAChoice(
                                ['Department', 'Group', 'User'],
                                'Choose object type:',
                            )).toLowerCase();

                            switch (objectType) {
                                case 'department':
                                    relations.push({
                                        name,
                                        object_type: objectType,
                                        object: await makeAChoice(
                                            departmentChoices,
                                            'Choose department:',
                                            'autocomplete',
                                        ),
                                    });

                                    break;

                                case 'group':
                                    relations.push({
                                        name,
                                        object_type: objectType,
                                        object: await makeAChoice(
                                            groupChoices,
                                            'Choose group: ',
                                            'autocomplete',
                                        ),
                                    });

                                    break;

                                case 'user':
                                    relations.push({
                                        name,
                                        object_type: objectType,
                                        object: await makeAChoice(
                                            userChoices,
                                            'Choose user: ',
                                            'autocomplete',
                                        ),
                                    });

                                    break;
                            }

                            break;

                        case 'remove':
                            const indexes = await makeAChoice(
                                relations.map(formatRelation).map((item, idx) => ({
                                    name: `${item.access} =>\t${item.objectType}\t${
                                        item.label
                                    }`,
                                    value: idx,
                                })),
                                'Choose permission to remove',
                                'checkbox',
                            );
                            for (const idx of indexes) {
                                delete relations[idx];
                            }

                            break;

                        case 'done':
                            break actions;
                    }

                    resource.relations = relations.filter(Boolean);
                    console.log();
                }
                // actions done. Save result

                await Connect.updateRelations(
                    orgId,
                    resource.id,
                    resource.relations!.map((item) => ({
                        name: item.name,
                        object_type: item.object_type,
                        object_id: item.object!.id,
                    })),
                    { uid: admin_id!, ip: ip.address() },
                );
            }),
        );
};

const formatDepartment = (item: Department) => `${item.name}`;
const formatGroup = (item: Group) => `${item.name}`;
const formatUser = (item: User) =>
    `${item.nickname}@ ${[item.name.last, item.name.first, item.name.middle]
        .filter(Boolean)
        .join(' ')} (${item.id})`;
const formatRelation = (item: Relation) => {
    let label = '';
    switch (item.object_type) {
        case 'department':
            label = formatDepartment(item.object as Department);
            break;

        case 'group':
            label = formatGroup(item.object as Group);
            break;

        case 'user':
            label = formatUser(item.object as User);
    }

    return { access: item.name, label, objectType: item.object_type };
};
