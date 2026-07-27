QT       += core gui widgets

CONFIG   += c++17
TEMPLATE  = app
TARGET    = crossword

# Чтобы исходники с кириллицей корректно компилировались MSVC
msvc:QMAKE_CXXFLAGS += /utf-8

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    generator.cpp \
    cellwidget.cpp \
    words.cpp

HEADERS += \
    mainwindow.h \
    generator.h \
    cellwidget.h \
    words.h

# Копируем words.txt в каталог сборки, чтобы программа находила его рядом с собой
words_txt.files   = $$PWD/words.txt
words_txt.path    = $$OUT_PWD
COPIES           += words_txt
