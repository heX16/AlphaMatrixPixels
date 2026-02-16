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
- Классы должны оставаться наследуемыми (не использовать модификатор `final`, запрещающий наследование).

## Быстрый старт

### Тесты
- Запуск из MSYS2 Bash: `./tests/build_and_run_tests.sh`
- Скрипт компилирует и запускает `tests/pixel_matrix_tests.cpp` и печатает `Passed/Failed`.

### Arduino (FastLED)
Скетч `examples/AlphaMatrixPixelsBase/AlphaMatrixPixelsBase.ino` демонстрирует работу с LED-матрицей через FastLED.

### Примечание по AMP_CONSTEXPR
В `src/fixed_point.hpp` используется макрос `AMP_CONSTEXPR` (по умолчанию `constexpr`), который на Arduino по умолчанию
переключается в `inline`, чтобы избежать проблем старых компиляторов с `constexpr` и float math.

## Основные типы
- `amp::csColorRGBA` — упакованный цвет ARGB; поддерживает конструктор из packed `0xAARRGGBB` и конструктор по каналам `(a,r,g,b)`. Размер гарантирован 4 байта (packed/pragma + static_assert).
- `amp::tMatrixPixelsCoord` (`int32_t`) — координаты (допускает отрицательные смещения для клиппинга).
- `amp::tMatrixPixelsSize` (`uint16_t`) — размеры матрицы.

## API: перечень классов/структур

### `namespace amp`
- `csColorRGBA` (struct) — ARGB цвет (0xAARRGGBB) + операции SourceOver (straight alpha).
- `csMatrixBase` (abstract class) — общий интерфейс матрицы в терминах `csColorRGBA` (`getPixel/setPixelRewrite/setPixel`).
- `csMatrixPixels` (class) — RGBA-матрица; наследуется от `csMatrixBase`.
- `csMatrixBytes` (class) — матрица `uint8_t`; наследуется от `csMatrixBase` + предоставляет `getValue/setValue` для сырого доступа.
- `csMatrixBoolean` (class) — матрица битов (bool); наследуется от `csMatrixBase` + предоставляет `getValue/setValue` для сырого доступа.
- `csRect` (class) — прямоугольник (x, y, width, height) + пересечение `intersect()`.
- `csRandGen` (class) — простой генератор псевдослучайных чисел (8-bit) для эффектов.
- `csEventBase` (class) — базовый тип события для обмена с внешними системами (WIP).
- `PropType` (enum class) — тип свойства для внешней интроспекции (WIP).
- `csEffectBase` (class) — базовый интерфейс рендера/эффекта: интроспекция свойств + `propChanged/recalc/render` + события (WIP).
- `csRenderMatrixBase` (class) — базовый класс для эффектов, которые рисуют в матрицу через интерфейс `csMatrixBase` (см. ниже).

### `namespace amp::math`
- `csFP16` (struct) — signed 8.8 fixed-point.
- `csFP32` (struct) — signed 16.16 fixed-point.

### Важно
- Классы/структуры из `dev/GUI_Test/` и `tests/` (например `csMainProgramSDL`, `TestStats`) **не являются частью библиотеки** — это демо и тестовый код.


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
- Конструктор из `uint32_t packed`: имеет необычные оссобености работы с альфа каналом, читай код перед использованием.

### Алгоритм смешивания (для одного канала, нормализовано к `0..1`)
```
// Porter-Duff SourceOver: источник рисуется поверх назначения
A_src = src.a * global_alpha  // эффективная альфа источника
Aout = A_src + A_dst * (1 - A_src)  // итоговая альфа
Cout = (C_src * A_src + C_dst * A_dst * (1 - A_src)) / Aout
```
Примечание: в реальной реализации используется целочисленная uint8 арифметика нормализованная к `0..255`.




## Класс `csMatrixPixels`

**Исходник:** `src/matrix_pixels.hpp`

2D-матрица пикселей ARGB (0xAARRGGBB). Реализует `csMatrixBase` напрямую (без конвертации).

### Ключевые особенности

- **Безопасные границы:** запись вне границ игнорируется, чтение возвращает `{0,0,0,0}`.
- **Метод `getRect()`:** возвращает `csRect{0,0,width,height}` — используется для клиппинга в рендерах.
- **Метод `drawMatrix()`:** композиция матриц с автоклиппингом.

## Матрицы: общий интерфейс (`csMatrixBase`)

