import { formatJSON } from '../lib/utils';
import { ConfigOverride } from './defaults';

const config: ConfigOverride = {
    logger: {
        streams: [
            {
                level: 'info',
                stream: require('@yandex-int/yandex-logger/streams/line')({
                    stream: process.stdout,
                    template: [
                        "{{msg}}",
                        '{{#body}}\n  {{_body}}{{/body}}',
                        '{{#incomingMessage}}\n  {{_incomingMessage}}{{/incomingMessage}}',
                        '{{#context}}\n  {{_context}}{{/context}}',
                        '{{#intent}}\n  {{_intent}}{{/intent}}',
                    ].join(''),
                    resolvers: {
                        _body: (record: any) => {
                            return formatJSON(record.body, 4);
                        },
                        _incomingMessage: (record: any) => {
                            return `NLU: ${JSON.stringify(record.incomingMessage.request.nlu)}`;
                        },
                        _context: (record: any) => {
                            return `Context: ${JSON.stringify(record.context)}`;
                        },
                        _intent: (record: any) => {
                            return `Intent: ${JSON.stringify(record.intent)}`;
                        },
                    }
                })
            }
        ]
    },
};

export default config;
