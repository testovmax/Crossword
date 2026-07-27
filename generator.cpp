#include "generator.h"

#include <QRandomGenerator>
#include <QPair>
#include <QMap>
#include <algorithm>
#include <climits>

void CrosswordGenerator::resetState()
{
    m_grid = QVector<QVector<QChar>>(SIZE, QVector<QChar>(SIZE, QChar()));
    m_placed.clear();
    m_finalGrid.clear();
    m_numbers.clear();
    m_rows = 0;
    m_cols = 0;
}

int CrosswordGenerator::generateOnce(const QVector<WordEntry>& dictionary,
                                     int desiredCount)
{
    resetState();
    if (dictionary.isEmpty() || desiredCount <= 0) return 0;

    // Первое (самое длинное) слово — по горизонтали в центре
    const WordEntry& first = dictionary.first();
    place(first.word, first.clue, ORIGIN, ORIGIN - first.word.length() / 2, true);

    int placedCount = 1;
    for (int i = 1; i < dictionary.size() && placedCount < desiredCount; ++i) {
        if (tryFit(dictionary[i]))
            ++placedCount;
    }
    return placedCount;
}

int CrosswordGenerator::generate(QVector<WordEntry> dictionary, int desiredCount)
{
    // Нормализация: верхний регистр
    for (auto& e : dictionary)
        e.word = e.word.toUpper();

    auto* rng = QRandomGenerator::global();

    // Простая стратегия: перемешиваем словарь, сортируем по длине (длинные
    // вперёд — у них больше шансов на пересечения), запускаем одну попытку.
    // Если получилось мало слов, пробуем ещё пару раз с другим порядком.
    int bestPlaced = 0;
    QVector<QVector<QChar>> bestFinalGrid;
    QVector<PlacedWord>     bestPlacedWords;
    QVector<QVector<int>>   bestNumbers;
    int bestRows = 0, bestCols = 0;

    constexpr int ATTEMPTS = 5;
    for (int attempt = 0; attempt < ATTEMPTS; ++attempt) {
        QVector<WordEntry> dict = dictionary;
        for (int i = dict.size() - 1; i > 0; --i) {
            int j = int(rng->bounded(i + 1));
            dict.swapItemsAt(i, j);
        }
        std::stable_sort(dict.begin(), dict.end(),
            [](const WordEntry& a, const WordEntry& b) {
                return a.word.length() > b.word.length();
            });

        int placed = generateOnce(dict, desiredCount);
        finalize();

        // Берём вариант с максимальным числом размещённых слов.
        if (placed > bestPlaced) {
            bestPlaced       = placed;
            bestFinalGrid    = m_finalGrid;
            bestPlacedWords  = m_placed;
            bestNumbers      = m_numbers;
            bestRows         = m_rows;
            bestCols         = m_cols;
        }

        // Если разместили всё, что просили — хватит.
        if (bestPlaced >= desiredCount) break;
    }

    // Восстанавливаем лучший результат
    m_finalGrid = bestFinalGrid;
    m_placed    = bestPlacedWords;
    m_numbers   = bestNumbers;
    m_rows      = bestRows;
    m_cols      = bestCols;

    return bestPlaced;
}

