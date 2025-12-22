# Документация `AlphaMatrixPixels`

## О предназначении этого файла

**Этот файл служит долговременной памятью для GPT ассистента и всех его будущих экземпляров.**

### Важная информация:
- При каждом перезапуске сессии кратковременная память GPT стирается
- Этот файл сохраняет критически важную информацию о проекте между сессиями
- Здесь хранится документация, особенности архитектуры и важные детали реализации
- Файл обновляется по мере развития проекта и получения новой информации

### Инструкции для будущих экземпляров GPT:
1. **Прочитайте этот файл полностью** перед началом работы с проектом
2. **Используйте информацию** из этого файла для понимания контекста
3. **Обновляйте файл** при получении новой важной информации
4. **Добавляйте разделы** по мере необходимости для организации знаний


## Именование

**Название проекта:** AlphaMatrixPixels (сокращенно "AMPix")

**Namespace:** `amp`

- Публичные C++ символы: `namespace amp`.
- Классы/структуры: префикс `cs` + PascalCase (`csMatrixPixels`, `csColorRGBA`, ...).
- Математика и fixed-point: `namespace amp::math`, типы `csFP16/csFP32`.

## Быстрый старт

### Тесты
- Запуск из MSYS2 Bash: `./run_tests.sh`
- Скрипт компилирует и запускает `tests/pixel_matrix_tests.cpp` и печатает `Passed/Failed`.

!!!

### Демо GUI (SDL2)
Сборка и зависимости описаны в `GUI_Test/BUILD.md`.
!!!

## Arduino / Embedded toolchains (важно)

Некоторые Arduino toolchain'ы (особенно старые версии GCC) могут не поддерживать часть современного C++ на уровне библиотек,
что проявляется ошибками вида:
- `requested alignment is not an integer constant` (часто из сторонних библиотек, например FastLED, в `alignas(...)`)
- `enclosing class of constexpr ... is not a literal type` (если `constexpr` объявлен у метода класса с `new[]`, `union`, и т.п.)

### Рекомендации
- Для сборки под ESP8266/ESP32 используйте актуальные core-пакеты плат (через Boards Manager) и совместимую версию FastLED.
- Если видите ошибки `alignas(...)` внутри FastLED:
  - обновите core пакета платы (компилятор), или
  - откатите FastLED на более старую версию, совместимую с вашим компилятором.

### Примечание по AMP_CONSTEXPR
В `fixed_point.hpp` используется макрос `AMP_CONSTEXPR` (по умолчанию `constexpr`), который на Arduino по умолчанию
переключается в `inline`, чтобы избежать проблем старых компиляторов с `constexpr` и float math.

## Основные типы
- `amp::csColorRGBA` — упакованный цвет ARGB; поддерживает конструктор из packed `0xAARRGGBB` и конструктор по каналам `(a,r,g,b)`. Размер гарантирован 4 байта (packed/pragma + static_assert).
- `amp::tMatrixPixelsCoord` (`int32_t`) — координаты (допускает отрицательные смещения для клиппинга).
- `amp::tMatrixPixelsSize` (`uint16_t`) — размеры матрицы.

## API: перечень классов/структур

### `namespace amp`
- `csColorRGBA` (struct) — ARGB цвет (0xAARRGGBB) + операции SourceOver (straight alpha).
- `csMatrixPixels` (class) — матрица пикселей RGBA в памяти + клиппинг/блендинг.
- `csRect` (class) — прямоугольник (x, y, width, height) + пересечение `intersect()`.
- `csRandGen` (class) — простой генератор псевдослучайных чисел (8-bit) для эффектов.
- `csParamsBase` (class) — базовый интерфейс параметров эффекта (WIP/расширяемый).
- `csEventBase` (class) — базовый тип события для обмена с внешними системами (WIP).
- `csRenderBase` (class) — базовый интерфейс рендера/эффекта (recalc/render + события).
- `csRenderGradient` (class) — градиентный эффект на float.
- `csRenderGradientFP` (class) — градиентный эффект на fixed-point (`amp::math::csFP32`).
- `csRenderPlasma` (class) — “plasma” эффект на float.

