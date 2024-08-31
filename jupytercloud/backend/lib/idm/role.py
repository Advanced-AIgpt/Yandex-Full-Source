ROLE_TREE = {
    'slug': 'role',
    'name': {
        'ru': 'Роль',
        'en': 'Role',
    },
    'values': {
        'admin': {
            'name': {
                'ru': 'Администратор',
                'en': 'Administrator',
            },
            'help': {
                'ru': 'Права администратора JupyterHub',
                'en': 'Administrative access to JupyterHub',
            },
        },
        'quota': {
            'name': {
                'ru': 'Квота вычислительных ресурсов',
                'en': 'VM instances quota',
            },
            'help': {
                'ru': 'Необходима для доступа к квоте JupyterCloud',
                'en': 'Required to access JupyterCloud quota',
            },
            'roles': {
                'slug': 'vm',
                'name': {
                    'ru': 'Тип виртуальной машины',
                    'en': 'VM instance type',
                },
                'values': {
                    # NB: yes, old slugs - cpu1_ram4_hdd24 and new real values - 1/4/72ssd
                    'cpu1_ram4_hdd24': {
                        'name': '1x CPU, 4 GB RAM, 72 GB SSD',
                        'help': {
                            'ru': 'Не требует согласования',
                            'en': 'No approval required',
                        },
                    },
                    'cpu2_ram16_hdd48': {
                        'name': '2x CPU, 16 GB RAM, 144 GB SSD',
                        'help': {
                            'ru': 'Требует согласования',
                            'en': 'Approval required',
                        },
                    },
                    'cpu4_ram32_hdd96': {
                        'name': '4x CPU, 32 GB RAM, 288 GB SSD',
                        'help': {
                            'ru': 'Требует согласования',
                            'en': 'Approval required',
                        },
                    },
                },
                # NB: this field is currently unused, but I'm afraid to break
                # IDM synchronization with existing roles, so I keep it here for now
                'fields': [{
                    'slug': 'quota',
                    'name': {
                        'ru': 'Количество контейнеров (DEPRECATED)',
                        'en': 'Number of instances (DEPRECATED)',
                    },
                    'type': 'charfield',
                    'required': False,
                    'options': {
                        'blank_allowed': False,
                    },
                }],
            },
        },
    },
}
