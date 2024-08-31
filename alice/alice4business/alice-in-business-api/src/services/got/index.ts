import got from 'got';
import log from '../../lib/log';
import { CounterType } from './types';
import gotCounters from './counters';
import { getHttpStatusType, is4xxErr, is5xxErr, isConnectionError } from './helpers';
import gotHistograms from './histograms';

interface CreateGotOptions {
    sourceName: string;
    baseUrl?: string;
    timeout?: number;
}

const createGot = ({
    sourceName,
    baseUrl,
    timeout = 10000,
}: CreateGotOptions): typeof got => {
    gotCounters.createCounters(sourceName);
    gotHistograms.createHistogram(sourceName);

    const inc = (type: CounterType) => gotCounters.incCounter(sourceName, type);
    const writeGotHistogramValue = (value: number) =>
        gotHistograms.writeHistogramValue(sourceName, value);

    const logError = (e: any) => {
        inc('error');

        if (e instanceof got.TimeoutError) {
            inc('timeout_error');

            log.warn(`got: Timeout error in ${sourceName}`, {
                url: e.url,
                path: e.path,
                code: e.code,
                method: e.method,
            });

            return;
        }

        if (isConnectionError(e)) {
            inc('connection_error');

            log.warn(`got: Connection error in ${sourceName}`, {
                url: e.url,
                path: e.path,
                code: e.code,
                method: e.method,
            });

            return;
        }

        if (e instanceof got.HTTPError) {
            const statusCode = e.statusCode;

            inc(getHttpStatusType(statusCode));

            const logLevel =
                is4xxErr(statusCode) || is5xxErr(statusCode) ? 'warn' : 'info';

            log[logLevel](`got: ${e.statusMessage} (${statusCode}) in ${sourceName}`, {
                url: e.url,
                path: e.path,
                statusCode,
                headers: e.headers,
                body: (e as any).body,
            });

            return;
        }

        inc('other_error');
        log.warn(`got: Other error in ${sourceName}`, { error: e });

        return;
    };

    const gotFn = async (...args: Parameters<typeof got>) => {
        let response;

        inc('total');

        try {
            response = await got(args[0], {
                timeout,
                retry: 0,
                ...(baseUrl && { baseUrl }),
                ...args[1],
            });

            const statusCode = response.statusCode!;

            inc(getHttpStatusType(statusCode));
        } catch (e) {
            logError(e);

            // response будет только в HTTPError и ReadError
            response = e.response;

            throw e;
        } finally {
            if (response) {
                const { timings } = response as got.Response<any>;

                writeGotHistogramValue(timings.phases.total!);
            }
        }

        return response;
    };

    return gotFn as typeof got;
};

export default createGot;
