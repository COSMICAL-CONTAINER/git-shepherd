#pragma once

#include <QSortFilterProxyModel>
#include <QtQmlIntegration/qqmlintegration.h>

// 按 tab 过滤仓库：all / folder（指定文件夹及其路径前缀下的仓库）/ standalone（单独添加）
class RepoFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(QString folderPath READ folderPath WRITE setFolderPath NOTIFY folderPathChanged)

public:
    explicit RepoFilterProxy(QObject *parent = nullptr);

    QString mode() const { return m_mode; }
    void setMode(const QString &mode);
    QString folderPath() const { return m_folderPath; }
    void setFolderPath(const QString &path);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

signals:
    void modeChanged();
    void folderPathChanged();

private:
    QString m_mode = QStringLiteral("all");
    QString m_folderPath;
};
