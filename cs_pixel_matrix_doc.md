# Документация `cs_pixel_matrix`

**Название проекта:** MatrixPixels

## Назначение
`cs_pixel_matrix.hpp` — чистая математическая 2D-матрица пикселей в формате ARGB (0xAARRGGBB) со straight-alpha и композицией Porter-Duff SourceOver. Не привязана к железу, пригодна для моделирования/рендера в памяти.

## Основные типы
- `csColorRGBA` — упакованный ARGB (A в старшем байте). Размер гарантирован 4 байта (packed/pragma + static_assert).
- `tPixelMatrixCoord` (`int32_t`) — координаты (допускает отрицательные смещения для клиппинга).
- `tPixelMatrixSize` (`uint16_t`) — размеры матрицы.

### Особенности `csColorRGBA`
- Конструктор из `uint32_t packed`:
  - Если альфа = 0, вход трактуется как `0xRRGGBB` и альфа принудительно становится `0xFF`.
  - Иначе байты интерпретируются как есть (0xAARRGGBB).
- Оператор `/=` делит все каналы (включая alpha) целочисленно:
  ```cpp
  csColorRGBA c{200,100,50,255};
  c /= 2; // -> {100,50,25,127}
  ```
- Унарный `-` инвертирует все каналы (alpha тоже).
- Статические методы:
  - `blendChannel` — смешивает один канал с учётом premult и итоговой альфы.
  - `sourceOverStraight(dst, src, global_alpha)` — SourceOver со глобальной альфой-множителем.
  - `sourceOverStraight(dst, src)` — SourceOver, используя только `src.a`.

## Класс `csPixelMatrix`
Хранит буфер `width * height` (динамический `new[]`), операции вне границ — тихо игнорируются (геттер возвращает прозрачный чёрный).

### Конструкторы/присваивания
- `csPixelMatrix(w, h)` — инициализация нулями.
- Копирование/перемещение и соответствующие операторы поддерживаются (глубокая копия или перенос буфера).

### Методы
- `width(), height()` — размеры.
- `setPixel(x, y, color)` — запись без смешивания.
- `setPixelBlend(x, y, color, alpha)` — SourceOver, альфа источника умножена на `alpha`.
- `setPixelBlend(x, y, color)` — SourceOver, только альфа пикселя.
- `getPixel(x, y)` — цвет или {0,0,0,0} вне границ.
- `getPixelBlend(x, y, bg)` — возвращает результат SourceOver( bg, pixel(x,y) ), вне границ вернёт `bg`.
- `drawMatrix(dx, dy, source, alpha=255)` — рисует `source` поверх с клиппингом, альфа-множитель общий.

### Пример использования
```cpp
#include "cs_pixel_matrix.hpp"

csPixelMatrix m(4, 3);
csColorRGBA red{255, 0, 0, 255};
csColorRGBA semi{0, 0, 255, 128}; // полупрозрачный синий

m.setPixel(1, 1, red);
m.setPixelBlend(1, 1, semi);          // синее поверх красного, учтёт alpha пикселя
m.setPixelBlend(2, 1, semi, 64);      // дополнительно ослабит источник

csColorRGBA bg{0x202020};             // 0xFF202020 из-за автоподнятия альфы
csColorRGBA blended = m.getPixelBlend(1, 1, bg);
```

### Пример `drawMatrix` с клиппингом
```cpp
csPixelMatrix dest(8, 8);
csPixelMatrix src(4, 4);
src.setPixel(0, 0, csColorRGBA{255,255,0,128});
dest.drawMatrix(-1, -1, src, 200); // частично выйдет за границы, будет обрезано
```

## Замечания по бинарной совместимости
- `csColorRGBA` упакован до 4 байт (pragma/attribute + static_assert).
- Union: читать/писать последовательно через одно представление (`value` или поля), чтобы избегать сюрпризов на экзотических ABI.
- Формат ARGB: порядок байт A,R,G,B при доступе через `raw`/`value` соответствует `0xAARRGGBB`.

## Алгоритм смешивания (для одного канала, нормализовано к 0..255)
```
As = mul8(src.a, global_alpha)  // или src.a без глобальной альфы
invAs = 255 - As
Aout = As + mul8(Ad, invAs)
src_p = mul8(Cs, As)
dst_p = mul8(Cd, Ad)
out_p = src_p + mul8(dst_p, invAs)
Cout = (Aout == 0) ? 0 : div255(out_p, Aout)
```
`mul8` = (a*b+127)/255, `div255` = (p*255 + A/2)/A — оба с округлением к ближайшему.

