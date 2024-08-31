class SpawnControls {
    constructor(quota_name, clusters) {
        this.fields = ['cluster', 'account', 'segment', 'preset', 'network_id'];
        this.quota_name = quota_name;
        this.clusters = clusters;

        this.selects = {};
        this.info = {};

        this.fields.forEach(field => {
            let suffix = quota_name + '-' + field;
            this.selects[field] = $('#select-' + suffix);
            this.info[field] = $('#select-info-' + suffix);
        })

        this.button = $('#submit-button-' + quota_name);
        this.message_box = $('#message-box-' + quota_name);
    }

    get_selected_value(field) {
        return this.selects[field].find(':selected').val();
    }

    get_selected_cluster() {
        const name = this.get_selected_value('cluster');
        return this.clusters[name];
    }

    get_selected_account() {
        const name = this.get_selected_value('account');
        return this.get_selected_cluster().accounts[name];
    }

    get_selected_segment() {
        const name = this.get_selected_value('segment');
        return this.get_selected_account().segments[name];
    }

    get_selected_preset() {
        const name = this.get_selected_value('preset');
        return this.get_selected_segment().presets.all[name];
    }

    empty_select(field) {
        this.selects[field].empty();
    }

    empty_icons(field) {
        this.info[field].empty();
    }

    append_option(field, value, text, selected, available=true) {
        let select = this.selects[field];

        if (!available) {
            text += ' (not available)';
        }

        let option = $('<option/>')
            .attr({
                'selected': selected,
            })
            .val(value)
            .text(text)
            .appendTo(select);

        if (!available) {
            option.addClass("bg-danger");
        }
    }

    append_info_icon(field, icon_name, text, level=null, link=null) {
        let icon = $('<span/>')
            .addClass('form-control-static')
            .addClass('mdi mdi-24px mdi-' + icon_name)
            .attr({
                'data-toggle': 'tooltip',
                'title': text,
            })
            .tooltip({
                html: true,
                placement: 'auto',
            });

        if (level) {
            icon.addClass('text-' + level);
        }
        if (link) {
            $('<a/>')
                .attr({
                    href: link,
                    target: '_blank',
                })
                .append(icon)
                .appendTo(this.info[field]);
        } else {
            icon.appendTo(this.info[field]);
        }

    }

    create_message_panel(level, title) {
        let panel = $('<div/>')
            .addClass("panel panel-" + level)
            .appendTo(this.message_box);

        $('<div/>')
            .addClass('panel-heading')
            .append(
                $('<h3/>').text(title).addClass('panel-title')
            )
            .appendTo(panel);

        $('<ul/>')
            .attr('id', 'message-list-' + this.quota_name + '-' + level)
            .addClass('list-group')
            .appendTo(panel);
    }

    append_message(text, level) {
        let message_list = $('#message-list-' + this.quota_name + '-' + level);

        $('<li/>')
            .addClass('list-group-item')
            .html(text)
            .appendTo(message_list);
    }
}


