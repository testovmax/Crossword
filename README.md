
## Сборка

### Вариант 1: qmake (Qt Creator)

1. Открыть `crossword.pro` в Qt Creator.
2. Выбрать набор инструментов (Kit) с Qt 5.15+ или Qt 6.
3. Нажать **Build → Run** (`Ctrl+R`).

Из командной строки:

```bash
qmake crossword.pro
make           
./crossword
```

### Вариант 2: CMake

```bash
cmake -B build
cmake --build build
./build/crossword
```
