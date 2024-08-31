slices = ''
with open(v, 'r') as f:
    slices = f.read().strip()
res = {
    "model_metadata": [
        'Slices "{}"'.format(slices)
    ]
}
return res
