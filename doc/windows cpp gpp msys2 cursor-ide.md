# Windows: C++ (g++) через MSYS2 + настройка Cursor (UCRT64)

Эта инструкция описывает **рабочий путь**, который приводит к состоянию:

- В Windows установлен **MSYS2** (вместе с `pacman`)
- Установлен компилятор **`g++`** (MSYS2 UCRT64 toolchain)
- В Cursor терминал открывается в **UCRT64** среде
- Компиляция `.cpp` из терминала и через build task в Cursor работает

---

## Установка MSYS2 (вместе с pacman) через winget

Откройте PowerShell/Terminal и выполните:

```powershell
winget install --id MSYS2.MSYS2 -e --source winget --accept-source-agreements --accept-package-agreements --disable-interactivity
```

Проверка, что `pacman` на месте (обычно MSYS2 ставится в `C:\msys64`):

```powershell
Test-Path C:\msys64\usr\bin\pacman.exe
```

---

## Обновление MSYS2 и установка C++ toolchain (g++)

Откройте **MSYS2 UCRT64** (важно именно UCRT64) и выполните:

```bash
pacman -Syu
```

Если терминал попросит закрыть процессы MSYS2 — закройте MSYS2 окна, откройте снова UCRT64 и выполните:

```bash
pacman -Su
```

Поставьте toolchain:

```bash
pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain
```

Проверка:

```bash
g++ --version
```

---

## Установка Git (внутри MSYS2 UCRT64)

Откройте **MSYS2 UCRT64** и установите Git:

```bash
pacman -Syu
pacman -S --needed git
```

Проверка:

```bash
git --version
which git
```

Ожидаемо, `which git` покажет путь из UCRT64, например:

```text
/usr/bin/git
```

---

## Важно: вы должны быть в UCRT64, а не MSYS

Если `g++` не находится, сначала проверьте среду:

```bash
echo $MSYSTEM
```

Нужно увидеть:

```text
UCRT64
```

Если там `MSYS`, откройте именно:

- `C:\msys64\ucrt64.exe` (откроет отдельное окно терминала)

**Примечание:** Для интеграции с Cursor IDE используйте настройки терминала, описанные ниже.

---

## Быстрый тест компиляции (hello world)

Откройте папку проекта в Cursor (**File → Open Folder…**). Дальше команды выполняйте в терминале Cursor **внутри текущего каталога проекта**.

Создайте тестовую папку и перейдите в неё:

```bash
mkdir -p cpp_test
cd cpp_test
```

`hello.cpp`:

```cpp
#include <iostream>

int main() {
    std::cout << "Hello from MSYS2 g++\n";
    return 0;
}
```

Компиляция и запуск:

```bash
g++ -v -std=c++17 hello.cpp -o hello.exe
./hello.exe
```

`-v` печатает, какие внутренние шаги/программы запускает компилятор (удобно, чтобы видеть, что он "живой").

Ожидаемый вывод:

```text
Hello from MSYS2 g++
```

---

## Один bash-скрипт "в одну строку" для компиляции

В папке `cpp_test` создайте `compile.sh`:

```bash
#!/bin/bash
g++ -v -std=c++17 hello.cpp -o hello.exe
```

**Примечание:** Используйте команду `g++` из PATH, а не полный путь. В окружении UCRT64 компилятор доступен через PATH.

Запуск:

```bash
chmod +x compile.sh
./compile.sh
```

Если нужно видеть, что запускает скрипт, включите трассировку bash:

```bash
bash -x ./compile.sh
```

---

## Настройка Cursor, чтобы терминал был UCRT64 и сборка была удобной


### Глобально (один раз для всех папок / всех проектов)

Локальные настройки в `.vscode/settings.json` работают **только для текущей папки** (workspace). Чтобы терминал **во всех папках по умолчанию** открывался как **MSYS2 UCRT64**, нужно добавить профиль в **глобальные (User) настройки**.

#### Как сделать в Cursor / VS Code

- **Шаг 1**: открой Command Palette: `Ctrl+Shift+P` \ `F1`
- **Шаг 2**: выбери команду:
  - `Preferences: Open User Settings (JSON)` (или `Settings: Open User Settings (JSON)`)
- **Шаг 3**: добавь (или аккуратно "слей" с уже существующим) этот блок в пользовательский `settings.json`:

