import anyTest, { TestInterface } from 'ava';
import sinon from 'sinon';
import * as promoActivatingImpl from '../../../lib/implements/promo-activate';
import * as worker from '../../../lib/workers/promo-counter';
import { createOrganization, wipeDatabase } from '../../helpers/db';
import { startWorkers, stopWorkers } from '../../helpers/workers';
import { sleep } from '../../../lib/utils';
import config from '../../../lib/config';

const test = anyTest as TestInterface;

test.beforeEach(async (t) => {
    sinon.stub(promoActivatingImpl, 'activatePromoCodeForDevice').resolves();
    await wipeDatabase();
    await startWorkers(worker.promoCounterWorker, 10);
    await createOrganization();
});

test.afterEach.always(async (t) => {
    await stopWorkers(worker.promoCounterWorker);
    await sleep(config.app.countersWorker.interval * 2);
    sinon.restore();
});

test.todo('solomon');
