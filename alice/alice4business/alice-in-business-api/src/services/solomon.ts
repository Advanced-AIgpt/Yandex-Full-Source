import { ServerResponse, createServer } from 'http';
import { asyncMiddleware } from '../controllers/utils';
import gotCounters from './got/counters';
import gotHistograms from './got/histograms';
import { workerCounters } from '../lib/workers/utils';

const SOLOMON_PORT = 3444;

export const workers = ['operations', 'sync', ''];

const counters: Record<string, number> = {
    device__switch_success: 0, // успешные переключения колонки
    device__switch_error: 0, // ошибки переключения колонки

    room__switch_success: 0, // успешные переключения комнаты
    room__switch_error: 0, // ошибки переключения комнаты

    room__activate_error: 0, // ошибка активация комнаты
    device__activate_error: 0, // ошибки активации колонки

    room__reset_success: 0, // успешные сбросы комнаты
    room__reset_error: 0, // ошибки сброса комнаты

    device__reset_success: 0, // успешнык сбросы колонки
    device__reset_error: 0, // ошибки сброса колонки

    promocode__activate_success: 0, // успешные активации промокодов
    promocode__activate_error: 0, // ошибки активации промокодов
};

export const switchCounters: Record<string, number> = {
    compareCounter: 0,
    switchCounter: 0,
};

const TimerHistogramBucketsMs = [
    0,
    10,
    20,
    30,
    40,
    50,
    60,
    70,
    80,
    90,
    100,
    110,
    120,
    130,
    140,
    150,
    160,
    170,
    180,
    190,
    200,
    210,
    220,
    230,
    240,
    250,
    260,
    270,
    280,
    290,
    300,
    400,
    500,
    600,
    700,
    800,
    900,
    1000,
    1100,
    1200,
    1300,
    1400,
    1500,
    1600,
    1700,
    1800,
    1900,
    2000,
    3000,
    5000
];

const SwitchTimerHistogramBucketsMs = [
    1000,
    2000,
    3000,
    4000,
    5000,
    6000,
    7000,
    8000,
    9000,
    10000,
    12000,
    14000,
    16000,
    18000,
    20000,
    22000,
    24000,
    26000,
    28000,
    30000,
    35000,
    40000,
    45000,
    50000,
    60000, // минута
    70000,
    80000,
    90000,
    100000,
    110000,
    120000,
    130000,
    140000,
    150000,
    180000,
    210000,
    240000,
    270000,
    300000,
    360000,
    420000,
    480000,
    540000,
    600000,  // 10 минут
    1200000,
    1800000, // пол часа
    2400000,
    3000000,
    3600000, // час
];

export type HistMetric = {
    bounds: readonly number[];
    buckets: number[];
    inf: number;
};

export const createHist = (bucket: readonly number[]): HistMetric => ({
    bounds: bucket,
    buckets: bucket.map(() => 0),
    inf: 0,
});

export const fillHist = (hist: HistMetric, time: number) => {
    for (let i = 0; i < hist.bounds.length; i++) {
        if (time < hist.bounds[i]) {
            hist.buckets[i]++;
            return;
        }
    }
    hist.inf++;
};

export const makeInitialTimeHistogram = () => createHist(TimerHistogramBucketsMs);
export const makeInitialSwitchTimeHistogram = () => createHist(SwitchTimerHistogramBucketsMs);

const invocationHistograms: Record<string, HistMetric> = {};
const switchHistograms: Record<string, HistMetric> = {};

export const dialogovoHistograms = {
    afterDeviceSelect: makeInitialTimeHistogram(),
    startActivationCodeSelect: makeInitialTimeHistogram(),
    startActivationCodeCreate: makeInitialTimeHistogram(),
    endGetState: makeInitialTimeHistogram()
}

const invocations: any = {};

export function incCounter(counterName: string, countersObj = counters, value = 1) {
    if (!countersObj[counterName]) {
        countersObj[counterName] = 0;
    }

    countersObj[counterName] += value;
}

export function setCounter(counter: string, value = 1) {
    counters[counter] = value;
}

export type Counter = Record<string, number>;
export type Histogram = Record<string, HistMetric>;

type CounterType = 'RATE' | 'COUNTER';

type SerializedCounter = {
    labels: {
        sensor: string
    },
    type: CounterType;
    value: number;
};

type SerializedHistogram = {
    labels: {
        sensor: string
    },
    type: 'HIST_RATE';
    hist: HistMetric;
};

type SerializedMetric = SerializedCounter | SerializedHistogram;

function serializeCounters(countersObj: Counter, labels = {}): SerializedCounter[] {
    return Object.entries(countersObj).map(([key, value]) => {
        return {
            labels: {sensor: key, ...labels},
            type: 'COUNTER',
            value
        }
    })
}

