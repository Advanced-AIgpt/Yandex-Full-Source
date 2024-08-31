import test from 'ava';
import sinon from 'sinon';
import { createCredentials } from '../../helpers/api';
import { wipeDatabase } from '../../helpers/db';

test.beforeEach(async (t) => {
    await wipeDatabase();
    await createCredentials();
});
test.afterEach.always(async (t) => {
    sinon.restore();
});

test.todo('Возвращает 200 оk');
