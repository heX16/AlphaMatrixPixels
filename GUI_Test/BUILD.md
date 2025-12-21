# Сборка GUI_Test (MSYS2 + Code::Blocks / консоль)

## 64‑битный вариант (рекомендуется)
- Установить SDL2 в MinGW64:  
  `pacman -S mingw-w64-x86_64-SDL2`
- В Code::Blocks выбрать тулчейн: `C:/msys64/mingw64/bin/g++.exe` (а не usr/bin).
- Пути (уже в проекте):  
  - Compiler search dirs: `C:/msys64/mingw64/include/SDL2`, `C:/msys64/mingw64/include`  
  - Linker search dirs: `C:/msys64/mingw64/lib`
- Линковка: `-lmingw32 -lSDL2main -lSDL2 -lgdi32 -lsetupapi -lwinmm -limm32 -lole32 -loleaut32 -lversion`.
- Запуск: положить `C:/msys64/mingw64/bin/SDL2.dll` рядом с `HXLED_GUI_Test.exe` или добавить этот путь в `PATH`.

### Сборка из консоли (MinGW64 shell)
```
g++ main.cpp -std=c++17 -m64 -I.. ^
  -IC:/msys64/mingw64/include/SDL2 ^
  -LC:/msys64/mingw64/lib ^
  -lmingw32 -lSDL2main -lSDL2 -lgdi32 -lsetupapi -lwinmm -limm32 -lole32 -loleaut32 -lversion ^
  -o gui_test.exe
```

## 32‑битный вариант (если нужен)
- Установить SDL2 32‑бит: `pacman -S mingw-w64-i686-SDL2`
- Тулчейн: `C:/msys64/mingw32/bin/g++.exe`, флаг `-m32`
- Пути: include `C:/msys64/mingw32/include/SDL2`, lib `C:/msys64/mingw32/lib`
- DLL для запуска: `C:/msys64/mingw32/bin/SDL2.dll`.

