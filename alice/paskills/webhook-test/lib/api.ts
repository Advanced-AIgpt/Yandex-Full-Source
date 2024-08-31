import got from 'got';
import cfg from '../configs';
import logger from '../lib/logger';

const client = (got as any).extend({
    json: true,
    baseUrl: cfg.api.url,
    hooks: {
        afterResponse: (response: any) => {
            const route = response.requestUrl.replace(cfg.api.url, '').slice(1);

            logger.info({
                name: 'api',
                requestUrl: response.requestUrl,
                timings: response.timings,
                body: response.body,
            }, `API:${route}`);

            return response;
        },
    },
});

export const startGame = async () => {
    const response = await client('start_game');

    return {
        sessionId: response.body.session_id as string,
        questionsCount: response.body.before.questions_count as number,
    };
};

export const getQuestion = async (sessionId: string) => {
    const response = await client('get_question', {
        query: { session_id: sessionId },
    });

    if (response.body.pic === undefined)  {
        return null;
    }

    return {
        pictureUrl: response.body.pic as string,
        answers: response.body.options as number[],
    };
};

export const checkAnswer = async (sessionId: string, answer: number) => {
    const response = await client.post('check_question', {
        query: { session_id: sessionId },
        body: { answer },
    });

    return {
        isCorrect: Boolean(response.body.is_right_answer),
        correctAnswer: response.body.right_answer as number,
        aiAnswer: response.body.after.ai_answer as number,
    };
};

export const endGame = async (sessionId: string) => {
    const response = await client('end_game', {
        query: { session_id: sessionId },
    });

    if (response.body.result === undefined) {
        return null;
    }

    return {
        result: response.body.result as number,
        aiResult: response.body.game_after.ai_result as number,
    };
};
