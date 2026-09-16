#pragma once

#include <QString>
#include <QStringList>

// 在一个根目录下递归发现 git 仓库（遇到仓库即停止下钻）
class RepoScanner
{
public:
    static QStringList scan(const QString &root, int maxDepth = 3);
};