$(function () {
    const QYP_PROFILE_LINK = "https://qyp.yandex-team.ru/profile";
    const BYTE_UNITS = [
        'B',
        'kB',
        'MB',
        'GB',
        'TB',
        'PB',
        'EB',
        'ZB',
        'YB'
    ];

    const quotas = (() => {
        let result = [];
        if (QUOTA.have_personal) {
            result.push(['personal', QUOTA.personal_clusters]);
        }
        if (QUOTA.have_service) {
            result.push(['service', QUOTA.service_clusters]);
        }
        result.push(['jupyter', QUOTA.jupyter_clusters]);
        return result;
    })();

    function get_first_available(entities) {
        let result = entities.find(([name, entity]) => {
            return entity.available;
        });

        if (!result) {
            result = entities[0];
        }
        return result[1];
    }

    function format_bytes(number) {
        let i = 0;
        while (number > 1024 && i < BYTE_UNITS.length - 1) {
            number = number / 1024;
            i++;
        }

        let pretty_number = number.toFixed(1) * 1;
        return pretty_number.toLocaleString() + ' ' + BYTE_UNITS[i];
    }

    function format_usage(obj) {
        let usage = obj.personal_usage || obj.usage;
        let limit = obj.personal_limits || obj.limits;

        let cpu_usage = Math.round(usage.cpu / 1000);
        let cpu_limit = Math.round(limit.cpu / 1000);
        let mem_usage = format_bytes(usage.mem);
        let mem_limit = format_bytes(limit.mem);
        let ssd_usage = format_bytes(usage.ssd);
        let ssd_limit = format_bytes(limit.ssd);
        let hdd_usage = format_bytes(usage.hdd);
        let hdd_limit = format_bytes(limit.hdd);

        let ssd_io_usage = format_bytes(usage.ssd_io);
        let ssd_io_limit = format_bytes(limit.ssd_io);
        let hdd_io_usage = format_bytes(usage.hdd_io);
        let hdd_io_limit = format_bytes(limit.hdd_io);

        return (
            `Used:<br>` +
            `${cpu_usage}/${cpu_limit} CPU<br>` +
            `${mem_usage}/${mem_limit} RAM<br>` +
            `${ssd_usage}/${ssd_limit} SSD<br>` +
            `${hdd_usage}/${hdd_limit} HDD<br>` +
            `${ssd_io_usage}/s / ${ssd_io_limit}/s SSD io<br>` +
            `${hdd_io_usage}/s / ${hdd_io_limit}/s HDD io`
        );
    }

    function format_preset(preset) {
        let cpu = preset.cpu / 1000;
        let mem = format_bytes(preset.mem);
        let disk = format_bytes(preset.disk_size);
        let disk_type = preset.disk_type.toUpperCase();

        return `${cpu} CPU, ${mem} RAM, ${disk} ${disk_type}`;
    }

    function sort_entities(dict, func) {
        let items = Object.keys(dict).map(function(key) {
              return [key, dict[key]];
        });
        items.sort(func);
        return items;
    }

    function fill_defaults(tab_name, clusters) {
        let controls = new SpawnControls(tab_name, clusters);

        function change_cluster() {
            const cluster = controls.get_selected_cluster();
            const accounts = sort_entities(cluster.accounts);
            const default_ = get_first_available(accounts);

            controls.empty_icons('cluster');
            controls.empty_select('account');
            accounts.forEach(([account_name, account]) => {
                controls.append_option(
                    'account',
                    account_name,
                    account.readable_name,
                    (account_name == default_.name),
                    account.available,
                );
            });

            change_account();
        }

        function change_account() {
            const account = controls.get_selected_account();
            const segments = sort_entities(
                account.segments,
                ([name1, segment1], [name2, segment2]) => {
                    return name1.toLowerCase() == 'dev' ? -1 : 1;
                },
            );
            const default_ = get_first_available(segments);

            controls.empty_icons('account');
            controls.empty_select('segment');
            segments.forEach(([segment_name, segment]) => {
                controls.append_option(
                    'segment',
                    segment_name,
                    segment_name.toUpperCase(),
                    (segment_name == default_.name),
                    segment.available,
                )
            });

            controls.append_info_icon(
                'account', 'information-outline', 'Open ABC', 'info', account.abc_link
            );

            change_segment();
        }

        function change_segment() {
            const segment = controls.get_selected_segment();
            const presets = sort_entities(
                segment.presets.all,
                ([name1, preset1], [name2, preset2]) => {
                    if (preset1.disk_type != preset2.disk_type) {
                        return preset1.disk_type == 'ssd' ? -1 : 1;
                    }
                    if (preset1.cpu != preset2.cpu) {
                        return preset1.cpu - preset2.cpu;
                    }
                    if (preset1.mem != preset2.mem) {
                        return preset1.mem - preset2.mem;
                    }
                    if (preset1.disk != preset2.disk) {
                        return preset1.disk - preset2.disk;
                    }
                    return 0;
                }
            );

            const default_ = get_first_available(presets);

            controls.empty_icons('segment');
            controls.empty_select('preset');
            presets.forEach(([preset_name, preset]) => {
                controls.append_option(
                    'preset',
                    preset_name,
                    format_preset(preset),
                    (preset_name == default_.name),
                    preset.available,
                );
            });
            const usage = format_usage(segment);

            let profile_link = tab_name == 'jupyter' ? null : QYP_PROFILE_LINK;
            controls.append_info_icon('segment', 'chart-pie', usage, 'info', profile_link);

            change_preset();
        }

        function change_preset() {
            const preset = controls.get_selected_preset();
            const preset_name = controls.get_selected_value('preset');

            controls.button.attr('disabled', !preset.available || !AVAILABLE_NETWORKS.length);
            controls.button.attr('title', preset.available ? '' : 'Look at error messages above');
            controls.empty_icons('preset');
            controls.message_box.empty();

            let notes_by_level = {};
            for (let note of preset.notes) {
                notes_by_level[note.level] = notes_by_level[note.level] || [];
                notes_by_level[note.level].push(note.text);
            }

            $.each(notes_by_level, (level, notes) => {
                let title = {
                    danger: 'Errors',
                    warning: 'Warnings',
                }[level];
                controls.create_message_panel(level, title);

                for (let note of notes) {
                    controls.append_message(note, level);
                }

                const text = notes.join(';<br/>');
                controls.append_info_icon('preset', 'alert-circle', text, level);
            });

            if (!AVAILABLE_NETWORKS.length) {
                const level = 'danger';
                if (!notes_by_level.danger) {
                    controls.create_message_panel(level, 'Errors');
                }
                const text = "You don't have any available network to spawn into.";
                const note = (
                    text +
                    "<br/>" +
                    `Please refer to <a href="${NETWORKS_DOC_URL}" target="_blank">` +
                    "networks documentation</a>."
                );
                controls.append_message(note, level);
                controls.append_info_icon('network_id', 'alert-circle', text, level);
            }
        }

        const sorted_clusters = sort_entities(clusters);
        const default_ = get_first_available(sorted_clusters);

        controls.empty_select('cluster');
        sorted_clusters.forEach(([cluster_name, cluster]) => {
            controls.append_option(
                'cluster',
                cluster_name,
                cluster_name.toUpperCase(),
                (cluster_name == default_.name),
                cluster.available,
            );
        });

        change_cluster();

        controls.selects.cluster.change(change_cluster);
        controls.selects.account.change(change_account);
        controls.selects.segment.change(change_segment);
        controls.selects.preset.change(change_preset);

        if (default_.available) {
            return true;
        }
        return false;
    }

    let first_tab = null;
    quotas.forEach(([name, clusters]) => {
        const result = fill_defaults(name, clusters);

        if (result && !first_tab) {
            first_tab = name;
        }
    });

    if (first_tab) {
        $('#tab-header-' + first_tab).tab('show');
    }
});
