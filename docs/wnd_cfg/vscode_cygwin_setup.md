# Настройка Cygwin в Visual Studio Code на Windows

Инструкция по настройке Cygwin в Visual Studio Code (VS Code) на Windows включает настройку интегрированного терминала, добавление инструментов Cygwin в системный PATH и настройку путей компиляторов для разработки.

## 1. Интеграция терминала Cygwin в VS Code

Можно добавить Cygwin bash как профиль терминала для удобного доступа в VS Code.

1. Откройте VS Code и перейдите в **Settings** (`Ctrl + ,`).
2. Найдите `terminal.integrated.profiles.windows` и нажмите **Edit in settings.json**.
3. Добавьте следующий профиль в конфигурацию:

```json
{
  "terminal.integrated.profiles.windows": {
    "Cygwin": {
      "path": "C:\\cygwin64\\bin\\bash.exe",
      "args": ["--login"]
    }
  }
}
```

4. Сохраните файл. Теперь можно выбрать **Cygwin** из выпадающего меню терминала.

Альтернативный способ:
- Нажмите `F1`.
- Введите: `Terminal: Create New Terminal (with Profile)`
- Выберите **Cygwin**.

## 2. Настройка Cygwin для разработки на C/C++

Для использования компиляторов Cygwin (например, `gcc`, `g++`) и инструментов `make` они должны быть в системном PATH Windows.

### Обновление системного PATH
- Добавьте `C:\cygwin64\bin` в переменные окружения Windows (Environment Variables).

### Установка необходимых пакетов
- Убедитесь, что установлены пакеты `gcc-g++`, `gdb` и `make` с помощью утилиты Cygwin Setup.

### Расширение C++ для VS Code
- Установите [C/C++ extension by Microsoft](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools). Это позволит VS Code обнаруживать компиляторы Cygwin для IntelliSense и отладки.

## 3. Настройка сборки и отладки

### Задача сборки
- Создайте файл `.vscode/tasks.json` для автоматизации компиляции с использованием `g++` из Cygwin. Используйте меню **Terminal > Run Build Task** для запуска.

### Отладчик
- Настройте `.vscode/launch.json` для использования `gdb` из Cygwin.
- **Примечание:** Если возникают проблемы с новыми версиями GDB, некоторые разработчики рекомендуют понизить версию до стабильной (например, 9.2-1) для лучшей совместимости с отладчиком VS Code.

### Поддержка Makefile
- При использовании Makefile убедитесь, что расширение Makefile Tools настроено на путь к `make.exe` из Cygwin.

## 4. Опционально: Интеграция Git из Cygwin

Если вы предпочитаете Git из Cygwin вместо Git for Windows:

1. В настройках VS Code найдите `git.path`.
2. Установите значение `C:\\cygwin64\\bin\\git.exe`.
3. Имейте в виду, что маппинг путей (например, `/cygdrive/c/`) может иногда вызывать проблемы со встроенным Git UI в VS Code.

## Источники

- [VS Code - Cygwin as Integrated Terminal - Stack Overflow](https://stackoverflow.com/questions/46031646/vscode-cygwin-as-integrated-terminal)
- [Terminal Profiles - Visual Studio Code](https://code.visualstudio.com/docs/terminal/profiles)
- [Cygwin GDB and VSCode - Stack Overflow](https://stackoverflow.com/questions/66332819/cygwin-gdb-and-vscode)
