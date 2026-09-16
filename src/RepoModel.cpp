#include "RepoModel.h"

#include "GitRunner.h"
#include "RepoScanner.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QTimer>
#include <QUrl>

#include <algorithm>

namespace {

QString normalizePath(const QString &input)
{
    QString p = input.trimmed();
    if (p.startsWith(QStringLiteral("file:")))
        p = QUrl(p).toLocalFile();
    return QDir::cleanPath(QDir::fromNativeSeparators(p));
}

QString relativeTime(const QString &gitDate)
{
    // git %ci 形如 "2026-09-15 13:22:04 +0800"，取本地近似即可
    const QDateTime dt = QDateTime::fromString(gitDate.left(19),
                                               QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    if (!dt.isValid())
        return {};
    const qint64 days = dt.daysTo(QDateTime::currentDateTime());
    if (days <= 0)
        return QStringLiteral("今天");
    if (days == 1)
        return QStringLiteral("昨天");
    if (days < 30)
        return QStringLiteral("%1 天前").arg(days);
    if (days < 365)
        return QStringLiteral("%1 个月前").arg(days / 30);
    return QStringLiteral("%1 年前").arg(days / 365);
}

QString stateToString(RepoInfo::State s)
{
    switch (s) {
    case RepoInfo::State::Idle:            return QStringLiteral("idle");
    case RepoInfo::State::Checking:        return QStringLiteral("checking");
    case RepoInfo::State::Updating:        return QStringLiteral("updating");
    case RepoInfo::State::Switching:       return QStringLiteral("switching");
    case RepoInfo::State::UpToDate:        return QStringLiteral("upToDate");
    case RepoInfo::State::UpdateAvailable: return QStringLiteral("updateAvailable");
    case RepoInfo::State::Dirty:           return QStringLiteral("dirty");
    case RepoInfo::State::Diverged:        return QStringLiteral("diverged");
    case RepoInfo::State::NoUpstream:      return QStringLiteral("noUpstream");
    case RepoInfo::State::Error:           return QStringLiteral("error");
    case RepoInfo::State::Updated:         return QStringLiteral("updated");
    }
    return QStringLiteral("idle");
}

void fillLastCommit(const QString &path, QString *subject, QString *relTime)
{
    const auto log = GitRunner::run(path, {
        QStringLiteral("-c"), QStringLiteral("i18n.logOutputEncoding=UTF-8"),
        QStringLiteral("log"), QStringLiteral("-1"),
        QStringLiteral("--pretty=format:%s%n%ci") });
    if (!log.ok)
        return;
    const QStringList lines = log.out.trimmed().split(QLatin1Char('\n'));
    if (!lines.isEmpty())
        *subject = lines.first();
    if (lines.size() > 1)
        *relTime = relativeTime(lines.at(1));
}

CheckOutcome checkRepo(const QString &path, bool fetch)
{
    CheckOutcome o;

    if (!GitRunner::run(path, { QStringLiteral("rev-parse"),
                                QStringLiteral("--is-inside-work-tree") }).ok) {
        o.error = QStringLiteral("不是有效的 git 仓库或路径不可用");
        return o;
    }

    const auto branch = GitRunner::run(path, { QStringLiteral("branch"),
                                               QStringLiteral("--show-current") });
    o.branch = branch.ok ? branch.out.trimmed() : QString();
    if (o.branch.isEmpty())
        o.branch = QStringLiteral("(detached)");

    fillLastCommit(path, &o.lastCommit, &o.lastCommitTime);

    const auto allBranches = GitRunner::run(path, {
        QStringLiteral("branch"), QStringLiteral("-a"),
        QStringLiteral("--format=%(refname:short)") });
    if (allBranches.ok) {
        const QStringList lines = allBranches.out.split(QLatin1Char('\n'));
        for (const QString &line : lines) {
            const QString b = line.trimmed();
            if (b.isEmpty() || b == QLatin1String("HEAD")
                || b.endsWith(QLatin1String("/HEAD")) || b.contains(QLatin1String("->")))
                continue;
            o.branches << b;
        }
    }

    const auto status = GitRunner::run(path, { QStringLiteral("status"),
                                               QStringLiteral("--porcelain") });
    o.dirty = status.ok && !status.out.trimmed().isEmpty();

    const auto upstream = GitRunner::run(path, {
        QStringLiteral("rev-parse"), QStringLiteral("--abbrev-ref"),
        QStringLiteral("--symbolic-full-name"), QStringLiteral("@{u}") });
    if (!upstream.ok) {
        o.state = RepoInfo::State::NoUpstream;
        o.error = QStringLiteral("当前分支没有配置上游分支");
        return o;
    }

    if (fetch) {
        const auto f = GitRunner::run(path, { QStringLiteral("fetch"),
                                              QStringLiteral("--quiet") });
        if (!f.ok) {
            o.state = RepoInfo::State::Error;
            o.error = QStringLiteral("fetch 失败：") + f.firstErrorLine();
            return o;
        }
    }

    const auto behind = GitRunner::run(path, { QStringLiteral("rev-list"),
                                               QStringLiteral("count"),
                                               QStringLiteral("HEAD..@{u}") });
    const auto ahead = GitRunner::run(path, { QStringLiteral("rev-list"),
                                              QStringLiteral("count"),
                                              QStringLiteral("@{u}..HEAD") });
    o.behind = behind.ok ? behind.out.trimmed().toInt() : 0;
    o.ahead = ahead.ok ? ahead.out.trimmed().toInt() : 0;

    if (o.dirty)
        o.state = RepoInfo::State::Dirty;
    else if (o.ahead > 0)
        o.state = RepoInfo::State::Diverged;
    else if (o.behind > 0)
        o.state = RepoInfo::State::UpdateAvailable;
    else
        o.state = RepoInfo::State::UpToDate;
    return o;
}

CheckOutcome runSwitch(const QString &path, const QString &branch)
{
    CheckOutcome o;

    const auto status = GitRunner::run(path, { QStringLiteral("status"),
                                               QStringLiteral("--porcelain") });
    if (status.ok && !status.out.trimmed().isEmpty()) {
        o.state = RepoInfo::State::Dirty;
        o.error = QStringLiteral("本地有改动，未切换分支（处理改动后重试）");
        return o;
    }

    const bool isRemoteRef = branch.contains(QLatin1Char('/'));
    const QStringList args = isRemoteRef
        ? QStringList{ QStringLiteral("checkout"), QStringLiteral("-t"), branch }
        : QStringList{ QStringLiteral("checkout"), branch };
    const auto co = GitRunner::run(path, args);
    if (!co.ok) {
        o.state = RepoInfo::State::Error;
        o.error = QStringLiteral("切换失败：") + co.firstErrorLine();
        return o;
    }

    // 切换成功后做一次本地比较（不重新 fetch）
    return checkRepo(path, false);
}

} // namespace

RepoModel::RepoModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_pool.setMaxThreadCount(qBound(4, QThread::idealThreadCount(), 8));
    load();

