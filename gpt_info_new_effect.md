# Инструкция: Добавление новых спец эффектов в AlphaMatrixPixels

## О проекте AlphaMatrixPixels

**AlphaMatrixPixels** (сокращенно "AMPix") — это C++ библиотека для работы с LED-матрицами и создания визуальных эффектов. Библиотека предназначена для использования на Arduino и других встраиваемых системах, а также на десктопных платформах. Основные возможности: работа с матрицами пикселей в формате ARGB (альфа-канал + RGB), система рендеринга эффектов с интроспекцией свойств, поддержка fixed-point математики для работы без float на Arduino, система событий и связывания эффектов. Библиотека использует namespace `amp`, все классы имеют префикс `cs` и PascalCase именование (например, `csMatrixPixels`, `csColorRGBA`, `csRenderPlasma`). Эффекты рисуют в объект `csMatrixPixels`, который представляет 2D-матрицу пикселей с поддержкой альфа-блендинга и клиппинга.

---

## Выбор базового класса

- **`csEffectBase`** — если эффект не рисует в матрицу
- **`csRenderMatrixBase`** — если эффект рисует в матрицу пикселей
- **`csRenderDynamic`** — если эффект рисует в матрицу И нужны свойства `scale` и `speed`

---

## Система интроспекции свойств

Все эффекты используют систему интроспекции для внешнего доступа к свойствам в runtime. Это позволяет GUI, скриптам и другим системам получать список свойств, их типы, имена и указатели на данные без знания конкретного типа эффекта.

**Основные методы:**
- `getPropsCount()` — возвращает количество свойств (нумерация с 1)
- `getPropInfo(propNum, info)` — заполняет `csPropInfo` для свойства с номером `propNum`, через эту структуру можно получить указатели на данные объекта
- `propChanged(propNum)` — вызывается при изменении свойства (чтобы объект мог узнать что его данные изменили)

**Структура `csPropInfo`:**
- `valueType` — тип значения (`PropType::UInt8`, `PropType::Color`, etc.)
- `valuePtr` — указатель на поле эффекта
- `name` — имя свойства для GUI
- `readOnly` — только для чтения
- `disabled` — скрыто от GUI (по умолчанию `true`)

---

## Пошаговая инструкция


### Реализовать `render()`

```cpp
void render(csRandGen& rand, tTime currTime) const override {
    if (disabled || !matrix) {
        return;
    }
    
    const csRect target = rectDest.intersect(matrix->getRect());
    if (target.empty()) {
        return;
    }
    
    // Применить scale и speed (если используете csRenderDynamic)
    const float scaleF = scale.to_float();
    const float invScaleF = (scaleF > 0.0f) ? (1.0f / scaleF) : 1.0f;
    const float t = static_cast<float>(currTime) * 0.001f * speed.to_float();
    
    // Отрисовка
    const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
    const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
    
    for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
        for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
            // Ваша логика вычисления цвета
            matrix->setPixel(x, y, color);
        }
    }
}
```

### Реализовать `recalc()` (опционально)

```cpp
void recalc(csRandGen& rand, tTime currTime) override {
    if (disabled) {
        return;
    }
    // Обновить внутреннее состояние (частицы, анимация и т.д.)
}
```


### Настройка системы интроспекции (опционально)

Нужно только если нужно экспортировать свойства через систему интроспекции (для GUI, скриптов и т.д.). Если эффект используется только напрямую в коде, можно пропустить.

#### 1. Определить константы свойств

```cpp
class csRenderMyEffect : public csRenderDynamic {
public:
    // Поля эффекта (публичные)
    csColorRGBA color{255, 255, 255, 255};
    uint8_t intensity = 128;
    
    // Константы свойств (нумерация с 1)
    static constexpr uint8_t base = csRenderDynamic::propLast;
    static constexpr uint8_t propIntensity = base + 1;
    static constexpr uint8_t propLast = propIntensity;
    
    // ... методы ...
};
```

#### 2. Переопределить `getPropsCount()` (если добавляются новые свойства)

```cpp
uint8_t getPropsCount() const override {
    return propLast;
}
```

#### 3. Переопределить `getPropInfo()`

