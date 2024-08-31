Efficient BPE (Byte Pair Encoding) implementation for NLP tasks
=====================================================
Please refer to https://arxiv.org/pdf/1508.07909.pdf for details on Machine Translation applications.

The input to the algorithm consists of atmost two tab-separated columns:
- first column comprises space separated bytes/characters/tokens - the "bytes" for BPE algorithm
- second column is optional. If present, it should contain a single integer number - frequency of this sequence of "bytes"

In order to build sub-word units (like in the paper above) `dataset`-file might look like this:
```
.       308942560
,       239489904
н е     67498471
?       66181258
и       59661173
я       49499482
в       46357755
–       45444759
ч т о   45154924
!       39036194
а       29576582
н а     26707170
т ы     25837329
э т о   23054565
```

In order to build sub-sentence units (word-ngramms) dataset file might look like this:
```
пятюню давай прям
возмущаюсь , так как пара людей взбесила меня этой темой !
смотри у меня !
что именно
это было слишком неожиданно . все же было в порядке тт
а чо так ? : с
неет
что то с детьми : ) ) ) )
учись у него , лоал с :
пфффф конечно ) ) спрашиваешь ) это же рай на земле ) как мы можем упустить такую возможность )
красиво . . . и очень верно !
правильно : )
моя любимая
```

Produces two files:
- `alphabet-output`: `alphabet-size` most frequent tokens in the dataset
- `merges-output`: merges of the BPE algorithm in the order of decreasing frequency in the following format:
```
idOfLeftAlphabetSymbol \t idOfRightAlphabetSymbol \t pairFrequency
```

In the merges-output file two ids have special meaning:
- id = `alphabet-size` : end of token / end of sentence symbol
- id = `alphabet-size` + 1 : id of UNK (unknown) symbol (could be present only if skip-unknown option is not set)

All ids are 0-based.