    if (!m_repos.isEmpty()) {
        setSummary(QStringLiteral("检测中…"));
        QTimer::singleShot(0, this, [this] { refreshAll(); });
    } else {
        setSummary(QStringLiteral("从「扫描文件夹」或「添加仓库」开始"));
    }
}

RepoModel::~RepoModel()
{
    m_pool.clear();
    m_pool.waitForDone(); // 仓库任务持有 this，退出前必须等它们跑完
}

int RepoModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_repos.size());
}

QVariant RepoModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row < 0 || row >= m_repos.size())
        return {};
    const RepoInfo &r = m_repos.at(row);

    switch (role) {
    case PathRole:           return r.path;
    case NameRole:           return r.name;
    case FolderRole:         return r.folder;
    case BranchRole:         return r.branch;
    case PinnedBranchRole:   return r.pinnedBranch;
    case BranchesRole:       return r.branches;
    case StateRole:          return stateToString(r.state);
    case BehindRole:         return r.behind;
    case AheadRole:          return r.ahead;
    case LastCommitRole:     return r.lastCommit;
    case LastCommitTimeRole: return r.lastCommitTime;
    case ErrorRole:          return r.error;
    case CheckedRole:        return r.checked;
    }
    return {};
}

bool RepoModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    const int row = index.row();
    if (row < 0 || row >= m_repos.size() || role != CheckedRole)
        return false;
    m_repos[row].checked = value.toBool();
    emitRowChanged(row);
    emit checkedCountChanged();
    return true;
}

QHash<int, QByteArray> RepoModel::roleNames() const
{
    return {
        { PathRole,           "path" },
        { NameRole,           "name" },
        { FolderRole,         "folder" },
        { BranchRole,         "branch" },
        { PinnedBranchRole,   "pinnedBranch" },
        { BranchesRole,       "branches" },
        { StateRole,          "state" },
        { BehindRole,         "behind" },
        { AheadRole,          "ahead" },
        { LastCommitRole,     "lastCommit" },
        { LastCommitTimeRole, "lastCommitTime" },
        { ErrorRole,          "error" },
        { CheckedRole,        "checked" }
    };
}

