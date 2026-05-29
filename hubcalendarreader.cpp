#include "hubcalendarreader.h"

#include <syncbackend.h>
#include <backendrecord.h>

namespace {

// Strip the GenericSqliteBackend per-collection prefix from a record id
// if present. The sink encodes record ids as `<collectionId>\x01<origId>`;
// callers want the bare `<origId>` form back. If no SOH separator is
// found the input id is returned unchanged.
QString stripCollectionPrefix(const QString &recordId,
                              const QString &collectionId)
{
    const QString prefix = collectionId + QLatin1Char('\x01');
    if (recordId.startsWith(prefix))
        return recordId.mid(prefix.size());
    return recordId;
}

} // namespace

namespace WildPalms::CalendarPlugin {

HubCalendarReader::HubCalendarReader(Kalburator::Sync::SyncBackend *hub,
                                     QString collectionId)
    : m_hub(hub)
    , m_collectionId(std::move(collectionId))
{
    Q_ASSERT(m_hub);
}

QStringList HubCalendarReader::listRecordIds() const
{
    if (!m_hub) return {};
    QStringList ids;
    const auto records = m_hub->loadRecords(m_collectionId);
    ids.reserve(records.size());
    for (const auto &r : records) {
        if (!r.isDeleted)
            ids.append(stripCollectionPrefix(r.id, m_collectionId));
    }
    return ids;
}

QByteArray HubCalendarReader::recordBytes(const QString &id) const
{
    if (!m_hub) return {};
    const auto records = m_hub->loadRecords(m_collectionId);
    for (const auto &r : records) {
        if (r.isDeleted) continue;
        if (stripCollectionPrefix(r.id, m_collectionId) == id)
            return r.data;
    }
    return {};
}

} // namespace WildPalms::CalendarPlugin
