#ifndef WILDPALMS_CALENDAR_HUBCALENDARREADER_H
#define WILDPALMS_CALENDAR_HUBCALENDARREADER_H

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace Kalburator::Sync { class SyncBackend; }

namespace WildPalms::CalendarPlugin {

/**
 * @brief Thin per-domain facade over the canonical hub
 *        ("wp-hub" GenericSqliteBackend, collection "palm:calendar").
 *
 * Read-only. The view (CalendarView) talks to this facade -- never
 * to Kalburator::Sync::SyncBackend directly. The reader owns no Qt
 * signals; refresh is driven by PalmRuntime::syncCompleted.
 *
 * Lifetime: the hub pointer is borrowed and outlives the reader.
 * The reader is owned by CalendarBackendPlugin; the plugin outlives
 * the view that borrows the reader pointer.
 */
class HubCalendarReader {
public:
    HubCalendarReader(Kalburator::Sync::SyncBackend *hub,
                      QString collectionId);

    QStringList listRecordIds() const;
    QByteArray  recordBytes(const QString &id) const;
    QString     collectionId() const { return m_collectionId; }

private:
    Kalburator::Sync::SyncBackend *m_hub;
    QString m_collectionId;
};

} // namespace WildPalms::CalendarPlugin

#endif
