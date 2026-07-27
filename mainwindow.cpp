#include "mainwindow.h"
#include "cellwidget.h"
#include "words.h"

#include <QApplication>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QGroupBox>
#include <QTimer>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QLineEdit>
#include <QEvent>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QRandomGenerator>
#include <QDialog>
#include <QTextBrowser>
#include <QDialogButtonBox>
#include <QDateTime>
#include <QFile>
#include <QPair>

// ===== Описания достижений =====
namespace {

struct AchievementDef {
    QString id;
    QString name;
    QString description;
};

QVector<AchievementDef> allAchievements()
{
    return {
        { "first_solve",     QString::fromUtf8("Первый шаг"),
                             QString::fromUtf8("Решите свой первый кроссворд") },
        { "solve_5",         QString::fromUtf8("Энтузиаст"),
                             QString::fromUtf8("Решите 5 кроссвордов") },
        { "solve_25",        QString::fromUtf8("Завсегдатай"),
                             QString::fromUtf8("Решите 25 кроссвордов") },
        { "no_hints",        QString::fromUtf8("Без помощи"),
                             QString::fromUtf8("Решите кроссворд без подсказок") },
        { "no_hints_5",      QString::fromUtf8("Самостоятельный"),
                             QString::fromUtf8("Решите 5 кроссвордов подряд без подсказок") },
        { "speed_run",       QString::fromUtf8("Скоростной"),
                             QString::fromUtf8("Решите кроссворд (≥5 слов) меньше чем за 2 минуты") },
        { "big_solve",       QString::fromUtf8("Тяжеловес"),
                             QString::fromUtf8("Решите кроссворд из 15 и более слов") },
        { "streak_3",        QString::fromUtf8("Серия из трёх"),
                             QString::fromUtf8("Решите 3 кроссворда подряд") },
        { "streak_10",       QString::fromUtf8("Несгибаемый"),
                             QString::fromUtf8("Решите 10 кроссвордов подряд") },
        { "three_stars",     QString::fromUtf8("На все звёзды"),
                             QString::fromUtf8("Решите кроссворд на три звезды") }
    };
}

QString statsFilePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + "/crossword_stats.json";
}

} // namespace


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QString::fromUtf8("Генератор кроссвордов"));
    resize(1400, 900);

    setStyleSheet(
        "QPushButton { padding: 8px 16px; font-size: 16px; }"
        "QSpinBox    { padding: 6px 8px;  font-size: 16px; min-width: 70px; }"
        "QLabel      { font-size: 16px; }"
        "QGroupBox   { font-size: 16px; font-weight: bold; margin-top: 14px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
    );

    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    // --- Верхняя панель ---
    QHBoxLayout* topBar = new QHBoxLayout();
    topBar->setSpacing(10);
    topBar->setContentsMargins(8, 8, 8, 8);
    topBar->addWidget(new QLabel(QString::fromUtf8("Количество слов:")));

    m_countSpin = new QSpinBox();
    m_countSpin->setRange(2, 999);
    m_countSpin->setValue(10);
    topBar->addWidget(m_countSpin);

    m_generateBtn = new QPushButton(QString::fromUtf8("Сгенерировать"));
    topBar->addWidget(m_generateBtn);

    m_hintBtn = new QPushButton(QString::fromUtf8("Подсказка"));
    m_hintBtn->setToolTip(QString::fromUtf8(
        "Открыть правильную букву в текущей выделенной ячейке"));
    m_hintBtn->setFocusPolicy(Qt::NoFocus);  // не забирает фокус у ячейки
    topBar->addWidget(m_hintBtn);

    m_clearBtn = new QPushButton(QString::fromUtf8("Очистить"));
    topBar->addWidget(m_clearBtn);

    m_loadDictBtn = new QPushButton(QString::fromUtf8("Загрузить словарь..."));
    topBar->addWidget(m_loadDictBtn);

    m_zoomOutBtn = new QPushButton(QString::fromUtf8("−"));
    m_zoomOutBtn->setToolTip(QString::fromUtf8("Уменьшить"));
    m_zoomOutBtn->setFixedWidth(44);
    topBar->addWidget(m_zoomOutBtn);

    m_zoomInBtn = new QPushButton(QString::fromUtf8("+"));
    m_zoomInBtn->setToolTip(QString::fromUtf8("Увеличить"));
    m_zoomInBtn->setFixedWidth(44);
    topBar->addWidget(m_zoomInBtn);

    m_achievementsBtn = new QPushButton(QString::fromUtf8("Достижения"));
    topBar->addWidget(m_achievementsBtn);

    topBar->addStretch(1);

    m_timeLabel = new QLabel(QString::fromUtf8("Время: 00:00"));
    m_timeLabel->setStyleSheet("color: #333; font-size: 18px; font-weight: bold;");
    topBar->addWidget(m_timeLabel);

    m_streakLabel = new QLabel();
    m_streakLabel->setStyleSheet("color: #c45100; font-size: 16px; font-weight: bold;");
    topBar->addWidget(m_streakLabel);

    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color: #2a4a8a; font-size: 16px;");
    topBar->addWidget(m_statusLabel);

    // --- Сетка ---
    m_gridHost = new QWidget();
    m_gridLayout = new QGridLayout(m_gridHost);
    m_gridLayout->setSpacing(0);
    m_gridLayout->setHorizontalSpacing(0);
    m_gridLayout->setVerticalSpacing(0);
    m_gridLayout->setContentsMargins(12, 12, 12, 12);

    m_gridScroll = new QScrollArea();
    m_gridScroll->setWidget(m_gridHost);
    m_gridScroll->setWidgetResizable(false);
    m_gridScroll->setAlignment(Qt::AlignCenter);
    m_gridScroll->setMinimumWidth(560);

    // --- Списки вопросов ---
    QGroupBox* acrossBox = new QGroupBox(QString::fromUtf8("По горизонтали"));
    QVBoxLayout* aBoxLay = new QVBoxLayout(acrossBox);
    m_acrossList = new QListWidget();
    m_acrossList->setWordWrap(true);
    m_acrossList->setStyleSheet(
        "QListWidget { font-size: 17px; padding: 6px; }"
        "QListWidget::item { padding: 4px 2px; }"
    );
    aBoxLay->addWidget(m_acrossList);

    QGroupBox* downBox = new QGroupBox(QString::fromUtf8("По вертикали"));
    QVBoxLayout* dBoxLay = new QVBoxLayout(downBox);
    m_downList = new QListWidget();
    m_downList->setWordWrap(true);
    m_downList->setStyleSheet(
        "QListWidget { font-size: 17px; padding: 6px; }"
        "QListWidget::item { padding: 4px 2px; }"
    );
    dBoxLay->addWidget(m_downList);

    QVBoxLayout* cluesLay = new QVBoxLayout();
    cluesLay->addWidget(acrossBox);
    cluesLay->addWidget(downBox);

    QHBoxLayout* middle = new QHBoxLayout();
    middle->addWidget(m_gridScroll, 3);
    middle->addLayout(cluesLay, 2);

    QVBoxLayout* mainLay = new QVBoxLayout(central);
    mainLay->addLayout(topBar);
    mainLay->addLayout(middle, 1);

    // Таймер UI (тикает каждую секунду, обновляет время)
    m_uiTimer = new QTimer(this);
    m_uiTimer->setInterval(1000);
    connect(m_uiTimer, &QTimer::timeout, this, &MainWindow::onTimerTick);

    // --- Сигналы ---
    connect(m_generateBtn,     &QPushButton::clicked, this, &MainWindow::onGenerate);
    connect(m_hintBtn,         &QPushButton::clicked, this, &MainWindow::onHint);
    connect(m_clearBtn,        &QPushButton::clicked, this, &MainWindow::onClear);
    connect(m_loadDictBtn,     &QPushButton::clicked, this, &MainWindow::onLoadDictionary);
    connect(m_zoomInBtn,       &QPushButton::clicked, this, &MainWindow::onZoomIn);
    connect(m_zoomOutBtn,      &QPushButton::clicked, this, &MainWindow::onZoomOut);
    connect(m_achievementsBtn, &QPushButton::clicked, this, &MainWindow::onShowAchievements);

    loadStats();
    updateHintButtonText();
    updateStatusBar();

    QTimer::singleShot(0, this, &MainWindow::promptForCount);
}

