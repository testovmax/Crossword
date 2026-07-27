#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QSet>
#include <QMap>
#include <QDateTime>
#include "generator.h"

class CellWidget;
class QGridLayout;
class QListWidget;
class QPushButton;
class QSpinBox;
class QLabel;
class QWidget;
class QScrollArea;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onGenerate();
    void onWordCheck();
    void onHint();
    void onClear();
    void onLoadDictionary();
    void onZoomIn();
    void onZoomOut();
    void onShowAchievements();
    void onTimerTick();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void promptForCount();
    void rebuildGrid();
    void rebuildClueLists();
    bool ensureDictionaryLoaded();
    bool tryLoadDictionary(const QString& path, QString* errorMessage = nullptr);

    // Авто-переход курсора
    void advanceFocus(int r, int c);
    void setDirectionForCell(int r, int c);
    bool findCellOfEdit(QObject* obj, int& outR, int& outC) const;

    // Масштабирование
    QVector<QVector<QString>> collectLetters() const;
    void restoreLetters(const QVector<QVector<QString>>& letters);

    // --- Геймификация ---
    void loadStats();
    void saveStats();
    void resetGameState();
    void startGameTimer();
    void stopGameTimer();
    QString formatTime(qint64 ms) const;
    void updateHintButtonText();
    void updateStatusBar();

    // Полная победа: подсчёт звёзд, обновление статистики, диалог
    void onSolved();
    int  computeStars(qint64 elapsedMs, int hintsUsed, int wordCount) const;
    QStringList unlockNewAchievements(qint64 elapsedMs, int hintsUsed,
                                      int wordCount, int stars);

    // Проверка завершённых слов по ходу решения — подсветка зелёным
    void checkWordCompletions();

    // ----- состояние игры -----
    bool    m_directionHorizontal = true;
    int     m_lastFocusedR = -1;
    int     m_lastFocusedC = -1;
    bool    m_gameActive   = false;
    bool    m_gameSolved   = false;
    qint64  m_gameStartMs  = 0;
    qint64  m_gameElapsedMs = 0;
    int     m_hintsUsed    = 0;
    int     m_hintsLimit   = 3;
    QSet<int> m_completedWordNumbers;

    // ----- персистентная статистика -----
    int     m_totalSolved      = 0;
    int     m_currentStreak    = 0;
    int     m_bestStreak       = 0;
    qint64  m_totalSolveTimeMs = 0;
    QMap<int, qint64> m_bestTimePerSize;     // wordCount -> best time ms
    QSet<QString>     m_unlockedAchievements;

    // ----- объекты -----
    CrosswordGenerator m_gen;
    QVector<WordEntry> m_dictionary;
    QString m_dictionaryPath;
    QVector<QVector<CellWidget*>> m_cells;

    QSpinBox*    m_countSpin    = nullptr;
    QPushButton* m_generateBtn  = nullptr;
    QPushButton* m_hintBtn      = nullptr;
    QPushButton* m_clearBtn     = nullptr;
    QPushButton* m_loadDictBtn  = nullptr;
    QPushButton* m_zoomInBtn    = nullptr;
    QPushButton* m_zoomOutBtn   = nullptr;
    QPushButton* m_achievementsBtn = nullptr;
    QLabel*      m_statusLabel  = nullptr;
    QLabel*      m_timeLabel    = nullptr;
    QLabel*      m_streakLabel  = nullptr;

    QTimer*      m_uiTimer      = nullptr;

    int          m_cellSize     = 84;

    QWidget*     m_gridHost     = nullptr;
    QGridLayout* m_gridLayout   = nullptr;
    QScrollArea* m_gridScroll   = nullptr;

    QListWidget* m_acrossList   = nullptr;
    QListWidget* m_downList     = nullptr;
};

#endif // MAINWINDOW_H
