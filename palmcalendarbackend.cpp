#include "palmcalendarbackend.h"

#include "icstranscoder.h"

#include "palm/calendar/categorymappingstore.h"
#include "palm/sync/palmbackend.h"
#include "palm/sync/palmrecord.h"

#include "backendrecord.h"
#include "collectioninfo.h"
#include "shape.h"

#include <QDateTime>
#include <QStringList>

namespace WildPalms::CalendarPlugin {

namespace {

QString idForPalmRecord(std::uint32_t recordId)
{
    return WildPalms::PalmSync::PalmBackend::encodeRecordId(
        QStringLiteral("DatebookDB"), recordId);
}

bool decodeId(const QString &id, std::uint32_t *outRecordId)
{
    QString dbName;
    return WildPalms::PalmSync::PalmBackend::decodeRecordId(id, &dbName, outRecordId)
        && dbName == QLatin1String("DatebookDB");
}

} // namespace

PalmCalendarBackend::PalmCalendarBackend(
    WildPalms::PalmSync::PalmBackend *palmBackend,
    const WildPalms::PalmCalendar::CategoryMappingStore *categoryStore,
    QObject *parent)
    : Kalburator::Sync::SyncBackend(parent)
    , m_palmBackend(palmBackend)
    , m_categoryStore(categoryStore)
{
}

PalmCalendarBackend::~PalmCalendarBackend() = default;

QList<Kalburator::Shape::Shape> PalmCalendarBackend::nativeShapes() const
{
    return { { Kalburator::Shape::DomainId{QStringLiteral("calendar")},
               Kalburator::Shape::EncodingId{QStringLiteral("ical")} } };
}

QString PalmCalendarBackend::backendId()   const { return QStringLiteral("calendar"); }
QString PalmCalendarBackend::displayName() const { return QStringLiteral("Palm Calendar"); }
bool    PalmCalendarBackend::isAvailable() const
{
    return m_palmBackend != nullptr && m_palmBackend->isAvailable();
}

QList<Kalburator::Sync::CollectionInfo> PalmCalendarBackend::availableCollections()
{
    QList<Kalburator::Sync::CollectionInfo> out;

    Kalburator::Sync::CollectionInfo unfiled;
    unfiled.id   = collectionIdForSlot(0);
    unfiled.name = QStringLiteral("Unfiled");
    unfiled.type = QStringLiteral("calendar");
    out.append(unfiled);

    if (!m_categoryStore) return out;

    const QList<int> populated = m_categoryStore->populatedSlots(
        QStringLiteral("DatebookDB"));
    for (int slot : populated) {
        Kalburator::Sync::CollectionInfo info;
        info.id   = collectionIdForSlot(slot);
        info.name = m_categoryStore->slotName(
            QStringLiteral("DatebookDB"), slot);
        info.type = QStringLiteral("calendar");
        out.append(info);
    }
    return out;
}

Kalburator::Sync::CollectionInfo PalmCalendarBackend::collectionInfo(
    const QString &collectionId)
{
    for (const auto &c : availableCollections()) {
        if (c.id == collectionId) return c;
    }
    return {};
}

QString PalmCalendarBackend::createCollection(
    const Kalburator::Sync::CollectionInfo &)
{
    return {};
}

QList<Kalburator::Sync::BackendRecord> PalmCalendarBackend::loadRecords(
    const QString &collectionId)
{
    const int slot = slotFromCollectionId(collectionId);
    QList<Kalburator::Sync::BackendRecord> out;
    if (slot < 0 || !m_palmBackend) return out;

    const auto records = m_palmBackend->loadPalmRecords(QStringLiteral("DatebookDB"));
    for (const auto &pr : records) {
        if (static_cast<int>(pr.category) != slot) continue;
        QByteArray ics = encodePalmToIcs(pr);
        if (ics.isEmpty()) continue;

        Kalburator::Sync::BackendRecord br;
        br.id           = idForPalmRecord(pr.recordId);
        br.data         = ics;
        br.type         = QStringLiteral("text/calendar");
        br.lastModified = pr.lastModified;
        br.contentHash  = pr.contentHash();
        out.append(br);
    }
    return out;
}

std::optional<Kalburator::Sync::BackendRecord>
PalmCalendarBackend::loadRecord(const QString &recordId)
{
    std::uint32_t rid = 0;
    if (!decodeId(recordId, &rid) || !m_palmBackend) return std::nullopt;
    auto pr = m_palmBackend->loadPalmRecord(QStringLiteral("DatebookDB"), rid);
    if (!pr) return std::nullopt;

    QByteArray ics = encodePalmToIcs(*pr);
    if (ics.isEmpty()) return std::nullopt;

    Kalburator::Sync::BackendRecord br;
    br.id           = recordId;
    br.data         = ics;
    br.type         = QStringLiteral("text/calendar");
    br.lastModified = pr->lastModified;
    br.contentHash  = pr->contentHash();
    return br;
}

QString PalmCalendarBackend::createRecord(
    const QString &collectionId,
    const Kalburator::Sync::BackendRecord &record)
{
    const int slot = slotFromCollectionId(collectionId);
    if (slot < 0 || !m_palmBackend) return {};

    auto prOpt = decodeIcsToPalm(record.data, slot);
    if (!prOpt) return {};

    auto pr = *prOpt;
    pr.category     = static_cast<std::uint8_t>(slot);
    pr.lastModified = record.lastModified.isValid()
        ? record.lastModified
        : QDateTime::currentDateTimeUtc();

    const auto newId = m_palmBackend->createPalmRecord(
        QStringLiteral("DatebookDB"), pr);
    if (newId == 0) return {};
    return idForPalmRecord(newId);
}

bool PalmCalendarBackend::updateRecord(
    const Kalburator::Sync::BackendRecord &record)
{
    std::uint32_t rid = 0;
    if (!decodeId(record.id, &rid) || !m_palmBackend) return false;

    auto existing = m_palmBackend->loadPalmRecord(
        QStringLiteral("DatebookDB"), rid);
    if (!existing) return false;
    const int slot = static_cast<int>(existing->category);

    auto prOpt = decodeIcsToPalm(record.data, slot);
    if (!prOpt) return false;

    auto pr = *prOpt;
    pr.recordId     = rid;
    pr.category     = static_cast<std::uint8_t>(slot);
    pr.lastModified = record.lastModified.isValid()
        ? record.lastModified
        : QDateTime::currentDateTimeUtc();

    return m_palmBackend->updatePalmRecord(QStringLiteral("DatebookDB"), pr);
}

bool PalmCalendarBackend::deleteRecord(const QString &recordId)
{
    return m_palmBackend && m_palmBackend->deleteRecord(recordId);
}

QList<Kalburator::Sync::BackendRecord>
PalmCalendarBackend::modifiedSince(const QString &collectionId,
                                   const QDateTime &since)
{
    const int slot = slotFromCollectionId(collectionId);
    QList<Kalburator::Sync::BackendRecord> out;
    if (slot < 0 || !m_palmBackend) return out;

    const auto records = m_palmBackend->loadPalmRecords(QStringLiteral("DatebookDB"));
    for (const auto &pr : records) {
        if (static_cast<int>(pr.category) != slot) continue;
        if (since.isValid() && pr.lastModified <= since) continue;
        QByteArray ics = encodePalmToIcs(pr);
        if (ics.isEmpty()) continue;

        Kalburator::Sync::BackendRecord br;
        br.id           = idForPalmRecord(pr.recordId);
        br.data         = ics;
        br.type         = QStringLiteral("text/calendar");
        br.lastModified = pr.lastModified;
        br.contentHash  = pr.contentHash();
        out.append(br);
    }
    return out;
}

QStringList PalmCalendarBackend::deletedSince(const QString & /*collectionId*/,
                                              const QDateTime &since)
{
    if (!m_palmBackend) return {};
    const QString sourceCollection =
        WildPalms::PalmSync::PalmBackend::encodeCollectionId(
            QStringLiteral("DatebookDB"));
    return m_palmBackend->deletedSince(sourceCollection, since);
}

bool PalmCalendarBackend::supportsDeleteTracking() const
{
    return m_palmBackend && m_palmBackend->supportsDeleteTracking();
}

int PalmCalendarBackend::slotFromCollectionId(const QString &collectionId)
{
    static constexpr QLatin1String prefix(CollectionPrefix);
    if (!collectionId.startsWith(prefix)) return -1;
    bool ok = false;
    const int slot = collectionId.mid(prefix.size()).toInt(&ok);
    if (!ok || slot < 0 || slot > 15) return -1;
    return slot;
}

QString PalmCalendarBackend::collectionIdForSlot(int slot)
{
    return QString::fromLatin1(CollectionPrefix) + QString::number(slot);
}

} // namespace WildPalms::CalendarPlugin

#include "palmcalendarbackend.moc"
