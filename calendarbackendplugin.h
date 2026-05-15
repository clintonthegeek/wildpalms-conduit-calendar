#ifndef WILDPALMS_CALENDAR_CALENDARBACKENDPLUGIN_H
#define WILDPALMS_CALENDAR_CALENDARBACKENDPLUGIN_H

#include <memory>

#include "plugin.h"

namespace Kalburator::Conflict { struct RecordSnapshot; class ConflictHandler; }
namespace Kalburator::Sync { class SyncBackend; }
namespace WildPalms::PalmCalendar { class CategoryMappingStore; }
namespace WildPalms::PalmConflict { struct PalmBackendConfig; }
namespace WildPalms::PalmSync { class PalmBackend; }
namespace WildPalms::Runtime { class PalmDeviceAccess; }

class QIcon;
class QWidget;

namespace WildPalms::CalendarPlugin {

/**
 * @brief Calendar plugin (K.8b): inherits Kalburator::Plugin (K.7 surface).
 *
 * No longer a KCoreAddons MODULE plugin. Linked STATIC and loaded
 * in-process by PalmRuntime::registerPalmPlugins() (Task 6).
 *
 * Provides:
 *   - PalmCalendarBackend via createPalmBackend() — called directly
 *     by PalmRuntime; not routed through BackendContributions.
 *   - CalendarConflictHandler (calendar-aware overlays + Palm
 *     delegation).
 *   - CalendarView as a main-window tab.
 */
class CalendarBackendPlugin : public Kalburator::Plugin
{
public:
    CalendarBackendPlugin();
    ~CalendarBackendPlugin() override;

    // Kalburator::Plugin — all return {} (Palm plugins don't contribute
    // to the libkalburator BackendContribution system)
    QList<std::shared_ptr<Kalburator::Sync::BackendContribution>>
        backendContributions() const override { return {}; }

    // Plugin identity
    QString     pluginId()         const { return QStringLiteral("calendar"); }
    QString     displayName()      const;
    QIcon       icon()             const;
    QString     description()      const;
    QString     version()          const;
    QStringList claimedDatabases() const { return {QStringLiteral("DatebookDB")}; }

    // Palm backend — called directly by PalmRuntime (Task 6)
    std::unique_ptr<Kalburator::Sync::SyncBackend>
        createPalmBackend(WildPalms::Runtime::PalmDeviceAccess *device);

    // Conflict handler
    Kalburator::Conflict::ConflictHandler *createConflictHandler();

    // Main view
    bool     hasMainView()   const;
    QWidget *createMainView(QWidget *parent) const;
    QString  mainViewName()  const;
    QIcon    mainViewIcon()  const;

    // Conflict presentation (called by conflict UI layer)
    void    enrichConflictSnapshot(
        Kalburator::Conflict::RecordSnapshot &snapshot,
        bool isSourceSide) const;
    QString formatConflictRecordHtml(
        const Kalburator::Conflict::RecordSnapshot &snapshot) const;

private:
    std::unique_ptr<WildPalms::PalmCalendar::CategoryMappingStore> m_categoryStore;
    std::unique_ptr<WildPalms::PalmConflict::PalmBackendConfig>    m_palmConfig;
    std::unique_ptr<WildPalms::PalmSync::PalmBackend>              m_palmBackend;
    WildPalms::Runtime::PalmDeviceAccess *m_device = nullptr;   // borrowed; cached for createConflictHandler
};

} // namespace WildPalms::CalendarPlugin

#endif // WILDPALMS_CALENDAR_CALENDARBACKENDPLUGIN_H
