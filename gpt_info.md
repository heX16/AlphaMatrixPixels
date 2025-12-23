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

### Демо GUI (SDL2)
Сборка и зависимости описаны в `GUI_Test/BUILD.md`.

### Arduino (FastLED)
Скетч `AlphaMatrixPixels.ino` демонстрирует работу с LED-матрицей через FastLED.

## Arduino / Embedded toolchains (важно)

Некоторые Arduino toolchain'ы (особенно старые версии GCC) могут не поддерживать часть современного C++ на уровне библиотек,
что проявляется ошибками вида:
- `enclosing class of constexpr ... is not a literal type` (если `constexpr` объявлен у метода класса с `new[]`, `union`, и т.п.)

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
- `csEventBase` (class) — базовый тип события для обмена с внешними системами (WIP).
- `ParamType` (enum class) — тип параметра для внешней интроспекции (WIP).
- `csEffectBase` (class) — базовый интерфейс рендера/эффекта: интроспекция параметров + `paramChanged/recalc/render` + события (WIP).
- `csRenderMatrixBase` (class) — базовый класс для эффектов, которые рисуют в `csMatrixPixels` (см. ниже).
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




## Класс `csMatrixPixels`

**Исходник:** `matrix_pixels.hpp`

2D-матрица пикселей ARGB (0xAARRGGBB) со straight-alpha и композицией Porter-Duff SourceOver.

### Ключевые особенности

- **Безопасные границы:** запись вне границ игнорируется, чтение возвращает `{0,0,0,0}`.
- **Глубокое копирование:** copy/move конструкторы и операторы работают корректно.
- **Метод `getRect()`:** возвращает `csRect{0,0,width,height}` — используется для клиппинга в рендерах.
- **Метод `drawMatrix()`:** композиция матриц с автоклиппингом.


---

## Система рендеринга

**Исходник:** `matrix_render.hpp`

### Ключевые принципы

1. **Публичные поля** — все данные в `csRender*` классах `public`, без get/set. Это сознательное решение для производительности и простоты.

2. **Индексация параметров с 1** — `getParamsCount()==3` означает параметры 1, 2, 3 (не 0, 1, 2). Это важно для внешней интроспекции.

### Иерархия классов

- `csEffectBase` — абстрактный интерфейс (параметры, render, события)
- `csRenderMatrixBase` — база для эффектов, рисующих в `csMatrixPixels`
- Конкретные эффекты: `csRenderGradient`, `csRenderGradientFP`, `csRenderPlasma`

### Область рендера (`rect`)

Каждый рендер, наследующий `csRenderMatrixBase`, имеет поле `rect` типа `csRect`. Эффект рисует **только внутри этого прямоугольника**.

**Режим autosize (по умолчанию):**
- `renderRectAutosize = true`
- `rect` автоматически равен размеру матрицы
- При вызове `setMatrix(...)` прямоугольник обновляется

**Ручной режим:**
- `renderRectAutosize = false`
- `rect` задаётся вручную: `plasma.rect = csRect{x, y, w, h};`
- Это позволяет рисовать эффект в произвольной области матрицы (смещение, уменьшенный размер)

```cpp
plasma.renderRectAutosize = false;
plasma.rect = csRect{2, 2, 4, 4};  // рисовать только в квадрате 4×4 со смещением (2,2)
```

### Минимальный пример

```cpp
amp::csMatrixPixels canvas(8, 8);
amp::csRenderPlasma plasma;
amp::csRandGen rng;

plasma.setMatrix(canvas);  // важно: устанавливает matrix И обновляет rect

plasma.render(rng, (uint16_t)millis());
```

### Где смотреть детали

| Что | Где |
|-----|-----|
| `ParamType` enum | `matrix_render.hpp`, начало файла |
| Методы `csEffectBase` | `matrix_render.hpp`, класс `csEffectBase` |
| Поля/параметры `csRenderMatrixBase` | `matrix_render.hpp`, класс `csRenderMatrixBase` |
| Реализации эффектов | `matrix_render.hpp`, классы `csRenderGradient`, `csRenderPlasma` и др. |
