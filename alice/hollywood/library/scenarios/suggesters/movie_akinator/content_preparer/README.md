# Content Preparer

Builds movie base in the format used by MovieAkinator scenario.

Run:
```bash
ya m -r --checkout --yt-store .
./load_movie_infos --video-items-path <> --auth-token <> --output-path movie_infos.json
python3 build_tree.py --movie-infos-path movie_infos.json --result-path clustered_movies.json
python3 render_poster_clouds.py --clustered-movies-path clustered_movies.json
rm movie_infos.json
ya upload clustered_movies.json --ttl inf
```

And update the resource id in ../resources/ya.inc.

You will need python 3 with the following libraries:
```
Package                       Version
----------------------------- ------------------
attrs                         19.1.0
numpy                         1.17.2
Pillow                        6.2.0
requests                      2.21.0
scikit-network                0.12.1
tqdm                          4.31.1
```
