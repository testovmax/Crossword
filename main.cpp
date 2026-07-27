#include <QApplication>
#include <QFont>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    // Высокое DPI — нужно ВЫЗВАТЬ ДО создания QApplication, иначе ячейки
    // будут рендериться с дробным масштабом и казаться "кривыми".
    // (В Qt 6 эти атрибуты deprecated, но безвредны.)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps,  true);
#endif

    QApplication app(argc, argv);
    QApplication::setApplicationName(QString::fromUtf8("Кроссворд"));

    // Делаем шрифт интерфейса заметно крупнее по умолчанию
    QFont f = app.font();
    f.setPointSize(14);
    app.setFont(f);

    MainWindow w;
    w.showMaximized();
    return app.exec();
}
