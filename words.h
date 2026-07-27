#ifndef WORDS_H
#define WORDS_H

#include <QVector>
#include <QString>

struct WordEntry {
    QString word;
    QString clue;
};

// Загружает словарь из текстового файла.
//
// Формат файла (UTF-8):
//   * по одному слову на строку, разделитель — символ '|';
//   * пустые строки и строки, начинающиеся с '#', игнорируются.
//
// Пример: КОТ|Домашнее животное, которое мурлычет
//
// При ошибке (файл не открыт, плохая строка и т.п.) пишет описание
// в *errorMessage (если он не nullptr) и возвращает пустой вектор.
QVector<WordEntry> loadDictionaryFromFile(const QString& path,
                                          QString* errorMessage = nullptr);

#endif // WORDS_H
