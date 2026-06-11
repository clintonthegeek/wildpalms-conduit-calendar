#include "calendarbackendplugin.h"

#include "hubcalendarreader.h"
#include "palmcalendarbackend.h"
#include "calendarconflicthandler.h"
#include "calendardomainextension.h"
#include "calendarview.h"

#include "palm/calendar/categoryappinforeader.h"
#include "palm/calendar/categorymappingstore.h"
#include "palm/conflict/palmbackendconfig.h"
#include "palm/sync/palmbackend.h"
#include "runtime/palmdeviceaccess.h"
#include "runtime/palmruntime.h"

#include "conflictrecord.h"

#include <KCalendarCore/Event>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>

#include <QIcon>
#include <QLoggingCategory>
#include <QString>
#include <QWidget>

namespace {
Q_LOGGING_CATEGORY(WP_CALENDAR_PLUGIN, "wildpalms.calendar.plugin")
}

namespace WildPalms::CalendarPlugin {

CalendarBackendPlugin::CalendarBackendPlugin()
    : m_categoryStore(std::make_unique<WildPalms::PalmCalendar::CategoryMappingStore>())
    , m_palmConfig(std::make_unique<WildPalms::PalmConflict::PalmBackendConfig>())
{
    // O7: shape registration moved out of the ctor into shapeContributions();
    // PluginManager registers the contribution into the injected ShapeRegistries.
}

CalendarBackendPlugin::~CalendarBackendPlugin() = default;

QList<std::shared_ptr<Kalburator::Shape::ShapeContribution>>
CalendarBackendPlugin::shapeContributions() const
{
    // Timing contract: this runs at PluginManager load time (PalmRuntime ctor),
    // BEFORE createPalmBackend() populates m_categoryStore from the AppInfo block
    // at device-connect. The contribution/stages keep a borrowed POINTER (not a
    // snapshot), so by the time any transform runs (post-connect) the store is
    // populated. Do not cache the store by value, and do not assume it is
    // populated here.
    return { std::make_shared<CalendarPalmShapes>(m_categoryStore.get()) };
}

QString CalendarBackendPlugin::displayName() const { return QStringLiteral("Calendar"); }
QIcon   CalendarBackendPlugin::icon()        const
{
    return QIcon::fromTheme(QStringLiteral("view-calendar"));
}
QString CalendarBackendPlugin::description() const
{
    return QStringLiteral(
        "Synchronizes Palm DatebookDB with iCalendar files via virtual category sub-calendars");
}
QString CalendarBackendPlugin::version()     const { return QStringLiteral("2.0"); }

std::unique_ptr<Kalburator::Sync::SyncBackendBase>
CalendarBackendPlugin::createPalmBackend(WildPalms::Runtime::PalmDeviceAccess *device)
{
    if (!device) return nullptr;

    // Cached for createConflictHandler. Re-entry overwrites: the
    // plugin contract is once-per-session per device, so a
    // second call implies a new session and is intentional.
    m_device = device;

    // Build a PalmBackend adapter over the device. PalmDeviceAccess IS-A
    // IPalmDatabaseAccess, so no cast needed. The plugin owns this backend
    // for the duration of the session.
    m_palmBackend = std::make_unique<WildPalms::PalmSync::PalmBackend>(device);

    // Populate the category store from AppInfo. Failure is non-fatal:
    // the backend still surfaces palm:calendar/0 ("Unfiled").
    WildPalms::PalmCalendar::populateFromAppInfo(
        *m_categoryStore,
        QStringLiteral("DatebookDB"),
        m_palmBackend->readAppBlock(QStringLiteral("DatebookDB")));

    return std::make_unique<PalmCalendarBackend>(m_palmBackend.get(), m_categoryStore.get());
}

Kalburator::Conflict::ConflictHandler *
CalendarBackendPlugin::createConflictHandler()
{
    if (!m_device) {
        qCWarning(WP_CALENDAR_PLUGIN)
            << "createConflictHandler called before createPalmBackend — "
               "runtime must invoke createPalmBackend first to wire the device.";
        return nullptr;
    }
    // PalmDeviceAccess IS-A IPalmDatabaseAccess; no cast needed.
    return new CalendarConflictHandler(m_device, m_palmConfig.get());
}

bool CalendarBackendPlugin::hasMainView() const { return true; }

QWidget *CalendarBackendPlugin::createMainView(QWidget *parent) const
{
    auto *v = new CalendarView(parent);
    v->setHubReader(m_hubReader.get());
    if (m_runtime) {
        QObject::connect(m_runtime,
                         &WildPalms::Runtime::PalmRuntime::syncCompleted,
                         v, &CalendarView::refresh);
    }
    return v;
}

void CalendarBackendPlugin::setHub(Kalburator::Sync::SyncBackendBase *hub)
{
    Q_ASSERT(hub);
    m_hubReader = std::make_unique<WildPalms::CalendarPlugin::HubCalendarReader>(
        hub, QStringLiteral("palm:calendar"));
}

void CalendarBackendPlugin::setRuntime(WildPalms::Runtime::PalmRuntime *runtime)
{
    m_runtime = runtime;
}

QString CalendarBackendPlugin::mainViewName() const { return QStringLiteral("Calendar"); }

QIcon CalendarBackendPlugin::mainViewIcon() const
{
    return QIcon::fromTheme(QStringLiteral("view-calendar"));
}

void CalendarBackendPlugin::enrichConflictSnapshot(
    Kalburator::Conflict::RecordSnapshot &snapshot,
    bool /*isSourceSide*/) const
{
    if (snapshot.content.isEmpty()) return;

    KCalendarCore::ICalFormat fmt;
    auto cal = KCalendarCore::MemoryCalendar::Ptr(
        new KCalendarCore::MemoryCalendar(QTimeZone::utc()));
    if (!fmt.fromString(cal, QString::fromUtf8(snapshot.content))) return;
    auto events = cal->events();
    if (events.isEmpty()) return;
    auto event = events.first();
    if (!event) return;

    snapshot.metadata[QStringLiteral("title")] = event->summary();
    snapshot.metadata[QStringLiteral("dtStart")] = event->dtStart().toString(Qt::ISODate);
    snapshot.contentType = QStringLiteral("text/calendar");
}

QString CalendarBackendPlugin::formatConflictRecordHtml(
    const Kalburator::Conflict::RecordSnapshot &snapshot) const
{
    QString html;
    const QString title   = snapshot.metadata.value(QStringLiteral("title")).toString();
    const QString dtStart = snapshot.metadata.value(QStringLiteral("dtStart")).toString();
    if (!title.isEmpty()) {
        html += QStringLiteral("<h3>%1</h3>").arg(title.toHtmlEscaped());
    }
    if (!dtStart.isEmpty()) {
        html += QStringLiteral("<p><b>Starts:</b> %1</p>").arg(dtStart.toHtmlEscaped());
    }
    html += QStringLiteral("<pre>%1</pre>")
        .arg(QString::fromUtf8(snapshot.content).toHtmlEscaped());
    return html;
}

QStringList CalendarBackendPlugin::categorySlotNames() const
{
    if (!m_categoryStore) return {};
    return m_categoryStore->sixteenSlotNames(primaryDbName());
}

WildPalms::PalmCalendar::CategoryMappingStore *
CalendarBackendPlugin::categoryStore() const
{
    return m_categoryStore.get();
}

} // namespace WildPalms::CalendarPlugin