MainWindow::~MainWindow()
{
    saveStats();
}

// ===== Словарь =====

bool MainWindow::tryLoadDictionary(const QString& path, QString* errorMessage)
{
    QString err;
    QVector<WordEntry> dict = loadDictionaryFromFile(path, &err);
    if (dict.isEmpty()) {
        if (errorMessage) *errorMessage = err;
        return false;
    }
    m_dictionary     = dict;
    m_dictionaryPath = path;
    return true;
}

bool MainWindow::ensureDictionaryLoaded()
{
    if (!m_dictionary.isEmpty()) return true;

    QStringList candidates;
    candidates << QCoreApplication::applicationDirPath() + "/words.txt"
               << QDir::currentPath() + "/words.txt"
               << "./words.txt";

    for (const QString& p : candidates) {
        if (QFileInfo::exists(p)) {
            QString err;
            if (tryLoadDictionary(p, &err)) {
                m_statusLabel->setText(QString::fromUtf8(
                    "Словарь загружен: %1 слов").arg(m_dictionary.size()));
                return true;
            }
        }
    }

    QMessageBox::information(this,
        QString::fromUtf8("Словарь не найден"),
        QString::fromUtf8("Файл words.txt не найден. Выберите его."));

    onLoadDictionary();
    return !m_dictionary.isEmpty();
}

