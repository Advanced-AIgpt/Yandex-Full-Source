#!/usr/bin/env python
# encoding: utf-8
from django.forms import Form, TextInput, CharField
from django.core.validators import RegexValidator


class DialogSelectorForm(Form):
    date = CharField(validators=[RegexValidator(r'\d{4}-\d{2}-\d{2}')],
                     required=False)
    uuid = CharField(required=False)
    text = CharField(required=False)
    intent = CharField(required=False)

