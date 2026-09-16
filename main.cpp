#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QIODevice>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QSettings>
#include <QStandardPaths>

#include "AppSettings.h"

namespace {

void appendLog(const QString &line)
{
    static QFile file([] {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        return dir + QStringLiteral("/log.txt");
    }());
    if (!file.isOpen())
        file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    if (file.isOpen()) {
        file.write((QDateTime::currentDateTime().toString(QStringLiteral("MM-dd HH:mm:ss.zzz "))
                    + line + QLatin1Char('\n')).toUtf8());
        file.flush();
    }
}

void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    QString level = QStringLiteral("INFO ");
    switch (type) {
    case QtDebugMsg:    level = QStringLiteral("DEBUG"); break;
    case QtWarningMsg:  level = QStringLiteral("WARN "); break;
    case QtCriticalMsg: level = QStringLiteral("CRIT "); break;
    case QtFatalMsg:    level = QStringLiteral("FATAL"); break;
    default: break;
    }
    QString where;
    if (ctx.file)
        where = QStringLiteral("%1:%2 ").arg(QFileInfo(ctx.file).fileName()).arg(ctx.line);
    appendLog(level + where + msg);
    if (type == QtFatalMsg)
        abort();
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("git-shepherd"));
    QGuiApplication::setApplicationName(QStringLiteral("git-shepherd"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    qInstallMessageHandler(messageHandler);
    appendLog(QStringLiteral("==== start pid=%1 ====").arg(QCoreApplication::applicationPid()));

    AppSettings appSettings;
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("AppSettings"), &appSettings);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [](QObject *obj, const QUrl &url) {
        appendLog(obj ? QStringLiteral("QML loaded: ") + url.toString()
                      : QStringLiteral("QML FAILED: ") + url.toString());
        if (!obj)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.loadFromModule("GitShepherd", "Main");

    const int rc = app.exec();
    appendLog(QStringLiteral("==== exit %1 ====").arg(rc));
    return rc;
}