void MainWindow::onLoadDictionary()
{
    QString path = QFileDialog::getOpenFileName(this,
        QString::fromUtf8("Выберите файл словаря"),
        QDir::currentPath(),
        QString::fromUtf8("Текстовые файлы (*.txt);;Все файлы (*)"));
    if (path.isEmpty()) return;

    QString err;
    if (!tryLoadDictionary(path, &err)) {
        QMessageBox::warning(this,
            QString::fromUtf8("Ошибка загрузки словаря"), err);
        return;
    }
    m_statusLabel->setText(QString::fromUtf8(
        "Словарь загружен: %1 слов").arg(m_dictionary.size()));
}

void MainWindow::promptForCount()
{
    if (!ensureDictionaryLoaded()) {
        m_statusLabel->setText(QString::fromUtf8(
            "Загрузите словарь и нажмите «Сгенерировать»"));
        return;
    }
    onGenerate();
}

// ===== Генерация =====

void MainWindow::onGenerate()
{
    if (!ensureDictionaryLoaded()) return;

    // Если предыдущий кроссворд начат, но не дорешан — сбрасываем серию.
    if (m_gameActive && !m_gameSolved) {
        if (m_currentStreak > 0) {
            m_currentStreak = 0;
            saveStats();
        }
    }

    int desired = m_countSpin->value();
    const int maxAvailable = m_dictionary.size();

    if (desired > maxAvailable) {
        QMessageBox::warning(this,
            QString::fromUtf8("Превышен размер словаря"),
            QString::fromUtf8(
                "Вы запросили %1 слов, но в словаре только %2.\n"
                "Будет создан кроссворд из %2 слов.")
                .arg(desired).arg(maxAvailable));
        desired = maxAvailable;
        m_countSpin->setValue(desired);
    }

    int placed = m_gen.generate(m_dictionary, desired);
    if (placed == 0) {
        QMessageBox::warning(this,
            QString::fromUtf8("Ошибка"),
            QString::fromUtf8("Не удалось сгенерировать кроссворд."));
        return;
    }

    rebuildGrid();
    rebuildClueLists();
    resetGameState();
    startGameTimer();
    updateHintButtonText();
    updateStatusBar();

    QString msg = QString::fromUtf8("Размещено слов: %1 из %2")
                      .arg(placed).arg(desired);
    if (placed < desired) msg += QString::fromUtf8("  (не все слова удалось вписать)");
    m_statusLabel->setText(msg);
}

void MainWindow::rebuildGrid()
{
    while (QLayoutItem* item = m_gridLayout->takeAt(0)) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }
    m_cells.clear();

    int R = m_gen.rows();
    int C = m_gen.cols();
    m_cells.resize(R);

    for (int r = 0; r < R; ++r) {
        m_cells[r].resize(C);
        for (int c = 0; c < C; ++c) {
            CellWidget* cell = new CellWidget(m_cellSize, m_gridHost);
            QChar ch = m_gen.cellAt(r, c);
            if (!ch.isNull()) {
                cell->setLetter(ch, m_gen.numberAt(r, c));
                connect(cell, &CellWidget::letterEntered, this,
                        [this, r, c]() {
                            // Сбрасываем подсветку ячейки, в которой только
                                            // что изменили букву (была зелёной/красной — теперь требует
                                            // пересчёта). checkWordCompletions ниже её при необходимости
                            // снова сделает зелёной.
                            if (m_cells[r][c]) m_cells[r][c]->setHighlight(0);
                            advanceFocus(r, c);
                            checkWordCompletions();
                        });
                if (cell->edit()) {
                    cell->edit()->installEventFilter(this);
                    // Enter проверяет текущее слово (зелёное/красное)
                    connect(cell->edit(), &QLineEdit::returnPressed,
                            this, &MainWindow::onWordCheck);
                }
            } else {
                cell->setBlack();
            }
            m_gridLayout->addWidget(cell, r, c);
            m_cells[r][c] = cell;
            cell->show();
        }
    }

    const int margin = 24;
    int hostW = C * m_cellSize + margin;
    int hostH = R * m_cellSize + margin;
    m_gridHost->setMinimumSize(hostW, hostH);
    m_gridHost->resize(hostW, hostH);
    m_gridHost->show();

    // Автоматически ставим фокус на первую ячейку слова — чтобы кнопка
    // «Подсказка» уже знала, в какую ячейку писать.
    QTimer::singleShot(0, this, [this]() {
        for (int r = 0; r < m_cells.size(); ++r) {
            for (int c = 0; c < m_cells[r].size(); ++c) {
                CellWidget* cell = m_cells[r][c];
                if (cell && !cell->isBlack() && cell->edit()) {
                    cell->edit()->setFocus();
                    m_lastFocusedR = r;
                    m_lastFocusedC = c;
                    return;
                }
            }
        }
    });
}

