txt = input1
values = [[str(int(num)) for num in range_.split('-')] for range_ in txt.split(',')]
txt = ':'.join(['-'.join(range_) for range_ in values])
output1 = {'ignored-features': txt}
