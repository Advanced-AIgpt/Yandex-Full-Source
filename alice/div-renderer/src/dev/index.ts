import { join } from 'path';
import { existsSync, readFileSync } from 'fs';
import * as express from 'express';

import { isString } from 'lodash';
import { logger } from '../common/logger';
import { templates, Templates } from '../projects';
import { wrappers } from './wrappers';
import { NAlice } from '../protos';
import { MMRequest } from '../common/helpers/MMRequest';
import { createRequestState } from '../registries/common';

const app = express();

app.use(express.static(join(__dirname, 'assets')));

const SAMPLES = join(__dirname, 'samples');
const mmRequest = new MMRequest(
    new NAlice.NScenarios.TScenarioResponseBody(), new NAlice.NScenarios.TInput(), {});

const templatesRoutes = express.Router();
templatesRoutes.get('*', (req, res) => {
    const templateId = req.path.replace(/^\//, '');
    const fn = templates[templateId as keyof Templates];
    if (!fn) {
        res.status(404).send('template not found');
        return;
    }
    const data = (() => {
        let { d, f } = req.query;
        if (isString(d)) {
            try {
                return JSON.parse(d);
            } catch {
                return {};
            }
        }
        if (isString(f)) {
            if (!f.endsWith('.json')) {
                f = f + '.json';
            }
            const p = join(SAMPLES, f);
            if (existsSync(p)) {
                return JSON.parse((readFileSync(p, {
                    encoding: 'utf-8',
                })));
            }
            throw new Error('sample not found');
        }
    })();

    const wrap = req.query.wrap as keyof (typeof wrappers);
    if (wrap && !wrappers[wrap]) {
        res.status(400).send('unknown wrap type');
    }
    const result = wrappers[wrap]?.(fn(data, mmRequest, createRequestState())) ||
        fn(data, mmRequest, createRequestState());
    res.send(result);
});

app.use('/t', templatesRoutes);

const PORT = process.env.PORT || 8000;

app.listen(PORT).on('listening', () => {
    logger.info(`Dev server is listening on ${PORT}`);
});