bool CrosswordGenerator::canPlace(const QString& word, int row, int col,
                                  bool horizontal, bool requireIntersection) const
{
    const int len = word.length();
    const int dr = horizontal ? 0 : 1;
    const int dc = horizontal ? 1 : 0;

    // Границы
    if (row < 1 || col < 1 || row >= SIZE - 1 || col >= SIZE - 1)
        return false;
    int endR = row + dr * (len - 1);
    int endC = col + dc * (len - 1);
    if (endR < 1 || endC < 1 || endR >= SIZE - 1 || endC >= SIZE - 1)
        return false;

    // Ячейка ДО начала слова должна быть пустой
    int br = row - dr;
    int bc = col - dc;
    if (!m_grid[br][bc].isNull())
        return false;

    // Ячейка ПОСЛЕ конца слова должна быть пустой
    int ar = endR + dr;
    int ac = endC + dc;
    if (!m_grid[ar][ac].isNull())
        return false;

    bool hasIntersection = false;
    for (int i = 0; i < len; ++i) {
        int r = row + dr * i;
        int c = col + dc * i;
        QChar existing = m_grid[r][c];

        if (!existing.isNull()) {
            // Совпадение букв или конфликт
            if (existing != word[i])
                return false;
            hasIntersection = true;
        } else {
            // Перпендикулярные соседи должны быть пустыми
            // (чтобы избежать прилипания слов)
            int p1r = r + dc;  // перпендикуляр +
            int p1c = c + dr;
            int p2r = r - dc;  // перпендикуляр -
            int p2c = c - dr;
            if (!m_grid[p1r][p1c].isNull()) return false;
            if (!m_grid[p2r][p2c].isNull()) return false;
        }
    }

    if (requireIntersection && !hasIntersection)
        return false;

    return true;
}

void CrosswordGenerator::place(const QString& word, const QString& clue,
                               int row, int col, bool horizontal)
{
    const int len = word.length();
    const int dr = horizontal ? 0 : 1;
    const int dc = horizontal ? 1 : 0;

    for (int i = 0; i < len; ++i) {
        m_grid[row + dr * i][col + dc * i] = word[i];
    }

    PlacedWord pw;
    pw.word = word;
    pw.clue = clue;
    pw.row = row;
    pw.col = col;
    pw.horizontal = horizontal;
    m_placed.append(pw);
}

bool CrosswordGenerator::tryFit(const WordEntry& entry)
{
    const QString& w = entry.word;

    // Собираем все возможные точки пересечения
    struct Candidate {
        int row;
        int col;
        bool horizontal;
    };
    QVector<Candidate> candidates;

    for (int i = 0; i < w.length(); ++i) {
        QChar ch = w[i];
        // Сканируем уже размещённые слова в поисках совпадающей буквы
        for (const PlacedWord& pw : m_placed) {
            for (int j = 0; j < pw.word.length(); ++j) {
                if (pw.word[j] != ch) continue;

                // Точка пересечения
                int interR = pw.horizontal ? pw.row : pw.row + j;
                int interC = pw.horizontal ? pw.col + j : pw.col;

                // Новое слово размещаем перпендикулярно
                bool newHorizontal = !pw.horizontal;
                int startR = newHorizontal ? interR : interR - i;
                int startC = newHorizontal ? interC - i : interC;

                if (canPlace(w, startR, startC, newHorizontal, true)) {
                    candidates.append({ startR, startC, newHorizontal });
                }
            }
        }
    }

    if (candidates.isEmpty()) return false;

    // Текущий охватывающий прямоугольник уже размещённых букв.
    int curMinR = SIZE, curMaxR = -1, curMinC = SIZE, curMaxC = -1;
    for (int r = 0; r < SIZE; ++r) {
        for (int col = 0; col < SIZE; ++col) {
            if (!m_grid[r][col].isNull()) {
                curMinR = qMin(curMinR, r);
                curMaxR = qMax(curMaxR, r);
                curMinC = qMin(curMinC, col);
                curMaxC = qMax(curMaxC, col);
            }
        }
    }

    // Плотность: сколько букв-соседей у ячеек нового слова. Чем больше,
    // тем больше пересечений и/или прижимов к существующей кладке.
    auto density = [&](const Candidate& c) {
        int s = 0;
        int dr = c.horizontal ? 0 : 1;
        int dc = c.horizontal ? 1 : 0;
        for (int i = 0; i < w.length(); ++i) {
            int r = c.row + dr * i;
            int col = c.col + dc * i;
            if (!m_grid[r-1][col].isNull()) ++s;
            if (!m_grid[r+1][col].isNull()) ++s;
            if (!m_grid[r][col-1].isNull()) ++s;
            if (!m_grid[r][col+1].isNull()) ++s;
        }
        return s;
    };

    // Насколько новый bbox больше старого по сторонам.
    // Также штрафуем несимметричный bbox (длинная узкая полоса хуже квадрата).
    auto badness = [&](const Candidate& c) {
        int dr = c.horizontal ? 0 : 1;
        int dc = c.horizontal ? 1 : 0;
        int len = w.length();
        int nR1 = qMin(curMinR, c.row);
        int nR2 = qMax(curMaxR, c.row + dr * (len - 1));
        int nC1 = qMin(curMinC, c.col);
        int nC2 = qMax(curMaxC, c.col + dc * (len - 1));
        int h = nR2 - nR1 + 1;
        int w_ = nC2 - nC1 + 1;
        // Площадь + штраф за вытянутость
        return h * w_ + qAbs(h - w_) * 4;
    };

    // Плотность важнее всего, дальше — компактность bbox.
    auto score = [&](const Candidate& c) {
        return density(c) * 10000 - badness(c);
    };

    int best = INT_MIN;
    QVector<int> bestIdx;
    for (int i = 0; i < candidates.size(); ++i) {
        int s = score(candidates[i]);
        if (s > best)       { best = s; bestIdx.clear(); bestIdx.append(i); }
        else if (s == best) bestIdx.append(i);
    }

    auto* rng = QRandomGenerator::global();
    int idx = bestIdx[int(rng->bounded(bestIdx.size()))];
    const Candidate& c = candidates[idx];
    place(w, entry.clue, c.row, c.col, c.horizontal);
    return true;
}

