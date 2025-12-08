#!/usr/bin/env python3
"""
Скрипт для автоматической настройки проекта микроконтроллера в VS Code
после переноса из Eclipse
"""

import os
import json
import argparse
import sys
from pathlib import Path

def create_vscode_folder(project_path):
    """Создает папку .vscode если её нет"""
    vscode_dir = project_path / ".vscode"
    vscode_dir.mkdir(exist_ok=True)
    return vscode_dir

def detect_mcu_type(project_path):
    """Пытается определить тип микроконтроллера по файлам проекта"""
    mcu_types = {
        "STM32": ["stm32", "stm", "cortex-m"],
        "AVR": ["avr", "atmega", "attiny"],
        "ESP32": ["esp32", "esp-idf"],
        "ESP8266": ["esp8266"],
        "ARM": ["arm", "cortex"]
    }
    
    # Ищем в названиях файлов и папок
    for item in project_path.rglob("*"):
        if item.is_file():
            try:
                content = item.read_text(encoding='utf-8', errors='ignore')
                for mcu_type, keywords in mcu_types.items():
                    for keyword in keywords:
                        if keyword in content.lower():
                            return mcu_type
            except:
                continue
    return "ARM"  # По умолчанию

def create_tasks_json(vscode_dir, project_type):
    """Создает tasks.json для сборки проекта"""
    if project_type == "waf":
        tasks = {
            "version": "2.0.0",
            "tasks": [
                {
                    "label": "WAF Build",
                    "type": "shell",
                    "command": "./waf",
                    "args": ["build"],
                    "group": {
                        "kind": "build",
                        "isDefault": True  # Исправлено: true -> True
                    },
                    "problemMatcher": ["$gcc"],
                    "detail": "Сборка проекта с помощью WAF"
                },
                {
                    "label": "WAF Clean",
                    "type": "shell",
                    "command": "./waf",
                    "args": ["clean"],
                    "group": "build",
                    "detail": "Очистка проекта"
                },
                {
                    "label": "WAF Configure",
                    "type": "shell",
                    "command": "./waf",
                    "args": ["configure"],
                    "group": "build",
                    "detail": "Конфигурация WAF"
                }
            ]
        }
    else:
        tasks = {
            "version": "2.0.0",
            "tasks": [
                {
                    "label": "Build Project",
                    "type": "shell",
                    "command": "make",
                    "args": ["all"],
                    "group": {
                        "kind": "build",
                        "isDefault": True  # Исправлено: true -> True
                    },
                    "problemMatcher": ["$gcc"],
                    "detail": "Сборка проекта"
                },
                {
                    "label": "Clean Project",
                    "type": "shell",
                    "command": "make",
                    "args": ["clean"],
                    "group": "build",
                    "detail": "Очистка проекта"
                }
            ]
        }
    
    tasks_file = vscode_dir / "tasks.json"
    with open(tasks_file, 'w', encoding='utf-8') as f:
        json.dump(tasks, f, indent=4, ensure_ascii=False)
    print(f"✓ Создан {tasks_file}")

def create_launch_json(vscode_dir, mcu_type):
    """Создает launch.json для отладки"""
    
    # Базовые конфигурации для разных типов МК
    configurations = []
    
    if mcu_type in ["STM32", "ARM"]:
        configurations = [
            {
                "name": "Cortex Debug",
                "type": "cortex-debug",
                "request": "launch",
                "servertype": "openocd",
                "cwd": "${workspaceRoot}",
                "executable": "${workspaceFolder}/build/firmware.elf",
                "device": "STM32F103C8",  # Нужно будет изменить
                "configFiles": [
                    "interface/stlink-v2.cfg",
                    "target/stm32f1x.cfg"
                ],
                "svdFile": "${workspaceFolder}/STM32F103xx.svd",
                "runToEntryPoint": "main",
                "showDevDebugOutput": "raw"
            }
        ]
    elif mcu_type == "AVR":
        configurations = [
            {
                "name": "AVR Debug",
                "type": "avr-gdb",
                "request": "launch",
                "program": "${workspaceFolder}/build/firmware.elf",
                "device": "atmega328p"
            }
        ]
    else:
        configurations = [
            {
                "name": "Generic Debug",
                "type": "cppdbg",
                "request": "launch",
                "program": "${workspaceFolder}/build/firmware.elf",
                "args": [],
                "stopAtEntry": True,
                "cwd": "${workspaceFolder}",
                "environment": [],
                "externalConsole": False,
                "MIMode": "gdb"
            }
        ]
    
    launch_config = {
        "version": "0.2.0",
        "configurations": configurations
    }
    
    launch_file = vscode_dir / "launch.json"
    with open(launch_file, 'w', encoding='utf-8') as f:
        json.dump(launch_config, f, indent=4, ensure_ascii=False)
    print(f"✓ Создан {launch_file}")