int RepoModel::checkedCount() const
{
    int n = 0;
    for (const RepoInfo &r : m_repos)
        if (r.checked)
            ++n;
    return n;
}

int RepoModel::updatableCount() const
{
    int n = 0;
    for (const RepoInfo &r : m_repos)
        if (r.state == RepoInfo::State::UpdateAvailable)
            ++n;
    return n;
}

void RepoModel::addRepo(const QString &path)
{
    const QString p = normalizePath(path);
    if (!QFileInfo::exists(p + QLatin1String("/.git"))) {
        setSummary(QStringLiteral("添加失败：不是 git 仓库（%1）").arg(p));
        return;
    }
    if (indexOfPath(p) >= 0) {
        setSummary(QStringLiteral("已在列表中：%1").arg(p));
        return;
    }

    RepoInfo r;
    r.path = p;
    r.name = QFileInfo(p).fileName();
    beginInsertRows(QModelIndex(), m_repos.size(), m_repos.size());
    m_repos.append(r);
    endInsertRows();
    save();
    emitCounts();

    m_batch = Batch::Check;
    scheduleCheck(m_repos.size() - 1);
}

void RepoModel::importPaths(const QStringList &paths, const QString &folder)
{
    const QString folderNorm = folder.isEmpty() ? QString() : normalizePath(folder);
    int added = 0;
    for (const QString &raw : paths) {
        const QString p = normalizePath(raw);
        if (!QFileInfo::exists(p + QLatin1String("/.git")) || indexOfPath(p) >= 0)
            continue;
        RepoInfo r;
        r.path = p;
        r.name = QFileInfo(p).fileName();
        r.folder = folderNorm;
        beginInsertRows(QModelIndex(), m_repos.size(), m_repos.size());
        m_repos.append(r);
        endInsertRows();
        scheduleCheck(m_repos.size() - 1);
        ++added;
    }
    if (added > 0) {
        if (!folderNorm.isEmpty() && !m_folders.contains(folderNorm)) {
            m_folders << folderNorm;
            emit foldersChanged();
        }
        save();
        emitCounts();
        m_batch = Batch::Check;
        setSummary(QStringLiteral("已导入 %1 个仓库，检测中…").arg(added));
    }
}

void RepoModel::removeAt(int index)
{
    if (index < 0 || index >= m_repos.size())
        return;
    beginRemoveRows(QModelIndex(), index, index);
    m_repos.removeAt(index);
    endRemoveRows();
    save();
    emitCounts();
}

void RepoModel::removeFolder(const QString &folder)
{
    const QString f = normalizePath(folder);
    if (!m_folders.removeOne(f))
        return;
    beginResetModel();
    m_repos.erase(std::remove_if(m_repos.begin(), m_repos.end(),
                                 [f](const RepoInfo &r) { return r.folder == f; }),
                  m_repos.end());
    endResetModel();
    save();
    emit foldersChanged();
    emitCounts();
    setSummary(QStringLiteral("已移除文件夹 %1（及其下 %2 个登记仓库中的关联项）")
                   .arg(f).arg(0));
}

void RepoModel::scanFolder(const QString &root)
{
    if (busy())
        return;
    const QString r = normalizePath(root);
    setSummary(QStringLiteral("扫描中…"));
    beginJob();
    m_pool.start([this, r] {
        const QStringList found = RepoScanner::scan(r);
        QMetaObject::invokeMethod(this, [this, found] {
            QStringList fresh;
            for (const QString &p : found) {
                if (indexOfPath(p) < 0)
                    fresh << p;
            }
            endJob();
            emit scanFinished(fresh);
        }, Qt::QueuedConnection);
    });
}

void RepoModel::refreshAll()
{
    if (busy() || m_repos.isEmpty())
        return;
    m_batch = Batch::Check;
    setSummary(QStringLiteral("检测中…"));
    for (int i = 0; i < m_repos.size(); ++i)
        scheduleCheck(i);
}

