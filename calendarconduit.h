#ifndef CALENDARCONDUIT_H
#define CALENDARCONDUIT_H

#include "sync/conduit.h"

namespace Sync {

/**
 * @brief Conduit for Palm Calendar <-> iCalendar files
 *
 * Syncs:
 *   - Palm DatebookDB (binary format)
 *   - Local .ics files (iCalendar VEVENT format)
 *
 * Uses CalendarMapper for format conversion.
 * Supports bidirectional category sync.
 */
class CalendarConduit : public SyncConduitBase
{
    Q_OBJECT

public:
    explicit CalendarConduit(QObject *parent = nullptr);

    // ========== Conduit Identity ==========

    QString conduitId() const override { return "calendar"; }
    QString displayName() const override { return "Calendar"; }
    QStringList palmDatabaseNames() const override { return {"DatebookDB"}; }
    QString fileExtension() const override { return ".ics"; }

    // ========== UI Contribution ==========
    QIcon icon() const override {
        return QIcon::fromTheme(QStringLiteral("view-calendar"));
    }
    QString description() const override {
        return QStringLiteral("Synchronizes Palm DatebookDB with iCalendar files");
    }
    bool hasView() const override { return true; }
    QWidget *createView(QWidget *parent) override;
    QString viewName() const override { return QStringLiteral("Calendar"); }
    QIcon viewIcon() const override {
        return QIcon::fromTheme(QStringLiteral("view-calendar"));
    }

    // ========== Record Conversion ==========

    BackendRecord* palmToBackend(PilotRecord *palmRecord,
                                  SyncContext *context) override;

    PilotRecord* backendToPalm(BackendRecord *backendRecord,
                                SyncContext *context) override;

    bool recordsEqual(PilotRecord *palm, BackendRecord *backend,
                       const SyncContext *context) const override;

    QString palmRecordDescription(PilotRecord *record,
                                   const SyncContext *context) const override;

    // ========== Conflict Display ==========

    void enrichConflictSnapshot(QSyncCore::RecordSnapshot &snapshot,
                                 bool isSourceSide) const override;
    QString formatConflictRecordHtml(const QSyncCore::RecordSnapshot &snapshot) const override;
};

} // namespace Sync

#endif // CALENDARCONDUIT_H
