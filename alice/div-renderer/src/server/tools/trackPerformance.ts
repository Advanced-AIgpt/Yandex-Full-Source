export const getTimeMark = () => {
    const start = Date.now();

    return () => {
        const end = Date.now();
        const duration = end - start;

        return duration;
    };
};

type AnyFn<I, R> = (...args: I[]) => R
type AnyAsyncFn<I, R> = (...args: I[]) => Promise<R>;
type LoggerCallback<I> = (v: { duration: number, args: I[] }) => void;

export const trackFn = <I, R>(fn: AnyFn<I, R>, log: LoggerCallback<I>) => {
    const resultFn: AnyFn<I, R> = (...args) => {
        const startMark = getTimeMark();

        const fnResult = fn(...args);

        const duration = startMark();

        setTimeout(() => log({
            duration,
            args,
        }));

        return fnResult;
    };

    return resultFn;
};

export const trackAsyncFn = <I, R>(fn: AnyAsyncFn<I, R>, log: LoggerCallback<I>) => {
    const resultFn: AnyAsyncFn<I, R> = async(...args) => {
        const startMark = getTimeMark();

        const fnResult = await fn(...args);

        const duration = startMark();

        setTimeout(() => log({
            duration,
            args,
        }));

        return fnResult;
    };

    return resultFn;
};
