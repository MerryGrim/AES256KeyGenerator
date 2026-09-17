# AES-256 Key Generator

Native, standalone AES-256 key generator for Windows XP SP3 and newer.

[Русский](#русский) · [English](#english) · [中文](#中文)

---

## Русский

### О программе

AES-256 Key Generator — компактный автономный генератор криптографически стойких 256-битных ключей. Программа написана на чистом Win32 C, не требует установки, .NET или внешних библиотек.

### Возможности

- Windows XP SP3 и новее, 32-битный автономный EXE.
- Генерация от 1 до 100 ключей; значение по умолчанию — 16.
- Ручной ввод количества и кнопки `−` / `+`; значения ограничиваются диапазоном 1–100.
- Русский, английский и китайский интерфейс.
- Светлая и тёмная темы.
- Выделение нескольких ключей через Ctrl и диапазона через Shift.
- Копирование выбранных ключей или всего списка.
- Экспорт ключей в текстовый файл UTF-8.
- Системный генератор случайных чисел Windows CryptoAPI (`CryptGenRandom`).

Каждый ключ содержит 64 шестнадцатеричных символа: 32 байта, или 256 бит. Регистр букв `a–f` значения не меняет.

### Использование

1. Укажите количество ключей.
2. Нажмите «Сгенерировать».
3. Выберите нужные строки и скопируйте их либо сохраните весь список.

Храните созданные ключи в безопасном месте. Буфер обмена и обычный текстовый файл не являются защищённым хранилищем.

---

## English

### About

AES-256 Key Generator is a compact, standalone generator of cryptographically secure 256-bit keys. It is written in plain Win32 C and requires no installation, .NET runtime, or external libraries.

### Features

- Windows XP SP3 and newer, standalone 32-bit executable.
- Generates 1–100 keys; the default is 16.
- Manual count entry and `−` / `+` buttons; values are clamped to 1–100.
- Russian, English, and Chinese interface.
- Light and dark themes.
- Ctrl multi-selection and Shift range selection.
- Copy selected keys or the complete list.
- Export keys to a UTF-8 text file.
- Uses Windows CryptoAPI (`CryptGenRandom`) for secure random bytes.

Each key contains 64 hexadecimal characters: 32 bytes, or 256 bits. Hexadecimal letter case does not change the key value.

### Usage

1. Enter the required key count.
2. Press Generate.
3. Select and copy the required rows, or save the complete list.

Keep generated keys in a secure location. The clipboard and an ordinary text file are not secure storage.

---

## 中文

### 关于

AES-256 Key Generator 是一款小巧、独立运行的加密安全 256 位密钥生成器。程序使用原生 Win32 C 编写，无需安装、.NET 运行库或外部依赖。

### 功能

- 支持 Windows XP SP3 及更高版本，独立的 32 位可执行文件。
- 可生成 1–100 个密钥，默认数量为 16。
- 支持手动输入数量以及 `−` / `+` 按钮，数值自动限制在 1–100。
- 提供俄语、英语和中文界面。
- 支持浅色和深色主题。
- 使用 Ctrl 选择多个密钥，使用 Shift 选择连续范围。
- 复制所选密钥或复制全部密钥。
- 将密钥导出为 UTF-8 文本文件。
- 使用 Windows CryptoAPI (`CryptGenRandom`) 生成安全随机字节。

每个密钥包含 64 个十六进制字符，即 32 字节或 256 位。十六进制字母的大小写不会改变密钥值。

### 使用方法

1. 输入需要生成的密钥数量。
2. 点击“生成”。
3. 选择并复制需要的密钥，或保存完整列表。

请将生成的密钥保存在安全位置。剪贴板和普通文本文件并不是安全存储方式。

---

## Build

The release executable is built for x86 without a modern runtime dependency. From an x86 Visual Studio Developer Command Prompt:

```bat
rc /fo app.res app.rc
cl /nologo /c /utf-8 /TC /O1 /Oi- /GS- /Zl /D_WIN32_WINNT=0x0501 aes_key_generator.c
link /nologo /OUT:AES-256-Key-Generator.exe /SUBSYSTEM:WINDOWS,5.01 /MACHINE:X86 /ENTRY:WinMainEntry /NODEFAULTLIB aes_key_generator.obj app.res kernel32.lib user32.lib gdi32.lib
```

## Website

[dmr-zone.ru](http://dmr-zone.ru/)

## License

MIT License. See [LICENSE](LICENSE).
