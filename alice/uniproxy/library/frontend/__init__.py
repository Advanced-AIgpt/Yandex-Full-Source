import sys
import os
import tornado.template
import tornado.web
from alice.uniproxy.library.common_handlers import CommonRequestHandler
from voicetech.infra.library.tornado_resource_handlers import ResourceHandler, ResourceTemplateLoader


if getattr(sys, "is_standalone_binary", False):

    RESOURCES_ROOT = "/frontend"

    class FileHandler(CommonRequestHandler, ResourceHandler):
        pass

    TemplateLoader = ResourceTemplateLoader

else:
    # for Docker image

    RESOURCES_ROOT = os.path.join(os.path.dirname(__file__), "resources")
    FileHandler = tornado.web.StaticFileHandler
    TemplateLoader = tornado.template.Loader
