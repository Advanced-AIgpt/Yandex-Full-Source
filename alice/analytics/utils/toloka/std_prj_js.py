#!/usr/bin/env python
# encoding: utf-8
from __future__ import unicode_literals
from string import Template


class JsConstructor(object):
    """
    Вставки js кода в StdProject
    """
    def __init__(self):
        self.validators = []
        self.listeners = []

    def add_field(self, field):
        self.validators.append(self.make_validator(field))
        self.listeners.append(self.make_listener(field))

    def to_code(self):
        if not self.validators:
            return ''

        return self.code_wrapper.substitute(
            validators=''.join(self.validators),
            listeners=''.join(self.listeners),
        )

    # Общий шаблон

    code_wrapper = Template("""
exports.Task = extend(TolokaHandlebarsTask, function (options) {
  TolokaHandlebarsTask.call(this, options);
}, {
  validate: function(solution) {
    ${validators}
    return TolokaHandlebarsTask.prototype.validate.apply(this, arguments);
  },

  onRender: function() {
    ${listeners}
  }
});

function extend(ParentClass, constructorFunction, prototypeHash) {
    constructorFunction = constructorFunction || function () {};
    prototypeHash = prototypeHash || {};

    if (ParentClass) {
        constructorFunction.prototype = Object.create(ParentClass.prototype);
    }
    for (var i in prototypeHash) {
        constructorFunction.prototype[i] = prototypeHash[i];
    }
    return constructorFunction;
}
""")

    # Валидаторы

    validator_tmpl = Template("""
    if (!this.${fieldname}Played) {
      return {
        task_id: this.getTask().id,
        errors: {
          "__TASK__": {
            message: "${message}"
          }
        }
      };
    };
""")

    messages = {
        'audio': 'Вы не прослушали записи',
        'video': 'Вы не просмотрели видео',
    }

    def make_validator(self, field):
        return self.validator_tmpl.substitute(
            fieldname=field['name'],
            message=self.messages[field['type']],
        )

    # Рендереры

    listener_tmpl = Template("""
    var widget = this.getDOMElement().querySelector("#${fieldname}_player");

    widget.addEventListener("play", function(e) {
      this.${fieldname}Played = true;
    }.bind(this));

    widget.addEventListener("error", function(e) {
      var error = document.createElement("div");

      error.innerHTML = widget.innerHTML;
      widget.parentNode.replaceChild(error, widget);
    });
""")

    def make_listener(self, field):
        return self.listener_tmpl.substitute(fieldname=field['name'])

