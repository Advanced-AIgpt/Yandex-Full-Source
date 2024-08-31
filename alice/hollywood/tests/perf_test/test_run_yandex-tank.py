import os
import yaml
import yatest.common
import load.projects.yatool_perf_test.lib.yandex_tank_module as yandex_tank_module


def prepare_conf(config):
    with open(config) as stream:
        data = yaml.safe_load(stream)

    for i, item in enumerate(data['pandora']['config_content']['pools']):
        if 'gun' in item:
            data['pandora']['config_content']['pools'][i]['gun']['target'] = 'localhost:' + os.environ.get('RECIPE_PORT')

    with open(config, 'w') as stream:
        yaml.dump(data, stream)


def test_const(links):
    config = yatest.common.source_path("alice/hollywood/tests/perf_test/load.yaml")
    sla_conf = yatest.common.source_path("alice/hollywood/tests/perf_test/sla.yaml")

    dosomething = prepare_conf(config)

    res_code, sla, upload_id = yandex_tank_module.run_yandex_tank(config, sla_conf)

    if upload_id is not None:
        report_html = '''
        <html>
        <p>You will be redirected to Datalens report</p>
        <script>
           window.location.replace("https://datalens.yandex-team.ru/u4wjkycfj2lup-perfreport?reportId={}")
        </script>
        </html>
        '''.format(upload_id)
        path = yatest.common.output_path("datalens.html")
        with open(path, "w") as f:
            f.write(report_html)
        links.set("Datalens", path)

    assert res_code == 0
    assert sla
