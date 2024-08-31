# BeBeDer

BeBeDer is "Begemot Beggins Embedder".

## Params
- yt-proxy - yt proxy (default: hahn)
- input-table - yt table with only column *(text:string)*
- output-table - table will be overwritten with tuple *(text: string, normalized_text: string, sentence_embedding:List[float])*
- hosts - BeBe hosts
- rps - rps limit per BeBe host (default: 10)

## Usage
```bash
./bebeder --yt-proxy $yt_proxy --input-table $input_table --output-table $output_table --rps $rps $host [$another_host, ...]
```
```bash
./bebender -yt-proxy "hahn" -input-table "//tmp/input" -output-table "//tmp/output" --rps 21 ya.ru yandex.ru
```
