#ifndef WILDPALMS_CALENDAR_CALENDARBACKENDPLUGIN_H
#define WILDPALMS_CALENDAR_CALENDARBACKENDPLUGIN_H

#include <memory>

#include <QObject>

#include "core/ibackendplugin.h"

namespace WildPalms::PalmCalendar { class CategoryMappingStore; }
namespace WildPalms::PalmConflict { struct PalmBackendConfig; }
class PalmDeviceConnection;

namespace WildPalms::CalendarPlugin {

/**
 * @brief Second new-ABI WildPalms plugin (after Memo, E.9).
 *
 * Provides:
 *   - CalendarBlobBackend wrapping the shared PalmBackend (one
 *     collection per populated category slot).
 *   - PalmCalendarBackend (typed SyncBackend, returned for future
 *     PlanStan routing + the unified-runtime calendar tab).
 *   - CalendarConflictHandler (calendar-aware overlays + Palm
 *     delegation).
 *
 * Owns the per-session CategoryMappingStore, populated from the
 * Datebook AppInfo block at createBackends() time.
 *
 * Surfaces CalendarView as a main-window tab (reused unchanged from
 * the legacy CalendarConduit).
 */
class CalendarBackendPlugin : public QObject, public WildPalms::IBackendPlugin
{
    Q_OBJECT
    Q_INTERFACES(WildPalms::IBackendPlugin)
public:
    explicit CalendarBackendPlugin(QObject *parent = nullptr);
    ~CalendarBackendPlugin() override;

    // IPlugin
    QString pluginId()    const override;
    QString displayName() const override;
    QIcon   icon()        const override;
    QString description() const override;
    QString version()     const override;

    // IBackendPlugin
    QStringList      claimedDatabases() const override;
    ProvidedBackends createBackends(Kalburator::Sync::ISyncHost *host,
                                    PalmDeviceConnection         *device) override;

    // IBackendPlugin — conflict handler
    Kalburator::Sync::QSyncCore::ConflictHandler *createConflictHandler() override;

    // IBackendPlugin — main view
    bool     hasMainView()   const override;
    QWidget *createMainView(QWidget *parent) const override;
    QString  mainViewName()  const override;
    QIcon    mainViewIcon()  const override;

    // IBackendPlugin — conflict presentation
    void    enrichConflictSnapshot(
        Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot,
        bool isSourceSide) const override;
    QString formatConflictRecordHtml(
        const Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot) const override;

private:
    std::unique_ptr<WildPalms::PalmCalendar::CategoryMappingStore> m_categoryStore;
    std::unique_ptr<WildPalms::PalmConflict::PalmBackendConfig>    m_palmConfig;
    PalmDeviceConnection *m_device = nullptr;   // borrowed; cached for createConflictHandler
};

} // namespace WildPalms::CalendarPlugin

#endif // WILDPALMS_CALENDAR_CALENDARBACKENDPLUGIN_H
