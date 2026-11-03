в nix отдельно нужно устанавливать nixGL для поиска функций драйвера видеокарты
```sh
nix run github:nix-community/nixGL#nixGLIntel -- \
  /path/to/3D-Visualizer/build/demo/renderTest
```
