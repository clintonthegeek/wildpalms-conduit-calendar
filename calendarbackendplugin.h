#ifndef WILDPALMS_CALENDAR_CALENDARBACKENDPLUGIN_H
#define WILDPALMS_CALENDAR_CALENDARBACKENDPLUGIN_H

#include <memory>

#include "plugins/pimplugin.h"

namespace Kalburator::Conflict { struct RecordSnapshot; class ConflictHandler; }
namespace Kalburator::Sync { class SyncBackend; class SyncBackendBase; }
namespace WildPalms::CalendarPlugin { class HubCalendarReader; }
namespace WildPalms::PalmCalendar { class CategoryMappingStore; }
namespace WildPalms::PalmConflict { struct PalmBackendConfig; }
namespace WildPalms::PalmSync { class PalmBackend; }
namespace WildPalms::Runtime { class PalmDeviceAccess; class PalmRuntime; }

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
class CalendarBackendPlugin : public WildPalms::Plugins::PimPlugin
{
public:
    CalendarBackendPlugin();
    ~CalendarBackendPlugin() override;

    // Kalburator::Plugin — no backend contributions (Palm backends are created
    // directly by PalmRuntime, not via the BackendContribution system).
    QList<std::shared_ptr<Kalburator::Sync::BackendContribution>>
        backendContributions() const override { return {}; }

    // O7: contribute the (calendar, palm) peer shape + palm<->ical edges via
    // the shape-graph contribution system (PluginManager registers them into
    // the injected ShapeRegistries). Replaces the old ctor-time registerWith().
    QList<std::shared_ptr<Kalburator::Shape::ShapeContribution>>
        shapeContributions() const override;

    // Plugin identity
    QString     pluginId()         const { return QStringLiteral("calendar"); }
    QString     displayName()      const;
    QIcon       icon()             const;
    QString     description()      const;
    QString     version()          const;
    QStringList claimedDatabases() const override { return {QStringLiteral("DatebookDB")}; }

    // ── Conduit descriptor (PimPlugin virtuals, substrate A1) ──────
    QString conduitId() const override { return pluginId(); }
    Kalburator::Shape::DomainId domain() const override
    { return Kalburator::Shape::DomainId{QStringLiteral("calendar")}; }
    QString conduitDisplayName() const override { return displayName(); }
    QString conduitIconName() const override
    { return QStringLiteral("office-calendar"); }

    // F.3: Category slot snapshot — used by PalmRuntime::finishConnect to
    // write the snapshot into Profile after createPalmBackend populates
    // m_categoryStore from the live AppInfo block. Returns empty list if
    // the store hasn't been populated yet (e.g., createPalmBackend was
    // never called).
    QString     primaryDbName()       const override { return QStringLiteral("DatebookDB"); }
    QStringList categorySlotNames()   const override;

    // Task 3: borrowed accessor for hub<->remote routing translation.
    WildPalms::PalmCalendar::CategoryMappingStore *categoryStore() const override;

    // Sub-project D: PimPlugin lifecycle hooks.
    void setHub(Kalburator::Sync::SyncBackendBase *hub) override;
    void setRuntime(WildPalms::Runtime::PalmRuntime *runtime) override;

    // Palm backend — called directly by PalmRuntime (Task 6).
    // Substrate A1: return type widened to SyncBackendBase to match the
    // PimPlugin::createPalmBackend descriptor (unique_ptr is not covariant).
    std::unique_ptr<Kalburator::Sync::SyncBackendBase>
        createPalmBackend(WildPalms::Runtime::PalmDeviceAccess *device) override;

    // Conflict handler
    Kalburator::Conflict::ConflictHandler *createConflictHandler() override;

    // Main view
    bool     hasMainView()   const override;
    QWidget *createMainView(QWidget *parent) const override;
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

    // Sub-project D: per-domain reader over the canonical hub; constructed
    // in setHub, fed to CalendarView in createMainView.
    std::unique_ptr<WildPalms::CalendarPlugin::HubCalendarReader> m_hubReader;
    WildPalms::Runtime::PalmRuntime *m_runtime = nullptr;       // borrowed
};

} // namespace WildPalms::CalendarPlugin

#endif // WILDPALMS_CALENDAR_CALENDARBACKENDPLUGIN_H