### `namespace amp::math`
- `csFP16` (struct) — signed 8.8 fixed-point.
- `csFP32` (struct) — signed 16.16 fixed-point.

### Важно
- Классы/структуры из `GUI_Test/` и `tests/` (например `csMainProgramSDL`, `TestStats`) **не являются частью библиотеки** — это демо и тестовый код.


## Класс `csColorRGBA`

Формат цвета:
- Packed-формат во входных данных: `0xAARRGGBB` (AA — альфа, затем RR, GG, BB).
- Конструктор по каналам: `csColorRGBA(a, r, g, b)`.
- Конструктор RGB: `csColorRGBA(r, g, b)` (альфа будет `0xFF`).
- Альфа: **straight alpha** (не premultiplied).
- Композиция: Porter-Duff **SourceOver**.

- Формат ARGB: порядок байт A,R,G,B при доступе через `raw`/`value` соответствует `0xAARRGGBB`.

### Особенности `csColorRGBA`
- Конструктор по каналам: порядок аргументов **`(a, r, g, b)`**.
- Конструктор RGB: порядок аргументов **`(r, g, b)`**, альфа принудительно становится `0xFF`.
- Конструктор из `uint32_t packed`:
  - Если альфа = 0, вход трактуется как `0xRRGGBB` и альфа принудительно становится `0xFF`.
  - Иначе packed трактуется как `0xAARRGGBB`.
  - Итог: packed-конструктор с `A == 0` работает как «непрозрачный RGB», а не как прозрачный ARGB.
- Оператор `/=` делит все каналы (включая alpha) целочисленно:
  ```cpp
  // Порядок аргументов: (a, r, g, b)
  amp::csColorRGBA c{255, 200, 100, 50};
  c /= 2; // -> {a=127, r=100, g=50, b=25}
  ```
- Статические методы:
  - `blendChannel` — смешивает один канал с учётом premult и итоговой альфы.
  - `sourceOverStraight(dst, src, global_alpha)` — SourceOver со глобальной альфой-множителем.
  - `sourceOverStraight(dst, src)` — SourceOver, используя только `src.a`.


### Алгоритм смешивания (для одного канала, нормализовано к 0..255)
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




## Класс `amp::csMatrixPixels`
`csMatrixPixels` — чистая математическая 2D-матрица пикселей в формате ARGB (0xAARRGGBB) со straight-alpha и композицией Porter-Duff SourceOver. Не привязана к железу, пригодна для моделирования/рендера в памяти.

Хранит буфер `width * height` (динамический `new[]`), операции вне границ — тихо игнорируются (геттер возвращает прозрачный чёрный).
Записи вне границ (`setPixel*`) — тихо игнорируются.
Чтение вне границ (`getPixel`) — возвращает прозрачный чёрный `{0,0,0,0}`.

### Конструкторы/присваивания
- `amp::csMatrixPixels(w, h)` — инициализация нулями.
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
#include "matrix_pixels.hpp"

amp::csMatrixPixels m(4, 3);
amp::csColorRGBA red{255, 255, 0, 0};
amp::csColorRGBA semi{128, 0, 0, 255}; // полупрозрачный синий

m.setPixel(1, 1, red);
m.setPixelBlend(1, 1, semi);          // синее поверх красного, учтёт alpha пикселя
m.setPixelBlend(2, 1, semi, 64);      // дополнительно ослабит источник

amp::csColorRGBA bg{0x202020};             // 0xFF202020 из-за автоподнятия альфы
amp::csColorRGBA blended = m.getPixelBlend(1, 1, bg);
```

### Пример `drawMatrix` с клиппингом
```cpp
amp::csMatrixPixels dest(8, 8);
amp::csMatrixPixels src(4, 4);
src.setPixel(0, 0, amp::csColorRGBA{255,255,0,128});
dest.drawMatrix(-1, -1, src, 200); // частично выйдет за границы, будет обрезано
```
