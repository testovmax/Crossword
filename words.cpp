#include "words.h"

#include <QFile>
#include <QTextStream>
#include <QStringList>

QVector<WordEntry> loadDictionaryFromFile(const QString& path,
                                          QString* errorMessage)
{
    QVector<WordEntry> result;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = QString::fromUtf8("Не удалось открыть файл словаря: ")
                            + path;
        return result;
    }

    QTextStream in(&f);
    // Qt6 использует UTF-8 по умолчанию, для Qt5 укажем явно:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec("UTF-8");
#endif

    int lineNum = 0;
    while (!in.atEnd()) {
        ++lineNum;
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;

        int sep = line.indexOf(QLatin1Char('|'));
        if (sep < 0) {
            if (errorMessage)
                *errorMessage = QString::fromUtf8(
                    "Строка %1: ожидался разделитель '|' (СЛОВО|Вопрос)")
                    .arg(lineNum);
            return {};
        }

        QString word = line.left(sep).trimmed().toUpper();
        QString clue = line.mid(sep + 1).trimmed();

        if (word.isEmpty() || clue.isEmpty()) {
            if (errorMessage)
                *errorMessage = QString::fromUtf8(
                    "Строка %1: пустое слово или вопрос").arg(lineNum);
            return {};
        }

   
        if (word.length() < 2) {
            if (errorMessage)
                *errorMessage = QString::fromUtf8(
                    "Строка %1: слово '%2' слишком короткое")
                    .arg(lineNum).arg(word);
            return {};
        }

        result.append({ word, clue });
    }

    if (result.isEmpty() && errorMessage)
        *errorMessage = QString::fromUtf8("Файл словаря пустой");

    return result;
}
