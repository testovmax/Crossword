#include "cellwidget.h"

#include <QLineEdit>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QSignalBlocker>

CellWidget::CellWidget(int cellSize, QWidget* parent)
    : QWidget(parent), m_cellSize(cellSize)
{
    setFixedSize(m_cellSize, m_cellSize);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setBlack();
}

void CellWidget::setBlack()
{
    m_black = true;
    m_correct = QChar();
    m_number = 0;
    m_highlight = 0;

    if (m_edit) {
        m_edit->deleteLater();
        m_edit = nullptr;
    }
    update();
}

void CellWidget::setLetter(QChar correctLetter, int number)
{
    m_black = false;
    m_correct = correctLetter.toUpper();
    m_number = number;
    m_highlight = 0;

    if (!m_edit) {
        const int fontSize = qMax(10, int(m_cellSize * 0.5));
        const int topPad   = qMax(10, m_cellSize / 5);

        m_edit = new QLineEdit(this);
        m_edit->setMaxLength(1);
        m_edit->setAlignment(Qt::AlignCenter);
        m_edit->setFrame(false);
        m_edit->setStyleSheet(QString(
            "QLineEdit {"
            " background: transparent;"
            " border: none;"
            " font-size: %1px;"
            " font-weight: bold;"
            " color: #111;"
            "}").arg(fontSize));
        m_edit->setGeometry(2, topPad, m_cellSize - 4, m_cellSize - topPad - 2);

        connect(m_edit, &QLineEdit::textChanged, this, [this](const QString& t) {
            QString up = t.toUpper();
            if (up != t) {
                int pos = m_edit->cursorPosition();
                QSignalBlocker b(m_edit);
                m_edit->setText(up);
                m_edit->setCursorPosition(pos);
            }
            if (!up.isEmpty())
                emit letterEntered();
        });
    }
    m_edit->show();
    m_edit->clear();
    update();
}

QString CellWidget::currentLetter() const
{
    if (!m_edit) return QString();
    return m_edit->text().toUpper();
}

void CellWidget::setCurrentLetter(const QString& s)
{
    if (m_edit) m_edit->setText(s);
}

void CellWidget::setHighlight(int state)
{
    if (m_black) return;
    m_highlight = state;
    update();
}

void CellWidget::mousePressEvent(QMouseEvent* e)
{
    // Любой клик по ячейке отдаёт фокус полю ввода, даже если попали мимо него
    if (!m_black && m_edit) m_edit->setFocus();
    QWidget::mousePressEvent(e);
}

void CellWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    if (m_black) {
        // Пустая ячейка — полностью прозрачная, ничего не рисуем
        return;
    }

    // Цвет фона по состоянию
    QColor bg(Qt::white);
    switch (m_highlight) {
        case  1: bg = QColor("#c8f7c5"); break; // верно
        case -1: bg = QColor("#f7c5c5"); break; // ошибка
        default: bg = Qt::white;
    }

    // Заливка ровно по границам виджета
    p.fillRect(rect(), bg);

    // Рамка: используем целочисленные координаты + смещение 0.5,
    // чтобы линия легла на пиксель, а не между ними.
    QPen pen(QColor("#444"));
    pen.setWidth(1);
    pen.setCosmetic(true);
    p.setPen(pen);
    p.drawRect(0, 0, width() - 1, height() - 1);

    // Номер слова в верхнем-левом углу
    if (m_number > 0) {
        const int numFont = qMax(8, m_cellSize / 7);
        p.setPen(QColor("#444"));
        QFont f = p.font();
        f.setPixelSize(numFont);
        f.setBold(true);
        p.setFont(f);
        p.drawText(QRect(3, 1, m_cellSize / 2, numFont + 4),
                   Qt::AlignLeft | Qt::AlignTop,
                   QString::number(m_number));
    }
}