```cpp
void getPropInfo(uint8_t propNum, csPropInfo& info) override {
    csRenderDynamic::getPropInfo(propNum, info);
    
    switch (propNum) {
        case propIntensity:
            info.valueType = PropType::UInt8;
            info.name = "Intensity";
            info.valuePtr = &intensity;
            info.readOnly = false;
            info.disabled = false;
            break;
        case propColor:
            info.valuePtr = &color;
            info.disabled = false;
            break;
        case propScale:
        case propSpeed:
            info.disabled = false; // Включить стандартные свойства
            break;
    }
}
```

#### 4. Переопределить `propChanged()` (опционально)

Метод вызывается при изменении свойства через систему интроспекции. Используется для синхронизации полей, обновления кэша или валидации значений.

**Важно:**
- Вызывать метод базового класса только в `default` (чтобы не срабатывала логика для подавленных свойств)
- Нумерация свойств с 1
- Вызывается только при изменении через интроспекцию, не при прямом изменении полей

**Пример:**

```cpp
void propChanged(uint8_t propNum) override {
    switch (propNum) {
        case propRectDest: {
            // Синхронизировать связанные поля
            const tMatrixPixelsSize minSize = math::min(rectDest.width, rectDest.height);
            rectDest.width = minSize;
            rectDest.height = minSize;
            break;
        }
        case propIntensity:
            // Обновить кэш
            break;
        default:
            csRenderDynamic::propChanged(propNum);
            break;
    }
}
```

**Поля `csPropInfo`:**
- `valueType` — тип (`PropType::UInt8`, `PropType::Color`, `PropType::Bool`, etc.)
- `valuePtr` — указатель на поле эффекта
- `name` — имя для GUI
- `readOnly` — только для чтения
- `disabled` — скрыто от GUI (по умолчанию `true`)

**1.5. Идентификация семейства класса (опционально):**

```cpp
static constexpr PropType ClassFamilyId = PropType::EffectUserArea;

PropType getClassFamily() const override {
    return ClassFamilyId;
}

void* queryClassFamily(PropType familyId) override {
    if (familyId == ClassFamilyId) {
        return this;
    }
    return csRenderDynamic::queryClassFamily(familyId);
}
```

## Пример: простой эффект

```cpp
class csRenderBlink : public csRenderMatrixBase {
public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propBlinkSpeed = base + 1;
    static constexpr uint8_t propLast = propBlinkSpeed;
    
    csColorRGBA color{255, 255, 255, 255};
    uint8_t blinkSpeed = 2;
    
    uint8_t getPropsCount() const override {
        return propLast;
    }
    
    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderMatrixBase::getPropInfo(propNum, info);
        switch (propNum) {
            case propBlinkSpeed:
                info.valueType = PropType::UInt8;
                info.name = "Blink speed";
                info.valuePtr = &blinkSpeed;
                info.disabled = false;
                break;
            case propColor:
                info.valuePtr = &color;
                info.disabled = false;
                break;
        }
    }
    
    void render(csRandGen& /*rand*/, tTime currTime) const override {
        if (disabled || !matrix) {
            return;
        }
        
        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }
        
        const float t = static_cast<float>(currTime) * 0.001f * static_cast<float>(blinkSpeed);
        const float brightness = (sinf(t) + 1.0f) * 0.5f;
        const uint8_t alpha = static_cast<uint8_t>(brightness * 255.0f);
        const csColorRGBA blinkColor{alpha, color.r, color.g, color.b};
        
        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                matrix->setPixel(x, y, blinkColor);
            }
        }
    }
};
```


## Важные замечания

1. **Нумерация свойств с 1** — `getPropsCount() == 5` означает свойства 1, 2, 3, 4, 5
2. **Проверки в render()** — всегда проверять `disabled` и `matrix` в начале
3. **Клиппинг** — использовать `rectDest.intersect(matrix->getRect())`
4. **Поля публичные** — по дизайну для производительности
5. **render() const** — метод `const`, но может изменять `matrix` (это нормально)
6. **Область рендеринга** — `renderRectAutosize = true` (по умолчанию) автоматически устанавливает `rectDest` равным размеру матрицы



## Чеклист

- [ ] Выбран базовый класс
- [ ] Определены константы свойств (`base`, `propLast`)
- [ ] Переопределён `getPropsCount()`
- [ ] Переопределён `getPropInfo()` для всех свойств
- [ ] Реализован `render()` с проверками `disabled` и `matrix`
- [ ] Реализован `recalc()` (если нужно)
- [ ] Используется `rectDest.intersect(matrix->getRect())` для клиппинга
- [ ] Код добавлен в `src/matrix_render_efffects.hpp`
