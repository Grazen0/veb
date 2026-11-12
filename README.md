# Van Emde Boas

Implementación en C++ del árbol de Van Emde Boas.

## Compilación y ejecución de tests

```bash
# Set-up y compilar
meson setup build
meson subprojects download
meson compile -C build

# Correr tests y benchmarks
meson test -C build
```
