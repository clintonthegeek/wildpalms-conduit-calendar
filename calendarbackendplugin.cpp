#include "calendarbackendplugin.h"

#include "calendarblobbackend.h"
#include "calendarconflicthandler.h"
#include "calendarview.h"
#include "categoryappinforeader.h"
#include "icstranscoder.h"

#include "palm/calendar/categorymappingstore.h"
#include "palm/calendar/palmcalendarbackend.h"
#include "palm/conflict/palmbackendconfig.h"
#include "palm/palmdeviceconnection.h"
#include "palm/sync/palmbackend.h"

#include "conflictrecord.h"

#include <KCalendarCore/Event>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>

#include <QIcon>
#include <QString>
#include <QWidget>

namespace WildPalms::CalendarPlugin {

CalendarBackendPlugin::CalendarBackendPlugin(QObject *parent)
    : QObject(parent)
    , m_categoryStore(std::make_unique<WildPalms::PalmCalendar::CategoryMappingStore>())
    , m_palmConfig(std::make_unique<WildPalms::PalmConflict::PalmBackendConfig>())
{
}

CalendarBackendPlugin::~CalendarBackendPlugin() = default;

QString CalendarBackendPlugin::pluginId()    const { return QStringLiteral("calendar"); }
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

QStringList CalendarBackendPlugin::claimedDatabases() const
{
    return { QStringLiteral("DatebookDB") };
}

WildPalms::IBackendPlugin::ProvidedBackends
CalendarBackendPlugin::createBackends(Kalburator::Sync::ISyncHost *host,
                                      PalmDeviceConnection         *device)
{
    Q_UNUSED(host)
    ProvidedBackends out;
    if (!device) return out;

    m_device = device;

    auto *palmBackend = device->palmBackend();
    if (palmBackend) {
        // Populate the category store from AppInfo. Failure is non-fatal:
        // the backend still surfaces palm:calendar/0 ("Unfiled").
        populateFromAppInfo(*m_categoryStore,
                            QStringLiteral("DatebookDB"),
                            palmBackend->readAppBlock(QStringLiteral("DatebookDB")));
        out.blob = new CalendarBlobBackend(palmBackend, m_categoryStore.get());
    }

    if (device->device()) {
        out.calendar = new WildPalms::PalmCalendar::PalmCalendarBackend(
            device->device(), m_categoryStore.get());
    }
    return out;
}

Kalburator::Sync::QSyncCore::ConflictHandler *
CalendarBackendPlugin::createConflictHandler()
{
    if (!m_device || !m_device->device()) return nullptr;
    return new CalendarConflictHandler(m_device->device(), m_palmConfig.get());
}

bool CalendarBackendPlugin::hasMainView() const { return true; }

QWidget *CalendarBackendPlugin::createMainView(QWidget *parent) const
{
    return new CalendarView(parent);
}

QString CalendarBackendPlugin::mainViewName() const { return QStringLiteral("Calendar"); }

QIcon CalendarBackendPlugin::mainViewIcon() const
{
    return QIcon::fromTheme(QStringLiteral("view-calendar"));
}

void CalendarBackendPlugin::enrichConflictSnapshot(
    Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot,
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
    const Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot) const
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

} // namespace WildPalms::CalendarPlugin

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(CalendarBackendPluginFactory,
                           "calendar-backend-plugin.json",
                           registerPlugin<WildPalms::CalendarPlugin::CalendarBackendPlugin>();)

#include "calendarbackendplugin.moc"