def create_c_cpp_properties(vscode_dir, project_path):
    """Создает c_cpp_properties.json для IntelliSense"""
    
    # Автоматически находим include-папки
    include_paths = [
        "${workspaceFolder}/**",
        "${workspaceFolder}/inc",
        "${workspaceFolder}/include",
        "${workspaceFolder}/src"
    ]
    
    # Добавляем найденные папки с заголовками
    for folder in ["inc", "include", "src", "lib"]:
        if (project_path / folder).exists():
            include_paths.append(f"${{workspaceFolder}}/{folder}")
    
    properties = {
        "configurations": [
            {
                "name": "Microcontroller",
                "includePath": include_paths,
                "defines": [
                    "DEBUG",
                    "USE_HAL_DRIVER"
                ],
                "compilerPath": "/usr/bin/arm-none-eabi-gcc",
                "cStandard": "c11",
                "cppStandard": "c++17",
                "intelliSenseMode": "gcc-arm"
            }
        ],
        "version": 4
    }
    
    properties_file = vscode_dir / "c_cpp_properties.json"
    with open(properties_file, 'w', encoding='utf-8') as f:
        json.dump(properties, f, indent=4, ensure_ascii=False)
    print(f"✓ Создан {properties_file}")

def create_settings_json(vscode_dir):
    """Создает settings.json с настройками VS Code"""
    settings = {
        "files.associations": {
            "*.h": "c",
            "*.c": "c",
            "*.cpp": "cpp"
        },
        "C_Cpp.errorSquiggles": "disabled",
        "editor.formatOnSave": True,
        "files.exclude": {
            "**/build": True,
            "**/.vscode": False
        }
    }
    
    settings_file = vscode_dir / "settings.json"
    with open(settings_file, 'w', encoding='utf-8') as f:
        json.dump(settings, f, indent=4, ensure_ascii=False)
    print(f"✓ Создан {settings_file}")

def check_waf_files(project_path):
    """Проверяет наличие WAF файлов"""
    waf_files = ["waf", "waf.bat", "wscript"]
    found = any((project_path / file).exists() for file in waf_files)
    return "waf" if found else "make"

def create_readme(project_path, project_type, mcu_type):
    """Создает README с инструкциями"""
    
    build_command = "waf build" if project_type == "waf" else "make all"
    clean_command = "waf clean" if project_type == "waf" else "make clean"
    
    readme_content = """# Настройка проекта микроконтроллера в VS Code

## Автоматически сгенерированные файлы конфигурации

### Тип проекта: {project_type}
### Микроконтроллер: {mcu_type}

## Следующие шаги:

1. **Установите расширения VS Code:**
   - C/C++ (Microsoft)
   - Cortex-Debug (для ARM)
   - Hex Editor

2. **Настройте пути к компилятору:**
   - Отредактируйте `.vscode/c_cpp_properties.json`
   - Укажите правильный `compilerPath`

3. **Настройте отладку:**
   - Отредактируйте `.vscode/launch.json`
   - Укажите правильную модель МК и конфигурацию OpenOCD

4. **Проверьте сборку:**
   - Ctrl+Shift+P -> "Tasks: Run Build Task"
   - Или терминал: `{build_command}`

## Полезные команды:

### Сборка:
```bash
{build_command}