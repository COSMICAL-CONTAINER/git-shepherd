#pragma once

#include <QString>
#include <QStringList>

struct GitResult
{
    bool ok = false;
    QString out;
    QString err;

    QString firstErrorLine() const;
};

// 在调用线程内同步执行一条 git 命令，带超时保护。
class GitRunner
{
public:
    static GitResult run(const QString &workDir, const QStringList &args, int timeoutMs = 90000);
};
