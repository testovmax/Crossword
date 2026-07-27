#ifndef CELLWIDGET_H
#define CELLWIDGET_H

#include <QWidget>
#include <QChar>

class QLineEdit;

// Одна ячейка кроссворда. Полностью рисуется в paintEvent,
// чтобы избежать рассогласования соседних клеток.
class CellWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CellWidget(int cellSize = 84, QWidget* parent = nullptr);

    int cellSize() const { return m_cellSize; }

    void setBlack();                            // прозрачная пустая ячейка
    void setLetter(QChar correctLetter, int number = 0);

    bool isBlack() const { return m_black; }
    QChar correctLetter() const { return m_correct; }
    QString currentLetter() const;
    void setCurrentLetter(const QString& s);

    // 0 — нейтрально, 1 — верно (зелёный фон), -1 — ошибка (красный фон)
    void setHighlight(int state);

    QLineEdit* edit() const { return m_edit; }

signals:
    void letterEntered();

protected:
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;

private:
    bool m_black = true;
    QChar m_correct;
    int   m_number = 0;
    int   m_highlight = 0;
    int   m_cellSize = 84;
    QLineEdit* m_edit = nullptr;
};

#endif // CELLWIDGET_H
