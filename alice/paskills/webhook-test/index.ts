import express, { response } from 'express';
import { handleIncomingMessage } from './dialog';
import { IncomingMessage } from './dialog/transport';
import logger from './lib/logger';
import pkg from './package.json';

const { local_port: localPort } = pkg.config;
const { QLOUD_HTTP_PORT: port = localPort } = process.env;

const app = express();

const logPretty = (json: object) => console.log(JSON.stringify(json, null, 4));

app.use(express.json(), (req, res) => {
    (async () => {
        const incomingMessage = req.body as IncomingMessage;
        const headers = req.headers;

        logPretty(incomingMessage);
        if (incomingMessage?.request?.command?.toLocaleLowerCase()?.startsWith('ошибка http ')) {
            let errCode =
                incomingMessage.request.command.toLocaleLowerCase().replace('ошибка http ', '')

            logger.error('responding with error core: ' + errCode);
            res.sendStatus(parseInt(errCode))
        }

        const outgoingMessage = await handleIncomingMessage(incomingMessage, headers);

        logPretty(outgoingMessage);

        res.json(outgoingMessage);
    })().catch(error => {
        logger.error(error);
        res.sendStatus(500);
    });
});

app.listen(port, () => {
    console.log('Server started at port ' + port);
});
