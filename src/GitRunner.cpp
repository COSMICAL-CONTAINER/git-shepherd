#include "GitRunner.h"

#include <QElapsedTimer>
#include <QProcess>
#include <QProcessEnvironment>

QString GitResult::firstErrorLine() const
{
    const QStringList lines = err.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const QString t = line.trimmed();
        if (!t.isEmpty())
            return t;
    }
    return out.trimmed();
}

GitResult GitRunner::run(const QString &workDir, const QStringList &args, int timeoutMs)
{
    GitResult r;
    QProcess p;
    p.setWorkingDirectory(workDir);
    // 禁止 git 弹出交互式凭据提示，让无法鉴权的远端快速失败而不是挂死
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("GIT_TERMINAL_PROMPT"), QStringLiteral("0"));
    p.setProcessEnvironment(env);

    p.start(QStringLiteral("git"), args);
    if (!p.waitForStarted(5000)) {
        r.err = QStringLiteral("无法启动 git（请确认已安装并在 PATH 中）");
        return r;
    }

    QElapsedTimer timer;
    timer.start();
    while (!p.waitForFinished(1000)) {
        if (p.state() == QProcess::NotRunning)
            break;
        if (timer.elapsed() > timeoutMs) {
            p.kill();
            p.waitForFinished(3000);
            r.err = QStringLiteral("命令超时（%1 秒）：%2")
                        .arg(timeoutMs / 1000)
                        .arg(args.join(QLatin1Char(' ')));
            // 写命令被硬终止可能残留 .git/index.lock，必须向用户披露恢复方法
            if (args.contains(QLatin1String("checkout"))
                || args.contains(QLatin1String("pull"))) {
                r.err += QStringLiteral(
                    "\n命令已被强制终止；若该仓库后续 git 操作报 index.lock 错误，"
                    "请手动删除 .git/index.lock 后重试");
            }
            return r;
        }
    }

    r.out = QString::fromUtf8(p.readAllStandardOutput());
    r.err = QString::fromUtf8(p.readAllStandardError());
    r.ok = p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
    return r;
}