function serializeInvocationCounters(countersObj: Counter, sensor = '', labels = {}): SerializedCounter[] {
    return Object.entries(countersObj).map(([key, value]) => {
        return {
            labels: {'sensor': sensor, path: key, ...labels},
            type: 'RATE',
            value
        }
    })
}

function getCounters(): any {
    return [
        ...(serializeCounters(counters, {counterType: 'counters'})),
        ...(serializeCounters(switchCounters, {counterType: 'switchCounters'})),
        ...(serializeInvocationCounters(invocations, 'requests_rate')),
        ...(serializeCounters(gotCounters.counters, {counterType: 'got_counters'})),
        ...(serializeCounters(workerCounters.counters, {counterType: 'worker_counters'}))
    ];
}

function serializeHistograms(histogramsObj: Histogram, labels = {}): SerializedHistogram[] {
    return Object.entries(histogramsObj).map(([key, hist]) => {
        return {
            labels: {sensor: key, ...labels},
            type: 'HIST_RATE',
            hist
        }
    })
}

function serializeInvocationsHistograms(histogramsObj: Histogram, sensor = '', labels = {}): SerializedHistogram[] {
    return Object.entries(histogramsObj).map(([key, hist]) => {
        return {
            labels: {'sensor': sensor, path: key, ...labels},
            type: 'HIST_RATE',
            hist
        }
    })
}

function getHistograms(): any {
    return [
        ...(serializeInvocationsHistograms(invocationHistograms, 'requests_duration')),
        ...(serializeHistograms(switchHistograms, {histType: 'switch_hist'})),
        ...(serializeHistograms(gotHistograms.histograms, {histType: 'got_hist'})),
        ...(serializeHistograms(dialogovoHistograms, {histType: 'dialogovo_hist'}))
    ];
}

export function serialize(): {metrics: SerializedMetric[]} {
    return {
        metrics: ([] as SerializedMetric[])
            .concat(getCounters())
            .concat(getHistograms())
    };
}



function addInvocationMetric(route: string) {
    // remove all ':' from path like '/skills/:skillId/message'
    const path = route.replace(/:/g, '');
    invocations[path] = (invocations[path] ?? 0) + 1;
}

function addDurationMetric(route: string, time: number) {
    const path = route.replace(/:/g, '');
    if(!invocationHistograms[path]){
        invocationHistograms[path] = makeInitialTimeHistogram();
    }
    fillHist(invocationHistograms[path], time)
}

export function addSwitchMetric(action: string, time: number) {
    if(!switchHistograms[action]){
        switchHistograms[action] = makeInitialSwitchTimeHistogram();
    }
    fillHist(switchHistograms[action], time)
}

const getPathPrefix = (url: string) => {
    let prefix = '/';
    const tree = {
        'console': {
            'customer': {
                'guest': {}
            },
            'devices': {},
            'rooms': {},
            'operations': {},
            'organizations': {}
        },
        'internal': {
            'connect': {},
            'dialogovo': {},
            'droideka': {},
            'quasar': {}
        },
        'public': {
            'devices': {},
            'rooms': {},
            'operations': {}
        },
        'idm': {}
    };

    const urlPaths = url.split('/');
    let currentNode = tree;
    // Start from 2 to ignore first '/' and 'api' from originalUrl
    for (let i = 2; i < urlPaths.length; i++) {
        const treeNode = (currentNode as any)[urlPaths[i]];
        if(treeNode !== undefined){
            prefix+=urlPaths[i]+'/';
            currentNode = treeNode;
        } else {
            break;
        }
    }
    return prefix.substring(0, prefix.length-1);
}

export const middleware = asyncMiddleware(
    async (req: Express.Request, res: ServerResponse) => {
        const ts = Date.now();

        const end = res.end;
        res.end = (...args: any) => {
            const responseTime = Date.now() - ts;

            end.apply(res, args);

            if ('route' in req) {
                const path = getPathPrefix((req as any).originalUrl) + (req as any).route.path;
                addDurationMetric(path, responseTime);
                addInvocationMetric(path);
            }
        };
    },
);

const server = createServer({}, (req, res) => {
    const path = req.url?.replace(/\?.*$/, '');
    if (path === '/solomon' && req.method === 'GET') {
        res.statusCode = 200;
        res.setHeader('Content-Type', 'application/json');
        res.write(JSON.stringify(serialize()));
        res.end();
        return;
    }
    res.statusCode = 404;
    res.end();
});

export const startSolomon = () => {
    server.listen(SOLOMON_PORT);
    server.on('listening', () => {
        console.log('Started solomon server on ' + SOLOMON_PORT);
    });
};