Все матрицы (`csMatrixPixels`, `csMatrixBytes`, `csMatrixBoolean`) наследуются от `csMatrixBase`, чтобы эффекты/алгоритмы могли работать с любым типом матрицы через единый набор виртуальных методов:
- `getPixel(x, y) -> csColorRGBA`
- `setPixelRewrite(x, y, csColorRGBA)`
- `setPixel(x, y, csColorRGBA)` и/или `setPixel(x, y, csColorRGBA, alpha)`

### Сырой доступ к данным (`getValue/setValue`)

`csMatrixBytes` и `csMatrixBoolean` дополнительно имеют невиртуальные методы `getValue/setValue` (работают с нативным типом `uint8_t`/`bool`). Их следует использовать в коде, где нужен именно “значение-пиксель”, без RGBA-конвертации.

## Правила конвертации (Bytes/Boolean ↔ RGBA)

Эти правила определяют, как `csMatrixBytes` и `csMatrixBoolean` реализуют виртуальный интерфейс `csMatrixBase` в терминах `csColorRGBA`.

### Bytes → RGBA (`csMatrixBytes::getPixel`)
- Пусть `v = getValue(x, y)` (0..255).
- Возвращаем `csColorRGBA{255, v, v, v}` (серый, полностью непрозрачный).

### RGBA → Bytes (`csMatrixBytes::setPixelRewrite`)
- Вычисляем интенсивность `i` из входного цвета (по умолчанию берём `max(r, g, b)`).
- `setValue(x, y, i)`. Альфа игнорируется (это overwrite).

### RGBA → Bytes (`csMatrixBytes::setPixel`)
- Эффективная альфа: `a = mul8(color.a, alpha)` (если есть параметр `alpha`, иначе `alpha = 255`).
- Интенсивность источника `src = max(r, g, b)`.
- Значение назначения `dst = getValue(x, y)`.
- Новое значение: `out = dst + ((src - dst) * a) / 255`.
- `setValue(x, y, out)`.

### Boolean → RGBA (`csMatrixBoolean::getPixel`)
- Пусть `b = getValue(x, y)`.
- `false` → `csColorRGBA{0, 0, 0, 0}` (прозрачный чёрный).
- `true` → `csColorRGBA{255, 255, 255, 255}` (белый непрозрачный).

### RGBA → Boolean (`csMatrixBoolean::setPixelRewrite`)
- Вычисляем интенсивность `i = max(r, g, b)`.
- `setValue(x, y, i != 0)`. Альфа игнорируется (это overwrite).

### RGBA → Boolean (`csMatrixBoolean::setPixel`)
- Эффективная альфа: `a = mul8(color.a, alpha)` (если есть параметр `alpha`, иначе `alpha = 255`).
- Интенсивность `i = max(r, g, b)`.
- Правило: если `a != 0` и `i != 0`, то `setValue(x, y, true)`, иначе `setValue(x, y, false)`.

## Система рендеринга

**Исходник:** `src/matrix_render.hpp`

### Ключевые принципы

**Публичные поля** — все данные в `csRender*` классах `public`, без get/set. Это сознательное решение для производительности и простоты.

### Интроспекции - система свойств

Система динамического доступа к свойствам эффектов в runtime. Позволяет внешним системам (GUI, скрипты, сериализация) получать список свойств, их типы, имена и указатели на данные без знания конкретного типа эффекта.

API:
- **`getPropsCount()`** — возвращает количество доступных свойств.
- **`getPropInfo(propNum, info)`** — заполняет `csPropInfo` для свойства с номером `propNum` (нумерация с 1).
- **`propChanged(propNum)`** — вызывается при изменении свойства (для обновления кэша, пересчёта зависимостей). Нумерация с 1!

### Идентификация семейств классов (Class Family System)

Система безопасного приведения типов без RTTI (замена `dynamic_cast` для Arduino с `-fno-rtti`).

API:
- **`getClassFamily()`** — возвращает `PropType` идентификатор семейства класса.
- **`queryClassFamily(PropType familyId)`** — возвращает указатель на объект, если он принадлежит запрошенному семейству, иначе `nullptr`.

Доступные семейства: `EffectBase`, `EffectMatrixDest`, `EffectGlyph`, `EffectPipe`, `EffectDigitalClock`.

Пример:
```cpp
if (auto* clock = static_cast<amp::csRenderDigitalClock*>(
    eff->queryClassFamily(amp::PropType::EffectDigitalClock)
)) {
    clock->time = 1234;
}
```

### Область рендера (`rect`)

Каждый рендер, наследующий `csRenderMatrixBase`, имеет поле `rect` типа `csRect`. Эффект рисует **только внутри этого прямоугольника**.

**Режим autosize (по умолчанию):**
- `renderRectAutosize = true`
- `rect` автоматически равен размеру матрицы (зависит от реализации конкретного класса)

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

