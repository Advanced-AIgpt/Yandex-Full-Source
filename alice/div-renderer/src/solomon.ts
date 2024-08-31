import { createServer } from 'http';
import { constant } from 'lodash';
import { logger } from './common/logger';
import { templates } from './projects';
import { getEnvs } from './env';

type TemplateKeys = keyof typeof templates;

const renderTimeBucket = [1, 2, 5, 10, 15, 20, 25, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 2000, 3000] as const;
type HistMetric = {
    bounds: readonly number[];
    buckets: number[];
    inf: number;
};

type MetricsStore<K extends string, E> = {
    [key in K]?: E;
};

type MetricsUpdator<E, I> = (metricsEntry: E, ...args: I[]) => void;

const createMetricsUpdatorFabric = <K extends string, E, S extends MetricsStore<K, E>>(store: S, createEntry: () => E) => {
    return <I>(update: MetricsUpdator<E, I>) => {
        return (key: K, ...args: I[]) => {
            if (!store[key]) {
                store[key] = createEntry() as S[K];
            }

            update(store[key] as E, ...args);
        };
    };
};

const createHist = (bucket: readonly number[]) => ({
    bounds: bucket,
    buckets: bucket.map(constant(0)),
    inf: 0,
});

const fillHist = (hist: HistMetric, time: number) => {
    for (let i = 0; i < hist.bounds.length; i++) {
        if (time < hist.bounds[i]) {
            hist.buckets[i]++;
            return;
        }
    }
    hist.inf++;
};

const totalTime = createHist(renderTimeBucket);
const totalTimeBatchSizeOne = createHist(renderTimeBucket);
const timeToParseChunks = createHist(renderTimeBucket);
const timeToParseAllProto = createHist(renderTimeBucket);
const timeToEncodeAllProto = createHist(renderTimeBucket);
const timeToSend = createHist(renderTimeBucket);
const timeToRender = createHist(renderTimeBucket);
const timeToGetMMRequest = createHist(renderTimeBucket);
const timeToEventLoopRun = createHist(renderTimeBucket);

type TemplateMetricsEntry = {
    requests: number;
    fails: number;
    renderTimeHist: HistMetric;
};
type TemplateMetricsStore = {
    [key in TemplateKeys]?: TemplateMetricsEntry;
}
const templateMetrics: TemplateMetricsStore = {};
const createTemplateMetricsUpdater = createMetricsUpdatorFabric(templateMetrics, () => ({
    requests: 0,
    fails: 0,
    renderTimeHist: createHist(renderTimeBucket),
}));
export const incTemplateRequest = createTemplateMetricsUpdater(m => m.requests++);
export const incTemplateFails = createTemplateMetricsUpdater(m => m.fails++);
export const putTemplateRenderTime = createTemplateMetricsUpdater((m, time: number) => {
    fillHist(m.renderTimeHist, time);
});

interface GCMetricsEntry {
    runCount: number;
    runDurationHist: HistMetric,
}
type GCMetricsStore = Record<string, GCMetricsEntry>;
const gcMetrics: GCMetricsStore = {};
const createGCMetricsUpdater = createMetricsUpdatorFabric(gcMetrics, () => ({
    runCount: 0,
    runDurationHist: createHist(renderTimeBucket),
}));

export const putTimeToGCRun = createGCMetricsUpdater((m, time: number) => {
    fillHist(m.runDurationHist, time);
});
export const incGCRun = createGCMetricsUpdater(m => m.runCount++);

