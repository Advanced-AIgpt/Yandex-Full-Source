import { ProtobufJSContext } from '@yandex-int/apphost-lib';
import { NAlice } from '../../protos';
import { putTimeToSend } from '../../solomon';
import { trackFn } from './trackPerformance';

interface SendResultParam {
    context: ProtobufJSContext;
    protoResult: NAlice.NRenderer.TRenderResponse[];
}
export const sendResult = trackFn(({ context, protoResult }: SendResultParam) => {
    protoResult.forEach(item => context.sendProtoItem('render_result', NAlice.NRenderer.TRenderResponse, item));
}, ({ duration }) => putTimeToSend(duration));
