import { ProtobufJSContext } from '@yandex-int/apphost-lib';
import { putTimeToParseChunks } from '../../solomon';
import { trackAsyncFn } from './trackPerformance';

export const getChunks = trackAsyncFn(async(ctx: ProtobufJSContext) => {
    const result = await ctx.allIncomingChunks();

    return result;
}, ({ duration }) => putTimeToParseChunks(duration));
