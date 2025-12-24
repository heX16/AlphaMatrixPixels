# Сборка GUI_Test (MSYS2 + Code::Blocks / консоль)

## 64‑битный вариант (рекомендуется)
- Установить SDL2, SDL_ttf и их зависимости в MinGW64:  
  `pacman -S mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-freetype mingw-w64-x86_64-harfbuzz mingw-w64-x86_64-graphite2 mingw-w64-x86_64-libpng mingw-w64-x86_64-zlib mingw-w64-x86_64-bzip2 mingw-w64-x86_64-brotli`
  
  Примечание: pacman обычно автоматически устанавливает зависимости, но если возникают ошибки линковки, установите их явно.
- В Code::Blocks выбрать тулчейн: `C:/msys64/mingw64/bin/g++.exe` (а не usr/bin).
- Пути (уже в проекте):  
  - Compiler search dirs: `C:/msys64/mingw64/include/SDL2`, `C:/msys64/mingw64/include`  
  - Linker search dirs: `C:/msys64/mingw64/lib`
- Линковка: `-lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lfreetype -lharfbuzz -lgraphite2 -lpng -lbz2 -lbrotlidec -lbrotlicommon -lz -lgdi32 -lsetupapi -lwinmm -limm32 -lole32 -loleaut32 -lrpcrt4 -ldwrite -lversion`.
- Запуск: положить `C:/msys64/mingw64/bin/SDL2.dll` и `C:/msys64/mingw64/bin/SDL2_ttf.dll` рядом с `HXLED_GUI_Test.exe` или добавить этот путь в `PATH`.

### Сборка из консоли (MinGW64 shell)
```
g++ main.cpp -std=c++17 -m64 -I.. ^
  -IC:/msys64/mingw64/include/SDL2 ^
  -LC:/msys64/mingw64/lib ^
  -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lfreetype -lharfbuzz -lgraphite2 -lpng -lbz2 -lbrotlidec -lbrotlicommon -lz -lgdi32 -lsetupapi -lwinmm -limm32 -lole32 -loleaut32 -lrpcrt4 -ldwrite -lversion ^
  -o gui_test.exe
```

## Полный список зависимостей проекта

### Основные библиотеки
- **SDL2** (`-lSDL2`, `-lSDL2main`) - графическая библиотека для отрисовки
- **SDL2_ttf** (`-lSDL2_ttf`) - библиотека для работы с TrueType шрифтами

### Зависимости SDL2_ttf
- **FreeType** (`-lfreetype`) - библиотека для рендеринга шрифтов
- **HarfBuzz** (`-lharfbuzz`) - библиотека для работы с текстом и шейпингом

### Зависимости FreeType
- **libpng** (`-lpng`) - поддержка PNG изображений в шрифтах
- **zlib** (`-lz`) - сжатие данных (inflate)
- **bzip2** (`-lbz2`) - поддержка BZ2 сжатия в шрифтах
- **Brotli** (`-lbrotlidec`, `-lbrotlicommon`) - поддержка Brotli сжатия в шрифтах

### Зависимости HarfBuzz
- **Graphite2** (`-lgraphite2`) - поддержка Graphite шрифтов
- **rpcrt4** (`-lrpcrt4`) - Windows API для UUID (используется Uniscribe)
- **dwrite** (`-ldwrite`) - DirectWrite API для работы с текстом в Windows

### Системные библиотеки Windows
- **mingw32** (`-lmingw32`) - MinGW runtime
- **gdi32** (`-lgdi32`) - Windows GDI API
- **setupapi** (`-lsetupapi`) - Windows Setup API
- **winmm** (`-lwinmm`) - Windows Multimedia API
- **imm32** (`-limm32`) - Input Method Manager
- **ole32** (`-lole32`) - OLE/COM API
- **oleaut32** (`-loleaut32`) - OLE Automation API
- **version** (`-lversion`) - Version Information API

## 32‑битный вариант (если нужен)
- Установить SDL2, SDL_ttf и их зависимости 32‑бит: `pacman -S mingw-w64-i686-SDL2 mingw-w64-i686-SDL2_ttf mingw-w64-i686-freetype mingw-w64-i686-harfbuzz mingw-w64-i686-graphite2 mingw-w64-i686-libpng mingw-w64-i686-zlib mingw-w64-i686-bzip2 mingw-w64-i686-brotli`
- Тулчейн: `C:/msys64/mingw32/bin/g++.exe`, флаг `-m32`
- Пути: include `C:/msys64/mingw32/include/SDL2`, lib `C:/msys64/mingw32/lib`
- Линковка: добавить `-lSDL2_ttf -lfreetype -lharfbuzz -lgraphite2 -lpng -lbz2 -lbrotlidec -lbrotlicommon -lz -lrpcrt4 -ldwrite` к списку библиотек
- DLL для запуска: `C:/msys64/mingw32/bin/SDL2.dll` и `C:/msys64/mingw32/bin/SDL2_ttf.dll`.

## Примечание про шрифты
Программа пытается загрузить системный шрифт Windows (Arial, Calibri или Consolas). 

