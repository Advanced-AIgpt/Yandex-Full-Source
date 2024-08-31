import json

import tornado.web

from .base import JCAPIHandler, JCPageHandler


SAVE_TOKEN_URI = 'yandex_oauth_save'


class YandexOAuthHandler(JCPageHandler):
    @tornado.web.authenticated
    async def get(self):
        if not self.current_user:
            raise tornado.web.HTTPError(
                401, 'Failed to identify username, possibly because missing referrer',
            )

        html = await self.render_template(
            'yandex_oauth.html',
            save_token_url=f'/hub/api/{SAVE_TOKEN_URI}',
            redirect_url=f'/hub/spawn/{self.current_user.name}',
            oauth_url=self.oauth.get_oauth_url(),
        )
        self.write(html)


class YandexOAuthSaveHandler(JCAPIHandler):
    async def post(self):
        if not self.current_user:
            raise tornado.web.HTTPError(
                401, 'Failed to identify username, possibly because missing referrer',
            )

        params = self.get_json_body()
        access_token = params.get('access_token')
        if access_token is None:
            raise tornado.web.HTTPError(
                400, 'missing access_token in request body',
            )

        self.oauth.add_or_update_jupyter_cloud_token(access_token)

        result = {'success': True}
        # place for future extending of using state field
        state_json = params.get('state')
        if state_json:
            state = json.loads(state_json)

            close = state.get('close')
            if close:
                result['close'] = close

            redirect = state.get('redirect')
            if redirect:
                result['redirect'] = redirect

        self.write(result)

    def finish(self, *args, **kwargs):
        """Roll back any uncommitted transactions from the handler."""
        if self._oauth:
            self._oauth.jupyter_cloud_db.clean_dirty()
        super().finish(*args, **kwargs)


default_handlers = [
    ('/yandex_oauth', YandexOAuthHandler),
    (f'/api/{SAVE_TOKEN_URI}', YandexOAuthSaveHandler),
]