```json
{
  "terminal.integrated.profiles.windows": {
    "MSYS2 UCRT64": {
      "path": "C:\\msys64\\usr\\bin\\bash.exe",
      "args": ["--login", "-i"],
      "env": {
        "MSYSTEM": "UCRT64",
        "MSYS2_PATH_TYPE": "inherit",
        "CHERE_INVOKING": "1"
      }
    }
  },
  "terminal.integrated.defaultProfile.windows": "MSYS2 UCRT64",
  "terminal.integrated.automationProfile.windows": {
    "path": "C:\\msys64\\usr\\bin\\bash.exe",
    "args": ["--login", "-i"],
    "env": {
      "MSYSTEM": "UCRT64",
      "MSYS2_PATH_TYPE": "inherit",
      "CHERE_INVOKING": "1"
    }
  },
  "terminal.integrated.shellIntegration.enabled": false
}
```

**Объяснение конфигурации:**
- `C:\msys64\usr\bin\bash.exe` — общий bash для всех окружений MSYS2, интегрируется с терминалом Cursor
- `--login` — загружает профиль окружения (PATH, переменные)
- `-i` — интерактивный режим
- `MSYSTEM=UCRT64` — указывает окружение UCRT64
- `MSYS2_PATH_TYPE=inherit` — наследует PATH из Windows
- `CHERE_INVOKING=1` — запускает в текущей директории

- **Шаг 4**: перезапусти Cursor (или `Developer: Reload Window`)
- **Шаг 5**: открой **новый** терминал и проверь:

```bash
echo $MSYSTEM
g++ --version
```

Ожидаемый вывод `echo $MSYSTEM`:
```text
UCRT64
```

#### Примечания

- **Приоритет**: настройки проекта (`.vscode/settings.json`) **перекрывают** глобальные user settings. Это норм: глобально — "по умолчанию", локально — если где-то нужно иначе.
- **Если Cursor/Agent игнорирует профиль на Windows**: включи опцию **Legacy Terminal Tool** в настройках Cursor и перезапусти приложение (это уже описано ниже в разделе про Legacy Terminal Tool).
- **Про `tasks.json`**: задачи сборки/запуска обычно остаются **проектными** (в `.vscode/tasks.json`), потому что команды/пути/аргументы часто отличаются между проектами. В tasks.json используйте команды из PATH (например, `g++`), а не полные пути.

### Build Task в Cursor

В проекте должен быть файл:

- `.vscode/tasks.json`

**Важно:** В tasks.json используйте команды из PATH (например, `g++`), а не полные пути. Компилятор должен быть доступен через PATH в окружении UCRT64.

Пример минимальной задачи сборки:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build (Debug)",
            "type": "shell",
            "command": "g++",
            "args": [
                "main.cpp",
                "-std=c++17",
                "-g",
                "-Wall",
                "-o",
                "app.exe"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": ["$gcc"]
        }
    ]
}
```

Как запускать:

- **Terminal → Run Build Task** (или `Ctrl+Shift+B`)
- **Terminal → Run Task → Build (Debug)**

---

## Если появляется странный префикс `q` (типа `qls`, `qcd`, `qcommand`)

Это не про алиасы (мы проверяли `type cd; type ls; type command` — там всё нормально), а про "глюк" интеграции терминала/ввода.

Рабочие шаги:

- **Отключить shell integration** (мы уже добавили это в `.vscode/settings.json`):
  - `"terminal.integrated.shellIntegration.enabled": false`
- Перезапустить Cursor / Reload Window

### Legacy Terminal Tool (важно для Windows)

**Важно:** в Cursor на Windows есть известный баг/ограничение: **без включения Legacy Terminal Tool настройки терминального профиля могут не применяться**, и агент/терминал "не переключается" на нужный shell (даже если вы настроили `.vscode/settings.json`).

Источник (форум Cursor):

- Пост с решением: [agent mode not using default terminal on windows — сообщение #2](https://forum.cursor.com/t/agent-mode-not-using-default-terminal-on-windows/136758/2)

Цитата из поста:

> I to turn the "Legacy Terminal Tool" option ON. My Agent starts with the correct default terminal.

Обсуждение похожего бага (Cursor продолжает использовать PowerShell вместо выбранного терминала):

- [Cursor terminal always defaults to PowerShell instead of Git Bash](https://forum.cursor.com/t/cursor-terminal-always-defaults-to-powershell-instead-of-git-bash/137384)

Если после отключения shell integration проблема останется, **включите Legacy Terminal Tool** в настройках Cursor и перезапустите приложение.

- **Legacy Terminal Tool** (в настройках Cursor), затем перезапуск

---

## Минимальная памятка (самое главное)

- `echo $MSYSTEM` должен быть **UCRT64**
- `g++ --version` должен работать
- `git --version` должен работать
- Сборка:

```bash
g++ -std=c++17 hello.cpp -o hello.exe
```


## система сборки Meson

`pacman -S --needed mingw-w64-ucrt-x86_64-meson mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-python`

