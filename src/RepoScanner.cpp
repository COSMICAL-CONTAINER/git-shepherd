#include "RepoScanner.h"

#include <QDir>
#include <QFileInfo>

#include <queue>
#include <utility>

QStringList RepoScanner::scan(const QString &root, int maxDepth)
{
    QStringList found;
    std::queue<std::pair<QString, int>> todo;
    todo.push({ QDir::cleanPath(root), 0 });

    static const QSet<QString> skipDirs = {
        QStringLiteral("node_modules"), QStringLiteral("target"),
        QStringLiteral("build"), QStringLiteral("vendor"),
        QStringLiteral("dist"), QStringLiteral("__pycache__"),
        QStringLiteral("venv"), QStringLiteral(".venv")
    };

    while (!todo.empty()) {
        const auto [dir, depth] = todo.front();
        todo.pop();

        if (QFileInfo::exists(dir + QLatin1String("/.git"))) {
            found << dir;
            continue; // 仓库内部不再下钻
        }
        if (depth >= maxDepth)
            continue;

        const QStringList subs = QDir(dir).entryList(
            QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
        for (const QString &sub : subs) {
            if (sub.startsWith(QLatin1Char('.')) || skipDirs.contains(sub))
                continue;
            todo.push({ dir + QLatin1Char('/') + sub, depth + 1 });
        }
    }

    found.sort();
    return found;
}
