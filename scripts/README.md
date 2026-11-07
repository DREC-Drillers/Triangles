# Скрипты

Основные команды сборки и запуска описаны в [README проекта](../README.md).

- `code_quality.py` — форматирование, clang-tidy и исправление именования.
- `benchmark_intersections.py` — измерение времени обработки и построение графика.

```bash
python3 scripts/code_quality.py --help
python3 scripts/benchmark_intersections.py --help
```

Для clang-tidy сначала подготовь compilation database и C++-модули:

```bash
cmake --preset quality
cmake --build --preset quality
python3 scripts/code_quality.py tidy --build-dir build/quality
```

Core Guidelines включаются отдельно через `--cppcoreguidelines` и остаются
предупреждениями. `style-fix` меняет имена, в том числе публичные; проверяй diff
перед коммитом. Новые C++-файлы нужно добавить в Git для включения в проверки.
