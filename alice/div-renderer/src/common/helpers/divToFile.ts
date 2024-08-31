import * as fs from 'fs';
import { createDirectiveToShowView } from '../../projects/centaur/actions/client';

export default function divToFile(path: string, options: Parameters<typeof createDirectiveToShowView>[0]) {
    fs.writeFileSync(path, JSON.stringify(
        createDirectiveToShowView(options),
    ), 'utf-8');
}
