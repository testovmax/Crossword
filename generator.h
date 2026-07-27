#ifndef GENERATOR_H
#define GENERATOR_H

#include <QString>
#include <QVector>
#include "words.h"

struct PlacedWord {
    QString word;
    QString clue;
    int row = 0;
    int col = 0;
    bool horizontal = true;
    int number = 0;
};

class CrosswordGenerator
{
public:
    // Пытается разместить как можно больше слов из словаря
    // (но не более desiredCount). Возвращает количество
    // фактически размещённых слов.
    int generate(QVector<WordEntry> dictionary, int desiredCount);

    int rows() const { return m_rows; }
    int cols() const { return m_cols; }

    // Возвращает букву ячейки или null QChar, если ячейка пустая (чёрная)
    QChar cellAt(int r, int c) const;

    // Номер ячейки в кроссворде (если в ней начинается слово), иначе 0
    int numberAt(int r, int c) const;

    QVector<PlacedWord> placedWords() const { return m_placed; }

private:
    static constexpr int SIZE = 64;
    static constexpr int ORIGIN = SIZE / 2;

    QVector<QVector<QChar>>  m_grid;       // SIZE x SIZE
    QVector<PlacedWord>      m_placed;

    QVector<QVector<QChar>>  m_finalGrid;  // обрезанная сетка
    QVector<QVector<int>>    m_numbers;    // номера ячеек
    int m_rows = 0;
    int m_cols = 0;

    bool canPlace(const QString& word, int row, int col, bool horizontal,
                  bool requireIntersection) const;
    void place(const QString& word, const QString& clue,
               int row, int col, bool horizontal);
    bool tryFit(const WordEntry& entry);
    void finalize();

    // Одна попытка генерации с заданным (уже перемешанным) словарём.
    // Возвращает количество размещённых слов.
    int  generateOnce(const QVector<WordEntry>& dictionary, int desiredCount);
    void resetState();
};

#endif // GENERATOR_H
