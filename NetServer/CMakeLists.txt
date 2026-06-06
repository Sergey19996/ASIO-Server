cmake_minimum_required(VERSION 3.10)

# 1. Проект и языки (C нужен для sqlite3.c)
project(NetServer LANGUAGES C CXX) 

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Определяем путь к корню проекта (FrameworkBasics)
set(PARENT ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_definitions(-DGAME_SERVER)

# 3. Пути к заголовочным файлам (теперь PARENT определен)
include_directories(${PARENT})            # Для #include "NetShared/..."
include_directories(${PARENT}/NetCommon)  # Для #include "olc_net.h"
include_directories(${PARENT}/NetShared)  # Для прямых инклудов

# 4. Исходники
# Собираем ваши .cpp файлы
file(GLOB_RECURSE SERVER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE SHARED_SOURCES "${PARENT}/NetShared/*.cpp")

# Исключаем лишние папки из поиска
list(FILTER SERVER_SOURCES EXCLUDE REGEX ".*/build/.*")
list(FILTER SERVER_SOURCES EXCLUDE REGEX ".*/x64/.*")
list(FILTER SERVER_SOURCES EXCLUDE REGEX ".*/\\.vs/.*")
list(FILTER SERVER_SOURCES EXCLUDE REGEX ".*/NetClient/.*")

# 5. Локальный SQLite
# Указываем ваш sqlite3.c (он лежит в той же папке, что и этот CMakeLists.txt)
set(SQLITE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3.c)

# 6. Создание исполняемого файла
# Добавляем ваши исходники + локальный SQLite
add_executable(NetServer ${SERVER_SOURCES} ${SHARED_SOURCES} ${SQLITE_SOURCE})

# 7. Линковка
# pthread — для потоков
# dl — нужно для работы SQLite на Linux (динамическая загрузка)
target_link_libraries(NetServer pthread dl)