# Примеры навыков Алисы

Чтобы вам было проще писать и запускать свои навыки, мы написали примеры и эту инструкцию.

### "Купи слона" на Python
Предлагает пользователю купить слона, пока он не согласится. Для создания веб-приложения используется Flask. 

[Исходный код]()
Код обработки запроса навыка на Python находится в файле `_имя папки_/elephant/elephant.py`.

### Попугай на node.js
Отвечает пользователю его же словами. Для запуска асинхронного сервиса используется пакет micro. 

[Исходный код]()
Код обработки запроса навыка на Node.js находится в файле `_имя папки_/parrot/index.js`, код конфигурации — в файле `_имя папки_/parrot/package.json`.

### Навык умного дома на Python
Пример провайдера с лампочками, розеткой и кондиционером. Считает, что существует только один пользователь *alone_user*\*, отвечает на его запросы и отображает текущее состояние устройств провайдера на сайте. Для создания веб-приложения используется Flask и gunicorn, для интерфейса — css и react.js. 

[Исходный код]()
Код обработки запросов навыка умного дома на Python (`_имя папки_/smart_home`) находится в файле `_имя папки_/smart_home/smart_home_api.py`, код конфигурации — в файле `_имя папки_/smart_home/config.py`.

\*если вы хотите поддерживать большее количество пользователей, то используйте базу данных и настройте авторизацию

# Локальный запуск и тестирование навыков

Вы можете запустить и самостоятельно протестировать навыки прямо на своем компьютере.

## Скачайте код навыков
Чтобы развернуть навыки, скачайте или склонируйте код [репозитория на GitHub]():
`git clone ...`
У вас должна получиться папка _имя папки_. 

## Запустите навыки
Находясь в папке _имя папки_, соберите образы навыков командой 
`sudo docker-compose build`
и запустите с помощью 
`sudo docker-compose up -d`

