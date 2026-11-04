Входные файлы для просмотра пересечений:

```bash
nix develop
cmake --preset app -DVISUALIZER_SOURCE_DIR="$PWD/../3D-Visualizer"
cmake --build --preset app
nixGLIntel ./build/app/triangles inputs/input_100.txt
```

Красные треугольники пересекаются, серые — нет. Окно открывается всегда.
ЛКМ + движение мыши — вращение, ПКМ — перемещение, колесо — масштаб.
Камера изначально настроена на координаты в диапазоне [-10, 10].

Генерация произвольного числа треугольников:
```bash
python3 scripts/generate_input.py 250 inputs/random_250.txt --min -50 --max 50
python3 scripts/generate_input.py 100 inputs/repeatable.txt --seed 42
```

Диапазон координат задаётся через `--min` и `--max` (по умолчанию [-10, 10]). `--seed` позволяет повторить генерацию.
