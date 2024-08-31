from rtlog import null_logger
from alice.uniproxy.library.events import EventException
from .base_event_processor import EventProcessor


__all__ = ["EventProcessor"]


__event_processors = {}


def register_event_processor(cls):
    """ Decorator: add particular class into Event Processor's registry
    """

    module_name = cls.__module__.split(".")[-1]
    class_name = cls.__name__
    event_type = "{}.{}".format(module_name, class_name).lower()

    if event_type in __event_processors:
        raise SystemError("Event processor '{}' already exists".format(event_type))

    setattr(cls, "event_type", event_type)
    __event_processors[event_type] = cls
    return cls


def create_event_processor(system, event, rt_log=null_logger(), **kwargs):
    processor_type = __event_processors.get(event.event_type())
    if processor_type is None:
        raise EventException(
            "Unsupported event: %s.%s (%s)" % (event.namespace, event.name, event.event_type()),
            event_id=event.message_id,
        )
    else:
        return processor_type(system, rt_log or null_logger(), event.message_id, **kwargs)


if "__path__" in locals():
    import pkgutil
    import importlib

    # import all direct submodules to perform processors' registration
    for _, module_name, ispkg in pkgutil.iter_modules(__path__):
        if not ispkg:
            module = importlib.import_module("." + module_name, __package__)