Ваши навыки доступны только вам по адресу [0.0.0.0](http://0.0.0.0). На нем отображается текущее состояние устройств из навыка умного дома.

## Протестируйте
Теперь вы можете отправлять запросы (например, с помощью программы [postman](https://www.postman.com)) на следующие адреса:

0.0.0.0/elephant — чтобы проверить работу навыка "Купи слона"

Пример запроса:
``{
	"version":"1.0",
	"session": {
		"user_id": "AC9WC3DF6FCE052E45A4566A48E6B7193774B84814CE49A922E163B8B29881DC",
		"new": true
	},
	"request": {
		"original_utterance": ""
	}
}``
Пример ответа:
``{
  "version": "1.0",
  "session": {
    "user_id": "AC9WC3DF6FCE052E45A4566A48E6B7193774B84814CE49A922E163B8B29881DC",
    "new": true
  },
  "response": {
    "end_session": false,
    "text": "Привет! Купи слона!",
    "buttons": [
      {
        "title": "Не хочу.",
        "hide": true
      },
      {
        "title": "Не буду.",
        "hide": true
      }
    ]
  }
}``


0.0.0.0/parrot — чтобы проверить работу навыка Попугай

Пример запроса:
``{
	"version":"1.0",
	"session": {
		"user_id": "AC9WC3DF6FCE052E45A4566A48E6B7193774B84814CE49A922E163B8B29881DC",
		"new": true
	},
	"request": {
		"original_utterance": "cock-a-doodle-doo"
	}
}``
Пример ответа:
``{
    "version": "1.0",
    "session": {
        "user_id": "AC9WC3DF6FCE052E45A4566A48E6B7193774B84814CE49A922E163B8B29881DC",
        "new": true
    },
    "response": {
        "text": "cock-a-doodle-doo",
        "end_session": false
    }
}``

Чтобы убедиться в работе наывка умного дома, проверьте запросы на следующие адреса:
* 0.0.0.0/v1.0
* 0.0.0.0/v1.0/user/unlink
Пример запроса:
**Headers:**
``Authorization:123qwe456a
X-Request-Id:ff36a3cc-ec34-11e6-b1a0-64510650abcf``

Пример ответа:
``{
    "request_id": "ff36a3cc-ec34-11e6-b1a0-64510650abcf"
}``

* 0.0.0.0/v1.0/user/devices
Пример запроса:
**Headers:**
``Authorization:123qwe456a
X-Request-Id:ff36a3cc-ec34-11e6-b1a0-64510650abcf``

Пример ответа:
``{
    "payload": {
        "devices": [
            {
                "capabilities": [
                    {
                        "retrievable": true,
                        "type": "devices.capabilities.on_off"
                    },
                    {
                        "parameters": {
                            "color_model": "hsv",
                            "temperature_k": {
                                "max": 9000,
                                "min": 2000
                            }
                        },
                        "retrievable": true,
                        "type": "devices.capabilities.color_setting"
                    },
                    {
                        "parameters": {
                            "instance": "brightness",
                            "range": {
                                "max": 100,
                                "min": 0,
                                "precision": 10
                            },
                            "unit": "unit.percent"
                        },
                        "retrievable": true,
                        "type": "devices.capabilities.range"
                    }
                ],
                "id": "1",
                "name": "Лампа Обыкновенная",
                "type": "devices.types.light"
            },
            {
                "capabilities": [
                    {
                        "retrievable": true,
                        "type": "devices.capabilities.on_off"
                    },
                    {
                        "parameters": {
                            "color_model": "hsv",
                            "temperature_k": {
                                "max": 9000,
                                "min": 2000
                            }
                        },
                        "retrievable": true,
                        "type": "devices.capabilities.color_setting"
                    },
                    {
                        "parameters": {
                            "instance": "brightness",
                            "range": {
                                "max": 100,
                                "min": 0,
                                "precision": 10
                            },
                            "unit": "unit.percent"
                        },
                        "retrievable": true,
                        "type": "devices.capabilities.range"
                    }
                ],
                "id": "2",
                "name": "Лампа Обыкновенная",
                "type": "devices.types.light"
            },
            {
                "capabilities": [
                    {
                        "retrievable": true,
                        "type": "devices.capabilities.on_off"
                    },
                    {
                        "parameters": {
                            "color_model": "hsv",
                            "temperature_k": {
                                "max": 9000,
                                "min": 2000
                            }
                        },
                        "retrievable": true,
                        "type": "devices.capabilities.color_setting"
                    },
                    {
                        "parameters": {
                            "instance": "brightness",
                            "range": {
                                "max": 100,
                                "min": 0,
                                "precision": 10
                            },
                            "unit": "unit.percent"
                        },
                        "retrievable": true,
                        "type": "devices.capabilities.range"
                    }
                ],
                "id": "3",
                "name": "Лампа Обыкновенная",
                "type": "devices.types.light"
            },
            {
                "capabilities": [
                    {
                        "retrievable": true,
                        "type": "devices.capabilities.on_off"
                    },
                    {
                        "parameters": {
                            "instance": "backlight"
                        },
                        "retrievable": true,
                        "type": "devices.capabilities.toggle"
                    }
                ],
                "id": "4",
                "name": "Розетка С Подсветочкой",
                "type": "devices.types.socket"
            },
            {
                "capabilities": [
                    {
                        "retrievable": true,
                        "type": "devices.capabilities.on_off"
                    },
                    {
                        "parameters": {
                            "instance": "temperature",
                            "range": {
                                "max": 40,
                                "min": -10,
                                "precision": 1
                            },
                            "unit": "unit.temperature.celsius"
                        },
                        "retrievable": true,
                        "type": "devices.capabilities.range"
                    },
                    {
                        "parameters": {
                            "instance": "fan_speed",
                            "modes": [
                                {
                                    "value": "low"
                                },
                                {
                                    "value": "medium"
                                },
                                {
                                    "value": "high"
                                }
                            ]
                        },
                        "retrievable": true,
                        "type": "devices.capabilities.mode"
                    },
                    {
                        "parameters": {
                            "instance": "swing",
                            "modes": [
                                {
                                    "value": "vertical"
                                },
                                {
                                    "value": "stationary"
                                }
                            ]
                        },
                        "retrievable": true,
                        "type": "devices.capabilities.mode"
                    },
                    {
                        "parameters": {
                            "instance": "thermostat",
                            "modes": [
                                {
                                    "value": "auto"
                                },
                                {
                                    "value": "cool"
                                },
                                {
                                    "value": "heat"
                                }
                            ]
                        },
                        "retrievable": true,
                        "type": "devices.capabilities.mode"
                    }
                ],
                "id": "5",
                "name": "Кондиционер Обыкновенный",
                "type": "devices.types.thermostat.ac"
            }
        ],
        "user_id": "alone_user"
    },
    "request_id": "ff36a3cc-ec34-11e6-b1a0-64510650abcf"
}``
* 0.0.0.0/v1.0/user/devices/query
Пример зпроса:
**Headers:**
``Authorization:123qwe456a
Content-Type:application/json
X-Request-Id:ff36a3cc-ec34-11e6-b1a0-64510650abcf``
**Body:**
``{
    "devices": [
    	{
			"id": "1",
			"custom_data": {}
		},
		{
			"id": "4",
			"custom_data": {}
		}
    ]
}``

Пример ответа:
``{
    "payload": {
        "devices": [
            {
                "capabilities": [
                    {
                        "state": {
                            "instance": "on",
                            "value": true
                        },
                        "type": "devices.capabilities.on_off"
                    },
                    {
                        "state": {
                            "instance": "hsv",
                            "value": {
                                "h": 179,
                                "s": 57,
                                "v": 2
                            }
                        },
                        "type": "devices.capabilities.color_setting"
                    },
                    {
                        "state": {
                            "instance": "brightness",
                            "value": 50
                        },
                        "type": "devices.capabilities.range"
                    }
                ],
                "id": "1"
            },
            {
                "capabilities": [
                    {
                        "state": {
                            "instance": "on",
                            "value": true
                        },
                        "type": "devices.capabilities.on_off"
                    },
                    {
                        "state": {
                            "instance": "backlight",
                            "value": false
                        },
                        "type": "devices.capabilities.toggle"
                    }
                ],
                "id": "4"
            }
        ]
    },
    "request_id": "ff36a3cc-ec34-11e6-b1a0-64510650abcf"
}``

* 0.0.0.0/v1.0/user/devices/action
Пример запроса:
**Headers:**
``Authorization:123qwe456a
Content-Type:application/json
X-Request-Id:ff36a3cc-ec34-11e6-b1a0-64510650abcf``
**Body:**
``{
	"payload": {
	    "devices": [
			{
				"id": "2",
				"custom_data": {},
				"capabilities": [
					{
			            "type": "devices.capabilities.color_setting",
			            "state": {
			                "instance": "hsv",
			                "value": {
			                	"h": 100, 
			                	"s": 100, 
			                	"v": 100
			                }
			            }
			        }
		        ]
		    },
		    {
				"id": "5",
				"custom_data": {},
				"capabilities": [
					{
			            "type": "devices.capabilities.mode",
			            "state": {
			                "instance": "fan_speed",
			                "value": "low"
			            }
			        }
		        ]
		    }
	    ]
	}
}``

Пример ответа:
``{
    "payload": {
        "devices": [
            {
                "capabilities": [
                    {
                        "state": {
                            "action_result": {
                                "status": "DONE"
                            },
                            "instance": "hsv"
                        },
                        "type": "devices.capabilities.color_setting"
                    }
                ],
                "id": "2"
            },
            {
                "capabilities": [
                    {
                        "state": {
                            "action_result": {
                                "status": "DONE"
                            },
                            "instance": "fan_speed"
                        },
                        "type": "devices.capabilities.mode"
                    }
                ],
                "id": "5"
            }
        ]
    },
    "request_id": "ff36a3cc-ec34-11e6-b1a0-64510650abcf"
}``

На [0.0.0.0](http://0.0.0.0) вы можете посмотреть текущее состояние устройств из навыка умного дома.

Более подробно про протокол работы навыков общего типа [здесь](https://yandex.ru/dev/dialogs/alice/doc/protocol-docpage/), про навыки умного дома [здесь](https://yandex.ru/dev/dialogs/alice/doc/smart-home/reference/resources-docpage/).