export const putTotalTime = (time: number) => {
    fillHist(totalTime, time);
};
export const putTotalTimeBatchSizeOne = (time: number) => {
    fillHist(totalTimeBatchSizeOne, time);
};
export const putTimeToParseChunks = (time: number) => {
    fillHist(timeToParseChunks, time);
};
export const putTimeToParseAllProto = (time: number) => {
    fillHist(timeToParseAllProto, time);
};
export const putTimeToEncodeAllProto = (time: number) => {
    fillHist(timeToEncodeAllProto, time);
};
export const putTimeToSend = (time: number) => {
    fillHist(timeToSend, time);
};
export const putTimeToRender = (time: number) => {
    fillHist(timeToRender, time);
};
export const putTimeToGetMMRequest = (time: number) => {
    fillHist(timeToGetMMRequest, time);
};
export const putTimerToEventLoopRun = (time: number) => {
    fillHist(timeToEventLoopRun, time);
};

const dumpSolomonMetrics = () => ({
    metrics: [
        ...Object.entries(templateMetrics).flatMap(([template, metrics]) => [
            {
                labels: {
                    sensor: 'div-renderer.requests',
                    template,
                },
                type: 'RATE',
                value: metrics.requests,
            },
            {
                labels: {
                    sensor: 'div-renderer.fails',
                    template,
                },
                type: 'RATE',
                value: metrics.fails,
            },
            {
                labels: {
                    sensor: 'div-renderer.render_time',
                    template,
                },
                type: 'HIST_RATE',
                hist: metrics.renderTimeHist,
            },
        ]),
        ...Object.entries(gcMetrics).flatMap(([gcEventType, metrics]) => [
            {
                labels: {
                    sensor: 'div-renderer.gc_run_count',
                    gcEventType,
                },
                type: 'RATE',
                value: metrics.runCount,
            },
            {
                labels: {
                    sensor: 'div-renderer.gc_run_time',
                    gcEventType,
                },
                type: 'HIST_RATE',
                hist: metrics.runDurationHist,
            },
        ]),
        {
            labels: {
                sensor: 'div-renderer.total_gc_run_count',
            },
            type: 'RATE',
            value: Object.values(gcMetrics).reduce((result, item) => result + item.runCount, 0),
        },
        {
            labels: {
                sensor: 'div-renderer.event_loop_run_delay',
            },
            type: 'HIST_RATE',
            hist: timeToEventLoopRun,
        },
        {
            labels: {
                sensor: 'div-renderer.total_time',
            },
            type: 'HIST_RATE',
            hist: totalTime,
        },
        {
            labels: {
                sensor: 'div-renderer.total_time.batch_size_one',
            },
            type: 'HIST_RATE',
            hist: totalTimeBatchSizeOne,
        },
        {
            labels: {
                sensor: 'div-renderer.time_to_parse_chunks',
            },
            type: 'HIST_RATE',
            hist: timeToParseChunks,
        },
        {
            labels: {
                sensor: 'div-renderer.time_to_parse_all_proto',
            },
            type: 'HIST_RATE',
            hist: timeToParseAllProto,
        },
        {
            labels: {
                sensor: 'div-renderer.time_to_encode_all_proto',
            },
            type: 'HIST_RATE',
            hist: timeToEncodeAllProto,
        },
        {
            labels: {
                sensor: 'div-renderer.time_to_send',
            },
            type: 'HIST_RATE',
            hist: timeToSend,
        },
        {
            labels: {
                sensor: 'div-renderer.time_to_render',
            },
            type: 'HIST_RATE',
            hist: timeToRender,
        },
        {
            labels: {
                sensor: 'div-renderer.time_to_get_mm_request',
            },
            type: 'HIST_RATE',
            hist: timeToGetMMRequest,
        },
    ],
});

const server = createServer({}, (req, res) => {
    const path = req.url?.replace(/\?.*$/, '');
    if (path === '/solomon' && req.method === 'GET') {
        res.statusCode = 200;
        res.setHeader('Content-Type', 'application/json');
        res.write(JSON.stringify(dumpSolomonMetrics()));
        res.end();
        return;
    }
    res.statusCode = 404;
    res.end();
});

export const startSolomon = () => {
    server.listen(getEnvs().SOLOMON_PORT);
    server.on('listening', () => {
        logger.info('Started solomon server on ' + getEnvs().SOLOMON_PORT);
    });
};