void MainWindow::rebuildClueLists()
{
    m_acrossList->clear();
    m_downList->clear();
    auto words = m_gen.placedWords();
    std::sort(words.begin(), words.end(),
        [](const PlacedWord& a, const PlacedWord& b) { return a.number < b.number; });
    for (const PlacedWord& pw : words) {
        QString line = QString::fromUtf8("%1. %2  (%3 букв)")
                           .arg(pw.number).arg(pw.clue).arg(pw.word.length());
        if (pw.horizontal) m_acrossList->addItem(line);
        else               m_downList->addItem(line);
    }
}

// ===== Проверка слова по Enter =====

void MainWindow::onWordCheck()
{
    if (m_cells.isEmpty()) return;

    // Определяем активную ячейку
    int r = -1, c = -1;
    QWidget* focus = QApplication::focusWidget();
    if (focus) findCellOfEdit(focus, r, c);
    if (r < 0 && m_lastFocusedR >= 0
        && m_lastFocusedR < m_cells.size()
        && m_lastFocusedC < m_cells[m_lastFocusedR].size()) {
        r = m_lastFocusedR;
        c = m_lastFocusedC;
    }
    if (r < 0) return;

    // Ищем слово, проходящее через (r, c) в текущем направлении.
    // Если в текущем направлении слова нет — пробуем перпендикулярное.
    auto words = m_gen.placedWords();
    const PlacedWord* target = nullptr;

    auto findIn = [&](bool wantHoriz) -> const PlacedWord* {
        for (const PlacedWord& pw : words) {
            if (pw.horizontal != wantHoriz) continue;
            int len = pw.word.length();
            if (pw.horizontal) {
                if (pw.row == r && pw.col <= c && c < pw.col + len) return &pw;
            } else {
                if (pw.col == c && pw.row <= r && r < pw.row + len) return &pw;
            }
        }
        return nullptr;
    };

    target = findIn(m_directionHorizontal);
    if (!target) target = findIn(!m_directionHorizontal);
    if (!target) return;

    // Подсвечиваем буквы текущего слова: зелёным или красным
    const int dr = target->horizontal ? 0 : 1;
    const int dc = target->horizontal ? 1 : 0;
    const int len = target->word.length();
    bool allCorrect = true;
    for (int i = 0; i < len; ++i) {
        int rr = target->row + dr * i;
        int cc = target->col + dc * i;
        CellWidget* cell = m_cells[rr][cc];
        if (!cell) continue;
        QString cur = cell->currentLetter();
        if (cur.isEmpty()) {
            cell->setHighlight(0);
            allCorrect = false;
        } else if (cur[0] == cell->correctLetter()) {
            cell->setHighlight(1);
        } else {
            cell->setHighlight(-1);
            allCorrect = false;
        }
    }

    if (allCorrect) m_completedWordNumbers.insert(target->number);

    // Применяем блокировку ко всем сейчас правильным словам
    checkWordCompletions();

    // Если весь кроссворд правильно заполнен — поздравительный диалог
    bool entireSolved = true;
    for (int rr = 0; rr < m_cells.size() && entireSolved; ++rr) {
        for (int cc = 0; cc < m_cells[rr].size(); ++cc) {
            CellWidget* cell = m_cells[rr][cc];
            if (!cell || cell->isBlack()) continue;
            QString cur = cell->currentLetter();
            if (cur.isEmpty() || cur[0] != cell->correctLetter()) {
                entireSolved = false;
                break;
            }
        }
    }
    if (entireSolved) onSolved();
}

// ===== Подсказка =====

void MainWindow::onHint()
{
    if (m_cells.isEmpty()) {
        m_statusLabel->setText(QString::fromUtf8("Сначала сгенерируйте кроссворд"));
        return;
    }
    if (m_hintsUsed >= m_hintsLimit) {
        QMessageBox::information(this,
            QString::fromUtf8("Подсказки закончились"),
            QString::fromUtf8("На этот кроссворд подсказок больше нет."));
        return;
    }

    // Ищем активную ячейку: сначала по реальному фокусу, потом по
    // последней запомненной позиции (на случай, если фокус всё-таки ушёл).
    int r = -1, c = -1;
    QWidget* focus = QApplication::focusWidget();
    if (focus) findCellOfEdit(focus, r, c);
    if (r < 0 && m_lastFocusedR >= 0
        && m_lastFocusedR < m_cells.size()
        && m_lastFocusedC < m_cells[m_lastFocusedR].size()) {
        r = m_lastFocusedR;
        c = m_lastFocusedC;
    }

    if (r < 0) {
        m_statusLabel->setText(QString::fromUtf8(
            "Кликните на нужную ячейку — подсказка появится в ней"));
        return;
    }

    CellWidget* cell = m_cells[r][c];
    if (!cell || cell->isBlack()) return;

    QString cur = cell->currentLetter();
    if (!cur.isEmpty() && cur[0] == cell->correctLetter()) {
        m_statusLabel->setText(QString::fromUtf8(
            "В этой ячейке уже правильная буква — подсказка не нужна"));
        return;
    }

    cell->setCurrentLetter(QString(cell->correctLetter()));
    cell->setHighlight(0);
    ++m_hintsUsed;
    updateHintButtonText();
    checkWordCompletions();

    // Сразу переходим к следующей букве, как при обычном вводе
    advanceFocus(r, c);
}

