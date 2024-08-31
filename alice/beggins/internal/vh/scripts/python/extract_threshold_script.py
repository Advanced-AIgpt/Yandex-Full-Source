def main(in1, in2, in3, mr_tables, token1=None, token2=None, param1=None, param2=None, html_file=None):
    threshold_meta = in1[0] if in1 else {}
    return dict(threshold=threshold_meta.get('threshold', 0.5))
