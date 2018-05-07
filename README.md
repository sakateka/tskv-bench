# tskv-bench
Простой тест скорости рабора строк из файла на разных языках программирования.
Не претендует на 100% справедливость.

![2018-05-07_14 00 48](https://user-images.githubusercontent.com/2256154/39688999-2a9c1750-51ff-11e8-8dfa-529b41463278.png)


Более точное сравнение производительности `rust` и `C` версий
```
In [10]: %timeit -n 15 subprocess.run(["./tskv-rs/target/release/tskv-rs"], stdout=open(os.devnull))
351 ms ± 8.3 ms per loop (mean ± std. dev. of 7 runs, 15 loops each)

In [11]: %timeit -n 15 subprocess.run(["./tskv-c"], stdout=open(os.devnull))
356 ms ± 5.38 ms per loop (mean ± std. dev. of 7 runs, 15 loops each)
```
