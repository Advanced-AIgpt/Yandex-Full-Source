import commander from "commander";
import inquirer from "inquirer";
import { RoomInstance, RoomSchema } from "../../../db/tables/room";
import { Room } from '../../../db';
import { withErrors, makeAChoice , getOrganizationsChoiceList, askForInput, getOrganizationDevicesChoiceList} from "../../utils";
import { DeviceSchema } from "../../../db/tables/device";

export default (program: commander.Command) => {
    program
        .command('rooms:create')
        .description('Create a room')
        .action(
            withErrors(async () => {
                const organizationId = await makeAChoice(
                    await getOrganizationsChoiceList(),
                    'Room organization',
                    'autocomplete',
                );
                const name = await askForInput("Room Name");
                const externalRoomId = await askForInput("Room External Id (null)", false);
                const r = await Room.create({
                    externalRoomId: externalRoomId || null, 
                    name,
                    organizationId
                } as RoomSchema);
                console.log(`Created room with id ${r.id}`);
                const devices = await makeAChoice(
                    await getOrganizationDevicesChoiceList(organizationId),
                    'Devices in the room',
                    'checkbox',
                );
                const updates = [] as Promise<any>[];
                for (const device of devices) {
                    updates.push(device.update({roomId: r.id} as DeviceSchema));
                }
                await Promise.all(updates)
            })
        );
}