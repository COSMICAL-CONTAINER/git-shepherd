#include "RepoFilterProxy.h"

#include "RepoModel.h"

RepoFilterProxy::RepoFilterProxy(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void RepoFilterProxy::setMode(const QString &mode)
{
    if (m_mode == mode)
        return;
    m_mode = mode;
    invalidateFilter();
    emit modeChanged();
}

void RepoFilterProxy::setFolderPath(const QString &path)
{
    if (m_folderPath == path)
        return;
    m_folderPath = path;
    invalidateFilter();
    emit folderPathChanged();
}

bool RepoFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    const QString folder = idx.data(RepoModel::FolderRole).toString();
    const QString path = idx.data(RepoModel::PathRole).toString();

    if (m_mode == QLatin1String("folder")) {
        if (folder == m_folderPath)
            return true;
        // 单独添加但物理上位于该文件夹下的仓库也归入此 tab
        return folder.isEmpty() && !m_folderPath.isEmpty()
               && path.startsWith(m_folderPath + QLatin1Char('/'));
    }
    if (m_mode == QLatin1String("standalone"))
        return folder.isEmpty();
    return true;
}
