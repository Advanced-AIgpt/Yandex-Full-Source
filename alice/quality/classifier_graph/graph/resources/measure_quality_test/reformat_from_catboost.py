ans = []
if len(v) <= 1:
    return ans
for i in range(0, len(v)):
    # _, label, text, group, predict = row.rstrip().split('\t')
    ans.append([str(i), v[i][2], v[i][0], v[i][3], v[i][1]])
return ans