void RepoModel::updateChecked()
{
    if (busy())
        return;

    QList<int> targets;
    for (int i = 0; i < m_repos.size(); ++i) {
        if (m_repos[i].checked && m_repos[i].state == RepoInfo::State::UpdateAvailable)
            targets << i;
    }
    if (targets.isEmpty()) {
        setSummary(QStringLiteral("选中的仓库中没有「可更新」状态的（仅干净且落后的仓库会执行 git pull --ff-only）"));
        return;
    }

    m_batch = Batch::Update;
    m_updateOk = m_updateSkip = m_updateFail = 0;
    setSummary(QStringLiteral("更新中…"));
    for (int row : targets) {
        m_repos[row].state = RepoInfo::State::Updating;
        m_repos[row].error.clear();
        emitRowChanged(row);
        beginJob();
        const RepoInfo info = m_repos.at(row);
        m_pool.start([this, info] {
            // 复用 check 的结构做 pull 结果回填
            const CheckOutcome r = [&info]() {
                const auto status = GitRunner::run(info.path, { QStringLiteral("status"),
                                                                QStringLiteral("--porcelain") });
                if (status.ok && !status.out.trimmed().isEmpty()) {
                    CheckOutcome skip;
                    skip.state = RepoInfo::State::Dirty;
                    skip.error = QStringLiteral("工作区有本地改动，已跳过更新");
                    return skip;
                }
                const auto pull = GitRunner::run(info.path, { QStringLiteral("pull"),
                                                              QStringLiteral("--ff-only") });
                if (!pull.ok) {
                    CheckOutcome fail;
                    fail.state = RepoInfo::State::Error;
                    fail.error = QStringLiteral("更新失败：") + pull.firstErrorLine();
                    return fail;
                }
                CheckOutcome done = checkRepo(info.path, false);
                done.state = RepoInfo::State::Updated;
                return done;
            }();
            QMetaObject::invokeMethod(this, [this, info, r] {
                applyUpdateResult(info.path, r);
            }, Qt::QueuedConnection);
        });
    }
}

void RepoModel::setAllChecked(bool checked)
{
    for (int i = 0; i < m_repos.size(); ++i) {
        if (m_repos[i].checked != checked) {
            m_repos[i].checked = checked;
            emitRowChanged(i);
        }
    }
    emit checkedCountChanged();
}

void RepoModel::switchBranch(const QString &path, const QString &branch)
{
    const int row = indexOfPath(normalizePath(path));
    if (row < 0 || busy())
        return;
    const RepoInfo::State s = m_repos.at(row).state;
    if (s == RepoInfo::State::Updating || s == RepoInfo::State::Switching)
        return;

    m_repos[row].state = RepoInfo::State::Switching;
    m_repos[row].error.clear();
    emitRowChanged(row);
    setSummary(QStringLiteral("切换分支中…"));
    beginJob();

    const RepoInfo info = m_repos.at(row);
    m_pool.start([this, info, branch] {
        const CheckOutcome r = runSwitch(info.path, branch);
        QMetaObject::invokeMethod(this, [this, info, r] {
            applyCheckResult(info.path, r, true);
        }, Qt::QueuedConnection);
    });
}

void RepoModel::pinCurrentBranch(const QString &path)
{
    const int row = indexOfPath(normalizePath(path));
    if (row < 0)
        return;
    const QString b = m_repos.at(row).branch;
    if (b.isEmpty() || b == QStringLiteral("(detached)"))
        return;
    m_repos[row].pinnedBranch = b;
    save();
    emitRowChanged(row);
    setSummary(QStringLiteral("%1 已设跟随分支：%2").arg(m_repos.at(row).name, b));
}

void RepoModel::unpinBranch(const QString &path)
{
    const int row = indexOfPath(normalizePath(path));
    if (row < 0)
        return;
    m_repos[row].pinnedBranch.clear();
    save();
    emitRowChanged(row);
    setSummary(QStringLiteral("%1 已取消跟随分支").arg(m_repos.at(row).name));
}

void RepoModel::load()
{
    QSettings s;
    m_folders.clear();
    const int fn = s.beginReadArray(QStringLiteral("folders"));
    for (int i = 0; i < fn; ++i) {
        s.setArrayIndex(i);
        m_folders << s.value(QStringLiteral("path")).toString();
    }
    s.endArray();

    m_repos.clear();
    const int n = s.beginReadArray(QStringLiteral("repos"));
    for (int i = 0; i < n; ++i) {
        s.setArrayIndex(i);
        RepoInfo r;
        r.path = s.value(QStringLiteral("path")).toString();
        r.name = QFileInfo(r.path).fileName();
        r.folder = s.value(QStringLiteral("folder")).toString();
        r.pinnedBranch = s.value(QStringLiteral("pinned")).toString();
        m_repos.append(r);
    }
    s.endArray();
}

