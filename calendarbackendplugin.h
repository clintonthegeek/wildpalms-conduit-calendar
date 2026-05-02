#ifndef WILDPALMS_CALENDAR_CALENDARBACKENDPLUGIN_H
#define WILDPALMS_CALENDAR_CALENDARBACKENDPLUGIN_H

#include <memory>

#include <QObject>

#include "core/ibackendplugin_v2.h"

namespace Kalburator::Sync::QSyncCore { struct RecordSnapshot; }
namespace WildPalms::PalmCalendar { class CategoryMappingStore; }
namespace WildPalms::PalmConflict { struct PalmBackendConfig; }
namespace WildPalms::PalmSync { class PalmBackend; }
namespace WildPalms::Runtime { class PalmDeviceAccess; }

namespace WildPalms::CalendarPlugin {

/**
 * @brief Calendar plugin migrated to IBackendPluginV2 (M2 Palm runtime rewrite).
 *
 * Provides:
 *   - CalendarBlobBackend wrapping a per-session PalmBackend (one
 *     collection per populated category slot). Returned by
 *     createPalmBackend(); PC-side backend is now chosen per-mapping
 *     by the user, not by the plugin.
 *   - CalendarConflictHandler (calendar-aware overlays + Palm
 *     delegation).
 *
 * Owns the per-session CategoryMappingStore (populated from the
 * Datebook AppInfo block at createPalmBackend() time) and the
 * per-session PalmBackend adapter.
 *
 * Surfaces CalendarView as a main-window tab (reused unchanged from
 * the legacy CalendarConduit).
 */
class CalendarBackendPlugin : public QObject, public WildPalms::IBackendPluginV2
{
    Q_OBJECT
    Q_INTERFACES(WildPalms::IBackendPluginV2)
public:
    explicit CalendarBackendPlugin(QObject *parent = nullptr);
    ~CalendarBackendPlugin() override;

    // IPlugin
    QString pluginId()    const override;
    QString displayName() const override;
    QIcon   icon()        const override;
    QString description() const override;
    QString version()     const override;

    // IBackendPluginV2
    QStringList claimedDatabases() const override;
    std::unique_ptr<Kalburator::Sync::IBlobBackend>
        createPalmBackend(WildPalms::Runtime::PalmDeviceAccess *device) override;

    // IBackendPluginV2 — conflict handler
    Kalburator::Sync::QSyncCore::ConflictHandler *createConflictHandler() override;

    // IBackendPluginV2 — main view
    bool     hasMainView()   const override;
    QWidget *createMainView(QWidget *parent) const override;
    QString  mainViewName()  const override;
    QIcon    mainViewIcon()  const override;

    // Conflict presentation (called by conflict UI layer)
    void    enrichConflictSnapshot(
        Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot,
        bool isSourceSide) const;
    QString formatConflictRecordHtml(
        const Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot) const;

private:
    std::unique_ptr<WildPalms::PalmCalendar::CategoryMappingStore> m_categoryStore;
    std::unique_ptr<WildPalms::PalmConflict::PalmBackendConfig>    m_palmConfig;
    std::unique_ptr<WildPalms::PalmSync::PalmBackend>              m_palmBackend;
    WildPalms::Runtime::PalmDeviceAccess *m_device = nullptr;   // borrowed; cached for createConflictHandler
};

} // namespace WildPalms::CalendarPlugin

#endif // WILDPALMS_CALENDAR_CALENDARBACKENDPLUGIN_H
