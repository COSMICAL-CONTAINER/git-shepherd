#pragma once

#include <QAbstractListModel>
#include <QThreadPool>
#include <QtQmlIntegration/qqmlintegration.h>

struct RepoInfo
{
    enum class State {
        Idle,           // 刚登记，尚未检测
        Checking,       // 正在 fetch / 比较
        Updating,       // 正在 pull
        Switching,      // 正在切换分支
        UpToDate,       // 与上游一致
        UpdateAvailable,// 干净且落后，可安全 pull
        Dirty,          // 工作区有本地改动
        Diverged,       // 领先上游（有未推送提交），不能快进
        NoUpstream,     // 当前分支没有上游
        Error,          // 检测或更新失败
        Updated         // 本次会话内更新成功
    };

    QString path;
    QString name;
    QString folder;        // 登记来源文件夹；空 = 单独添加的仓库
    QString branch;        // 当前分支（detached 时为 🏷 标签名或 "(detached)"）
    QString pinnedBranch;  // 用户设定的“跟随分支”
    QStringList branches;  // 本地 + 远端分支（检测时刷新）
    QStringList tags;      // 最近的标签，新在前（检测时刷新，最多 20 个）
    QString lastCommit;
    QString lastCommitTime; // 相对时间，如“3 天前”
    QString error;
    int behind = 0;
    int ahead = 0;
    bool dirty = false;
    bool checked = false;   // 会话内的勾选状态，不持久化
    State state = State::Idle;
};

struct CheckOutcome
{
    RepoInfo::State state = RepoInfo::State::Error;
    QString branch;
    QString error;
    QString lastCommit;
    QString lastCommitTime;
    QStringList branches;
    QStringList tags;
    int behind = 0;
    int ahead = 0;
    bool dirty = false;
};

class RepoModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QStringList folders READ folders NOTIFY foldersChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged)
    Q_PROPERTY(int checkedCount READ checkedCount NOTIFY checkedCountChanged)
    Q_PROPERTY(int updatableCount READ updatableCount NOTIFY updatableCountChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY summaryChanged)

public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        NameRole,
        FolderRole,
        BranchRole,
        PinnedBranchRole,
        BranchesRole,
        TagsRole,
        StateRole,
        BehindRole,
        AheadRole,
        LastCommitRole,
        LastCommitTimeRole,
        ErrorRole,
        CheckedRole
    };
    Q_ENUM(Roles)

    explicit RepoModel(QObject *parent = nullptr);
    ~RepoModel() override;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    QStringList folders() const { return m_folders; }
    int totalCount() const { return static_cast<int>(m_repos.size()); }
    int checkedCount() const;
    int updatableCount() const;
    bool busy() const { return m_activeJobs > 0; }
    QString summary() const { return m_summary; }

    Q_INVOKABLE void addRepo(const QString &path);
    Q_INVOKABLE void importPaths(const QStringList &paths, const QString &folder);
    Q_INVOKABLE void removeAt(int index);
    Q_INVOKABLE void scanFolder(const QString &root);
    Q_INVOKABLE void removeFolder(const QString &folder);
    Q_INVOKABLE void refreshAll();
    Q_INVOKABLE void updateChecked();
    Q_INVOKABLE void setAllChecked(bool checked);
    Q_INVOKABLE void switchBranch(const QString &path, const QString &ref, const QString &kind); // kind: ""=本地分支 "remote"=远端 "tag"=标签
    Q_INVOKABLE void pinCurrentBranch(const QString &path);
    Q_INVOKABLE void unpinBranch(const QString &path);

signals:
    void foldersChanged();
    void totalCountChanged();
    void checkedCountChanged();
    void updatableCountChanged();
    void busyChanged();
    void summaryChanged();
    void scanFinished(const QStringList &foundPaths);

private:
    enum class Batch { None, Check, Update };

    void load();
    void save() const;
    int indexOfPath(const QString &path) const;
    void scheduleCheck(int row);
    void applyCheckResult(const QString &path, const CheckOutcome &r, bool fromSwitch);
    void applyUpdateResult(const QString &path, const CheckOutcome &r);
    void beginJob();
    void endJob();
    void setSummary(const QString &text);
    void emitRowChanged(int row);
    void emitCounts();

    QVector<RepoInfo> m_repos;
    QStringList m_folders;
    QThreadPool m_pool;
    int m_activeJobs = 0;
    Batch m_batch = Batch::None;
    int m_updateOk = 0;
    int m_updateSkip = 0;
    int m_updateFail = 0;
    QString m_summary;
};