void CrosswordGenerator::finalize()
{
    int minR = SIZE, maxR = -1, minC = SIZE, maxC = -1;
    for (int r = 0; r < SIZE; ++r) {
        for (int c = 0; c < SIZE; ++c) {
            if (!m_grid[r][c].isNull()) {
                minR = qMin(minR, r);
                maxR = qMax(maxR, r);
                minC = qMin(minC, c);
                maxC = qMax(maxC, c);
            }
        }
    }

    if (maxR < 0) {
        m_rows = 0;
        m_cols = 0;
        return;
    }

    m_rows = maxR - minR + 1;
    m_cols = maxC - minC + 1;

    m_finalGrid = QVector<QVector<QChar>>(m_rows, QVector<QChar>(m_cols, QChar()));
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            m_finalGrid[r][c] = m_grid[r + minR][c + minC];
        }
    }

    // Корректируем координаты слов
    for (auto& pw : m_placed) {
        pw.row -= minR;
        pw.col -= minC;
    }

    // Сортируем для стандартной нумерации (сверху вниз, слева направо)
    std::sort(m_placed.begin(), m_placed.end(),
        [](const PlacedWord& a, const PlacedWord& b) {
            if (a.row != b.row) return a.row < b.row;
            if (a.col != b.col) return a.col < b.col;
            return a.horizontal && !b.horizontal;
        });

    // Группируем по стартовой ячейке: одна и та же ячейка может быть началом
    // и горизонтального, и вертикального слова — у них общий номер.
    m_numbers = QVector<QVector<int>>(m_rows, QVector<int>(m_cols, 0));
    int currentNumber = 0;
    for (int i = 0; i < m_placed.size(); ++i) {
        auto& pw = m_placed[i];
        int existing = m_numbers[pw.row][pw.col];
        if (existing == 0) {
            ++currentNumber;
            m_numbers[pw.row][pw.col] = currentNumber;
            pw.number = currentNumber;
        } else {
            pw.number = existing;
        }
    }
}

QChar CrosswordGenerator::cellAt(int r, int c) const
{
    if (r < 0 || c < 0 || r >= m_rows || c >= m_cols) return QChar();
    return m_finalGrid[r][c];
}

int CrosswordGenerator::numberAt(int r, int c) const
{
    if (r < 0 || c < 0 || r >= m_rows || c >= m_cols) return 0;
    return m_numbers[r][c];
}