void RepoModel::save() const
{
    QSettings s;
    s.remove(QStringLiteral("folders"));
    s.beginWriteArray(QStringLiteral("folders"));
    for (int i = 0; i < m_folders.size(); ++i) {
        s.setArrayIndex(i);
        s.setValue(QStringLiteral("path"), m_folders.at(i));
    }
    s.endArray();

    s.remove(QStringLiteral("repos"));
    s.beginWriteArray(QStringLiteral("repos"));
    for (int i = 0; i < m_repos.size(); ++i) {
        s.setArrayIndex(i);
        const RepoInfo &r = m_repos.at(i);
        s.setValue(QStringLiteral("path"), r.path);
        s.setValue(QStringLiteral("folder"), r.folder);
        s.setValue(QStringLiteral("pinned"), r.pinnedBranch);
    }
    s.endArray();
}

int RepoModel::indexOfPath(const QString &path) const
{
    for (int i = 0; i < m_repos.size(); ++i) {
        if (m_repos.at(i).path == path)
            return i;
    }
    return -1;
}

void RepoModel::scheduleCheck(int row)
{
    if (row < 0 || row >= m_repos.size())
        return;
    m_repos[row].state = RepoInfo::State::Checking;
    m_repos[row].error.clear();
    emitRowChanged(row);
    beginJob();

    const RepoInfo info = m_repos.at(row);
    m_pool.start([this, info] {
        const CheckOutcome r = checkRepo(info.path, true);
        QMetaObject::invokeMethod(this, [this, info, r] {
            applyCheckResult(info.path, r, false);
        }, Qt::QueuedConnection);
    });
}

void RepoModel::applyCheckResult(const QString &path, const CheckOutcome &r, bool fromSwitch)
{
    const int row = indexOfPath(path);
    endJob();
    if (row < 0)
        return;
    const RepoInfo::State cur = m_repos.at(row).state;
    if (!fromSwitch && (cur == RepoInfo::State::Updating || cur == RepoInfo::State::Switching))
        return;

    RepoInfo &repo = m_repos[row];
    repo.branch = r.branch;
    repo.branches = r.branches;
    repo.lastCommit = r.lastCommit;
    repo.lastCommitTime = r.lastCommitTime;
    repo.error = r.error;
    repo.behind = r.behind;
    repo.ahead = r.ahead;
    repo.dirty = r.dirty;
    repo.state = r.state;
    emitRowChanged(row);
    emitCounts();

    if (fromSwitch) {
        if (r.state == RepoInfo::State::Error || r.state == RepoInfo::State::Dirty)
            setSummary(QStringLiteral("%1：%2").arg(repo.name, r.error));
        else
            setSummary(QStringLiteral("%1 已切换到 %2").arg(repo.name, r.branch));
    }
}

void RepoModel::applyUpdateResult(const QString &path, const CheckOutcome &r)
{
    const int row = indexOfPath(path);
    endJob();
    if (row < 0)
        return;

    RepoInfo &repo = m_repos[row];
    repo.state = r.state;
    repo.error = r.error;
    if (!r.lastCommit.isEmpty()) {
        repo.lastCommit = r.lastCommit;
        repo.lastCommitTime = r.lastCommitTime;
    }
    if (r.state == RepoInfo::State::Updated) {
        repo.behind = 0;
        repo.ahead = 0;
        repo.dirty = false;
        repo.branch = r.branch;
        ++m_updateOk;
    } else if (r.state == RepoInfo::State::Dirty) {
        repo.dirty = true;
        ++m_updateSkip;
    } else {
        ++m_updateFail;
    }
    emitRowChanged(row);
    emitCounts();
}

void RepoModel::beginJob()
{
    if (++m_activeJobs == 1)
        emit busyChanged();
}

void RepoModel::endJob()
{
    if (m_activeJobs == 0)
        return;
    if (--m_activeJobs == 0) {
        emit busyChanged();
        if (m_batch == Batch::Check) {
            setSummary(QStringLiteral("检测完成 · 可更新 %1 / 共 %2")
                           .arg(updatableCount()).arg(totalCount()));
        } else if (m_batch == Batch::Update) {
            QString text = QStringLiteral("更新完成 · 成功 %1").arg(m_updateOk);
            if (m_updateSkip > 0)
                text += QStringLiteral(" · 跳过 %1").arg(m_updateSkip);
            if (m_updateFail > 0)
                text += QStringLiteral(" · 失败 %1").arg(m_updateFail);
            setSummary(text);
        }
        m_batch = Batch::None;
    }
}

void RepoModel::setSummary(const QString &text)
{
    if (m_summary == text)
        return;
    m_summary = text;
    emit summaryChanged();
}

void RepoModel::emitRowChanged(int row)
{
    emit dataChanged(index(row), index(row));
}

void RepoModel::emitCounts()
{
    emit totalCountChanged();
    emit checkedCountChanged();
    emit updatableCountChanged();
}