// ===== Очистка =====

void MainWindow::onClear()
{
    if (m_cells.isEmpty()) return;
    // Сначала снимаем блокировку, иначе setText на read-only не сработает
    for (int r = 0; r < m_cells.size(); ++r) {
        for (int c = 0; c < m_cells[r].size(); ++c) {
            CellWidget* cell = m_cells[r][c];
            if (!cell || cell->isBlack()) continue;
            if (cell->edit()) cell->edit()->setReadOnly(false);
            cell->setCurrentLetter(QString());
            cell->setHighlight(0);
        }
    }
    m_completedWordNumbers.clear();
    checkWordCompletions();
}

// ===== Масштабирование =====

QVector<QVector<QString>> MainWindow::collectLetters() const
{
    QVector<QVector<QString>> result(m_cells.size());
    for (int r = 0; r < m_cells.size(); ++r) {
        result[r].resize(m_cells[r].size());
        for (int c = 0; c < m_cells[r].size(); ++c) {
            CellWidget* cell = m_cells[r][c];
            if (cell && !cell->isBlack())
                result[r][c] = cell->currentLetter();
        }
    }
    return result;
}

void MainWindow::restoreLetters(const QVector<QVector<QString>>& letters)
{
    for (int r = 0; r < m_cells.size() && r < letters.size(); ++r) {
        for (int c = 0; c < m_cells[r].size() && c < letters[r].size(); ++c) {
            CellWidget* cell = m_cells[r][c];
            if (cell && !cell->isBlack() && !letters[r][c].isEmpty())
                cell->setCurrentLetter(letters[r][c]);
        }
    }
}

void MainWindow::onZoomIn()
{
    if (m_cellSize >= 140) return;
    auto saved = m_cells.isEmpty() ? QVector<QVector<QString>>() : collectLetters();
    m_cellSize += 12;
    if (!m_cells.isEmpty()) { rebuildGrid(); restoreLetters(saved); }
}

void MainWindow::onZoomOut()
{
    if (m_cellSize <= 36) return;
    auto saved = m_cells.isEmpty() ? QVector<QVector<QString>>() : collectLetters();
    m_cellSize -= 12;
    if (!m_cells.isEmpty()) { rebuildGrid(); restoreLetters(saved); }
}

// ===== Авто-переход курсора =====

static bool isLetterCell(const QVector<QVector<CellWidget*>>& cells, int r, int c)
{
    if (r < 0 || r >= cells.size()) return false;
    if (c < 0 || c >= cells[r].size()) return false;
    CellWidget* cell = cells[r][c];
    return cell && !cell->isBlack();
}

void MainWindow::advanceFocus(int r, int c)
{
    bool hasRight = isLetterCell(m_cells, r,     c + 1);
    bool hasDown  = isLetterCell(m_cells, r + 1, c);

    bool goRight = false, goDown = false;
    if (m_directionHorizontal && hasRight)        goRight = true;
    else if (!m_directionHorizontal && hasDown)   goDown  = true;
    else if (hasRight)                            { goRight = true; m_directionHorizontal = true;  }
    else if (hasDown)                             { goDown  = true; m_directionHorizontal = false; }

    CellWidget* next = nullptr;
    if (goRight) next = m_cells[r][c + 1];
    if (goDown)  next = m_cells[r + 1][c];

    if (next && next->edit()) {
        next->edit()->setFocus();
        next->edit()->selectAll();
    }
}

void MainWindow::setDirectionForCell(int r, int c)
{
    bool inH = isLetterCell(m_cells, r, c - 1) || isLetterCell(m_cells, r, c + 1);
    bool inV = isLetterCell(m_cells, r - 1, c) || isLetterCell(m_cells, r + 1, c);

    if (inH && !inV)      m_directionHorizontal = true;
    else if (inV && !inH) m_directionHorizontal = false;
}

