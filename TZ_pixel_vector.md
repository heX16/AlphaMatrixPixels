## Тип цвета

`tColorRGBA` = `uint32`.

- Формат: `0xRRGGBBAA` (`R` в старшем байте, `A` в младшем)
- Диапазон каналов: 0..255
- Семантика alpha: `A`=0 полностью прозрачно, `A`=255 полностью непрозрачно

## Уточнения (зафиксировано)

- Смешивание каналов выполняется линейно (без gamma/sRGB коррекции)
- Координаты вне границ: операции игнорируются (ошибкой не считаются)
- `getPixelBlend(x,y,bgColor)`: при смешивании учитывается `A` у `bgColor`
- `drawMatrix(x,y,source)`: выполняет клиппинг (тихо), выход за границы не является ошибкой
- Дополнительные методы (clear/fill/и т.п.) не требуются

## Alpha compositing (зафиксировано для пункта 3)

Правило композиции: Porter-Duff **SourceOver**, **straight alpha**.

Ожидаемое поведение: **альфа обычно растёт, если рисуем сверху** (то есть \(A_{out} \ge A_d\) при \(A_s > 0\)).

Обозначения (все `uint8` 0..255):

- `dst` = текущий пиксель матрицы (`Rd,Gd,Bd,Ad`)
- `src` = `color` (`Rs,Gs,Bs,As0`)
- `alpha` = параметр функции `setPixel(..., alpha)`

Эффективная альфа источника:

```text
As = mul8(As0, alpha)
```

Выходная альфа:

```text
Aout = As + mul8(Ad, 255 - As)
```

Точная формула для RGB (через premultiplied представление и обратное деление):

```text
src_p = mul8(Cs, As)                 // Cs = Rs/Gs/Bs
dst_p = mul8(Cd, Ad)                 // Cd = Rd/Gd/Bd
out_p = src_p + mul8(dst_p, 255 - As)
Cout  = (Aout == 0) ? 0 : div255(out_p, Aout)
```

Где:

```text
mul8(a,b)   = (a*b + 127) / 255      // округление к ближайшему
div255(p,A) = (p*255 + A/2) / A      // округление к ближайшему, A>0
```

Требование:

- `setPixelBlend(x,y,color,alpha)` должен рисовать используя формулы выше.
- `drawMatrix(x,y,source)` должен выполнять клиппинг и реализовываться через вызовы `setPixelBlend()`
  для каждого пикселя `source` (источник рисуется поверх destination).

## API

```text
class csMatrixPixels {
  csMatrixPixels(uint16 size_x, uint16 size_y)

  setPixel(x,y, tColorRGBA color)
  setPixelBlend(x,y, tColorRGBA color, uint8 alpha)

  tColorRGBA getPixel(x,y)
  tColorRGBA getPixelBlend(x,y, bgColor)

  drawMatrix(x,y, &csMatrixPixels source)
}
```


---------------------

