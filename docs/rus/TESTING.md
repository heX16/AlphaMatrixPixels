# Testing

- Тесты запускаются из Bash-консоли (MSYS2). **Дополнительно вызывать `bash` не нужно**.
- **Директорию менять не нужно** (команду `cd` не вызываем), просто запускаем скрипт:
  - `./tests/build_and_run_tests.sh`

⚠️ **НАПОМНИ СЕБЕ**: если терминал уже в корне репо (`/d/Prj/AlphaMatrixPixels`), **НЕ ПИШИ `cd`**. Просто сразу `./tests/build_and_run_tests.sh`. Если сомневаешься — сначала проверь `pwd`, но не добавляй лишний `cd`.
⚠️ **Для GPT ассистента**: если cwd уже /h/Prj/AlphaMatrixPixels, не добавляй cd, сразу `./tests/build_and_run_tests.sh`.

Типичный вывод (успешный прогон):

```
[run_tests] start
... (сообщения компиляции gcc/g++)
Passed: 38, Failed: 0
[run_tests] done
```

## Notes

- Если вы по какой-то причине НЕ в Bash (например, PowerShell/CMD), то откройте MSYS2 Bash и уже там выполните `./tests/build_and_run_tests.sh`.