bool MainWindow::findCellOfEdit(QObject* obj, int& outR, int& outC) const
{
    for (int r = 0; r < m_cells.size(); ++r) {
        for (int c = 0; c < m_cells[r].size(); ++c) {
            CellWidget* cell = m_cells[r][c];
            if (cell && cell->edit() == obj) { outR = r; outC = c; return true; }
        }
    }
    return false;
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::FocusIn) {
        int r, c;
        if (findCellOfEdit(obj, r, c)) {
            setDirectionForCell(r, c);
            m_lastFocusedR = r;
            m_lastFocusedC = c;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// ===== Геймификация =====

void MainWindow::resetGameState()
{
    m_gameActive   = true;
    m_gameSolved   = false;
    m_gameStartMs  = QDateTime::currentMSecsSinceEpoch();
    m_gameElapsedMs = 0;
    m_hintsUsed    = 0;
    m_completedWordNumbers.clear();
}

void MainWindow::startGameTimer()
{
    if (m_uiTimer) m_uiTimer->start();
    onTimerTick();
}

void MainWindow::stopGameTimer()
{
    if (m_uiTimer) m_uiTimer->stop();
}

void MainWindow::onTimerTick()
{
    if (!m_gameActive) return;
    m_gameElapsedMs = QDateTime::currentMSecsSinceEpoch() - m_gameStartMs;
    m_timeLabel->setText(QString::fromUtf8("Время: %1").arg(formatTime(m_gameElapsedMs)));
}

QString MainWindow::formatTime(qint64 ms) const
{
    qint64 totalSec = ms / 1000;
    int m = int(totalSec / 60);
    int s = int(totalSec % 60);
    return QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
}

void MainWindow::updateHintButtonText()
{
    int left = m_hintsLimit - m_hintsUsed;
    if (left < 0) left = 0;
    m_hintBtn->setText(QString::fromUtf8("Подсказка (%1)").arg(left));
    m_hintBtn->setEnabled(left > 0);
}

void MainWindow::updateStatusBar()
{
    QString streak;
    if (m_currentStreak > 0)
        streak += QString::fromUtf8("Серия: %1").arg(m_currentStreak);
    if (m_bestStreak > 0)
        streak += QString::fromUtf8("   (рекорд %1)").arg(m_bestStreak);
    m_streakLabel->setText(streak);
}

// ===== Сохранение/загрузка статистики =====

void MainWindow::loadStats()
{
    QFile f(statsFilePath());
    if (!f.open(QIODevice::ReadOnly)) return;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError) return;

    QJsonObject o = doc.object();
    m_totalSolved      = o.value("totalSolved").toInt();
    m_currentStreak    = o.value("currentStreak").toInt();
    m_bestStreak       = o.value("bestStreak").toInt();
    m_totalSolveTimeMs = qint64(o.value("totalSolveTimeMs").toDouble());

    m_bestTimePerSize.clear();
    QJsonObject best = o.value("bestTimePerSize").toObject();
    for (auto it = best.begin(); it != best.end(); ++it)
        m_bestTimePerSize[it.key().toInt()] = qint64(it.value().toDouble());

    m_unlockedAchievements.clear();
    QJsonArray ach = o.value("achievements").toArray();
    for (const QJsonValue& v : ach)
        m_unlockedAchievements.insert(v.toString());
}

void MainWindow::saveStats()
{
    QJsonObject o;
    o["totalSolved"]      = m_totalSolved;
    o["currentStreak"]    = m_currentStreak;
    o["bestStreak"]       = m_bestStreak;
    o["totalSolveTimeMs"] = double(m_totalSolveTimeMs);

    QJsonObject best;
    for (auto it = m_bestTimePerSize.begin(); it != m_bestTimePerSize.end(); ++it)
        best[QString::number(it.key())] = double(it.value());
    o["bestTimePerSize"] = best;

    QJsonArray ach;
    for (const QString& id : m_unlockedAchievements) ach.append(id);
    o["achievements"] = ach;

    QFile f(statsFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
}

// ===== Победа =====

int MainWindow::computeStars(qint64 elapsedMs, int hintsUsed, int wordCount) const
{
    // 3⭐: без подсказок и быстро (≤ wordCount * 8 секунд)
    // 2⭐: ≤ 2 подсказки и средне (≤ wordCount * 15 секунд)
    // 1⭐: иначе (решил вообще)
    qint64 elapsedSec = elapsedMs / 1000;
    qint64 fastLimit  = qint64(wordCount) * 8;
    qint64 medLimit   = qint64(wordCount) * 15;

    if (hintsUsed == 0 && elapsedSec <= fastLimit) return 3;
    if (hintsUsed <= 2 && elapsedSec <= medLimit)  return 2;
    return 1;
}

QStringList MainWindow::unlockNewAchievements(qint64 elapsedMs, int hintsUsed,
                                              int wordCount, int stars)
{
    QStringList newly;
    auto unlock = [&](const QString& id) {
        if (m_unlockedAchievements.contains(id)) return;
        m_unlockedAchievements.insert(id);
        for (const auto& a : allAchievements()) {
            if (a.id == id) { newly << a.name; break; }
        }
    };

    if (m_totalSolved >= 1)  unlock("first_solve");
    if (m_totalSolved >= 5)  unlock("solve_5");
    if (m_totalSolved >= 25) unlock("solve_25");
    if (hintsUsed == 0)      unlock("no_hints");
    if (wordCount >= 15)     unlock("big_solve");
    if (stars == 3)          unlock("three_stars");
    if (m_currentStreak >= 3)  unlock("streak_3");
    if (m_currentStreak >= 10) unlock("streak_10");

    qint64 sec = elapsedMs / 1000;
    if (wordCount >= 5 && sec < 120) unlock("speed_run");

    // «no_hints_5»: посчитаем грубо — если последние 5 решений без подсказок,
    // помечаем сразу при условии текущей серии без подсказок (упрощённо).
    if (hintsUsed == 0 && m_currentStreak >= 5) unlock("no_hints_5");

    return newly;
}

void MainWindow::onSolved()
{
    if (m_gameSolved) return;
    m_gameSolved = true;
    m_gameActive = false;
    stopGameTimer();

    qint64 elapsed = m_gameElapsedMs;
    int wordCount  = m_gen.placedWords().size();
    int stars      = computeStars(elapsed, m_hintsUsed, wordCount);

    // Обновляем статистику
    ++m_totalSolved;
    ++m_currentStreak;
    if (m_currentStreak > m_bestStreak) m_bestStreak = m_currentStreak;
    m_totalSolveTimeMs += elapsed;

    if (!m_bestTimePerSize.contains(wordCount)
        || elapsed < m_bestTimePerSize[wordCount])
        m_bestTimePerSize[wordCount] = elapsed;

    QStringList newAchievements = unlockNewAchievements(
        elapsed, m_hintsUsed, wordCount, stars);

    saveStats();
    updateStatusBar();

    // Поздравительный диалог
    QString starsStr;
    for (int i = 0; i < 3; ++i)
        starsStr += (i < stars) ? QString::fromUtf8("★") : QString::fromUtf8("☆");

    QString body;
    body += QString::fromUtf8("<h2 style='margin:0'>Поздравляем!</h2>");
    body += QString::fromUtf8("<p style='font-size:36px;margin:8px 0;color:#e6a700'>%1</p>")
                .arg(starsStr);
    body += QString::fromUtf8("<p>Время: <b>%1</b></p>").arg(formatTime(elapsed));
    body += QString::fromUtf8("<p>Слов: <b>%1</b></p>").arg(wordCount);
    body += QString::fromUtf8("<p>Подсказок использовано: <b>%1</b></p>")
                .arg(m_hintsUsed);
    body += QString::fromUtf8("<p>Серия: <b>%1</b> (рекорд %2)</p>")
                .arg(m_currentStreak).arg(m_bestStreak);

    if (m_bestTimePerSize[wordCount] == elapsed && m_totalSolved > 1)
        body += QString::fromUtf8(
            "<p style='color:#1a7f1a'><b>★ Новый рекорд для %1 слов!</b></p>")
            .arg(wordCount);

    if (!newAchievements.isEmpty()) {
        body += QString::fromUtf8("<hr><p><b>Новые достижения:</b></p><ul>");
        for (const QString& n : newAchievements)
            body += QString::fromUtf8("<li><b>%1</b></li>").arg(n);
        body += "</ul>";
    }

    QMessageBox box(this);
    box.setWindowTitle(QString::fromUtf8("Решено!"));
    box.setTextFormat(Qt::RichText);
    box.setText(body);
    box.setIcon(QMessageBox::NoIcon);
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}

// ===== Подсветка завершённых слов =====

void MainWindow::checkWordCompletions()
{
    if (m_cells.isEmpty()) return;

    auto words = m_gen.placedWords();

    auto isCorrect = [&](const PlacedWord& pw) {
        int dr = pw.horizontal ? 0 : 1;
        int dc = pw.horizontal ? 1 : 0;
        for (int i = 0; i < pw.word.length(); ++i) {
            int r = pw.row + dr * i;
            int c = pw.col + dc * i;
            if (r < 0 || r >= m_cells.size() || c < 0 || c >= m_cells[r].size()
                || !m_cells[r][c]) return false;
            QString cur = m_cells[r][c]->currentLetter();
            if (cur.isEmpty() || cur[0] != pw.word[i]) return false;
        }
        return true;
    };

    // 1) Слова, переставшие быть верными — снимаем зелёный.
    for (const PlacedWord& pw : words) {
        if (m_completedWordNumbers.contains(pw.number) && !isCorrect(pw)) {
            m_completedWordNumbers.remove(pw.number);
            int dr = pw.horizontal ? 0 : 1;
            int dc = pw.horizontal ? 1 : 0;
            for (int i = 0; i < pw.word.length(); ++i) {
                int r = pw.row + dr * i;
                int c = pw.col + dc * i;
                if (m_cells[r][c]) m_cells[r][c]->setHighlight(0);
            }
        }
    }

    // 2) Все правильные слова — подсвечиваем зелёным (идемпотентно).
    for (const PlacedWord& pw : words) {
        if (isCorrect(pw)) {
            m_completedWordNumbers.insert(pw.number);
            int dr = pw.horizontal ? 0 : 1;
            int dc = pw.horizontal ? 1 : 0;
            for (int i = 0; i < pw.word.length(); ++i) {
                int r = pw.row + dr * i;
                int c = pw.col + dc * i;
                if (m_cells[r][c]) m_cells[r][c]->setHighlight(1);
            }
        }
    }

    // 3) Блокировка: клетки правильных слов становятся read-only,
    //    остальные — снова доступны для ввода.
    auto enc = [](int r, int c) { return r * 1024 + c; };
    QSet<int> lockedCells;
    for (const PlacedWord& pw : words) {
        if (!m_completedWordNumbers.contains(pw.number)) continue;
        int dr = pw.horizontal ? 0 : 1;
        int dc = pw.horizontal ? 1 : 0;
        for (int i = 0; i < pw.word.length(); ++i)
            lockedCells.insert(enc(pw.row + dr * i, pw.col + dc * i));
    }
    for (int r = 0; r < m_cells.size(); ++r) {
        for (int c = 0; c < m_cells[r].size(); ++c) {
            CellWidget* cell = m_cells[r][c];
            if (!cell || cell->isBlack() || !cell->edit()) continue;
            bool lock = lockedCells.contains(enc(r, c));
            if (cell->edit()->isReadOnly() != lock)
                cell->edit()->setReadOnly(lock);
        }
    }

    // 4) Если все буквы в сетке заполнены правильно — это победа.
    if (!m_gameSolved) {
        bool allFilledCorrectly = true;
        for (int r = 0; r < m_cells.size() && allFilledCorrectly; ++r) {
            for (int c = 0; c < m_cells[r].size(); ++c) {
                CellWidget* cell = m_cells[r][c];
                if (!cell || cell->isBlack()) continue;
                QString cur = cell->currentLetter();
                if (cur.isEmpty() || cur[0] != cell->correctLetter()) {
                    allFilledCorrectly = false;
                    break;
                }
            }
        }
        if (allFilledCorrectly && !m_cells.isEmpty())
            onSolved();
    }
}

// ===== Окно достижений =====

void MainWindow::onShowAchievements()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8("Достижения и статистика"));
    dlg.resize(640, 720);

    QVBoxLayout* lay = new QVBoxLayout(&dlg);

    // Сводка
    QString summary;
    summary += QString::fromUtf8("<h2>Статистика</h2>");
    summary += QString::fromUtf8("<p>Решено кроссвордов: <b>%1</b></p>")
                  .arg(m_totalSolved);
    summary += QString::fromUtf8("<p>Текущая серия: <b>%1</b> (рекорд: <b>%2</b>)</p>")
                  .arg(m_currentStreak).arg(m_bestStreak);
    summary += QString::fromUtf8("<p>Всего времени за решением: <b>%1</b></p>")
                  .arg(formatTime(m_totalSolveTimeMs));

    if (!m_bestTimePerSize.isEmpty()) {
        summary += QString::fromUtf8("<p><b>Лучшие времена:</b></p><ul>");
        for (auto it = m_bestTimePerSize.begin(); it != m_bestTimePerSize.end(); ++it)
            summary += QString::fromUtf8("<li>%1 слов: <b>%2</b></li>")
                          .arg(it.key()).arg(formatTime(it.value()));
        summary += "</ul>";
    }

    summary += QString::fromUtf8("<h2>Достижения</h2>");
    auto all = allAchievements();
    int unlockedCount = 0;
    for (const auto& a : all) if (m_unlockedAchievements.contains(a.id)) ++unlockedCount;
    summary += QString::fromUtf8("<p>Получено: <b>%1 из %2</b></p>")
                  .arg(unlockedCount).arg(all.size());
    summary += "<ul>";
    for (const auto& a : all) {
        bool got = m_unlockedAchievements.contains(a.id);
        QString icon = got ? QString::fromUtf8("[+]") : QString::fromUtf8("[ ]");
        QString style = got ? "color:#1a7f1a" : "color:#888";
        summary += QString::fromUtf8(
            "<li style='%1'>%2 <b>%3</b> — %4</li>")
            .arg(style).arg(icon).arg(a.name).arg(a.description);
    }
    summary += "</ul>";

    QTextBrowser* view = new QTextBrowser();
    view->setHtml(summary);
    view->setStyleSheet("font-size: 15px;");
    lay->addWidget(view);

    QDialogButtonBox* btns = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    lay->addWidget(btns);

    dlg.exec();
}
