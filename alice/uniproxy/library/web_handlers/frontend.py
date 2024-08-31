from alice.uniproxy.library.common_handlers import CommonRequestHandler
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.frontend import RESOURCES_ROOT, TemplateLoader


class SettingsHandler(CommonRequestHandler):
    unistat_handler_name = 'settings_js'

    def get(self):
        loader = TemplateLoader(RESOURCES_ROOT)
        try:
            self.content_type = "text/javascript"
            self.write(loader.load("settings.js").generate(
                apiKey=config["key"]
            ))
        except Exception as exc:
            Logger.get().debug(exc)
            self.set_status(404)


class TtsDemoHandler(CommonRequestHandler):
    unistat_handler_name = 'ttsdemo_html'

    def get(self, locale='ru'):
        locale = locale or 'ru'
        loader = TemplateLoader(RESOURCES_ROOT)
        try:
            self.content_type = "text/html"
            self.write(loader.load("ttsdemo.html").generate(
                voice=self.get_argument("speaker", "oksana" if locale == 'ru' else 'alyss'),
                speed=self.get_argument("speed", ""),
                emotion=self.get_argument("emotion", ""),
                text=self.get_argument(
                    "text",
                    u"Тестирование синтеза речи." if locale == 'ru' else u'Yandex text to speech system.'),
                lang=self.get_argument("lang", locale),
                apikey=config["key"],
                tr=lambda x, y: x if locale == 'ru' else y,
            ))
        except Exception:
            self.set_status(404)


class AsrDemoHandler(CommonRequestHandler):
    unistat_handler_name = 'asrdemo_html'

    def get(self, locale='ru'):
        locale = locale or 'ru'
        loader = TemplateLoader(RESOURCES_ROOT)
        try:
            self.content_type = "text/html"
            action = self.request.path.split('/')[-1]
            self.write(loader.load(action).generate(
                apikey=config["key"],
                lang=self.get_argument("lang", locale),
                tr=lambda x, y: x if locale == 'ru' else y,
                language=self.get_argument("language", "ru-RU"),
                model=self.get_argument("model", "notes"),
            ))
        except Exception:
            self.set_status(404)
