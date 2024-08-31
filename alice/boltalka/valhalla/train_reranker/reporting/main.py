import json
import vh
from collections import OrderedDict
from alice.boltalka.tools.calc_reranker_metrics.utils import make_report


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_files=vh.mkinput(vh.File, nargs='+'), output_json=vh.mkoutput(vh.File),
         output_txt=vh.mkoutput(vh.File))
def make_total_report(input_files, output_json, output_txt):
    parsed_reports = []
    for input_file in input_files:
        with open(input_file, 'r') as f:
            parsed_reports.append(json.load(f, object_pairs_hook=OrderedDict))
    parsed_reports = sorted(parsed_reports, key=lambda x: x['logloss'])
    with open(output_json, 'w') as f:
        json.dump(parsed_reports, f)
    with open(output_txt, 'w') as f:
        f.write(make_report(parsed_reports))
    return output_json, output_txt
