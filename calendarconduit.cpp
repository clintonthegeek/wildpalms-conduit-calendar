#include "calendarconduit.h"
#include "calendarview.h"
#include "calendarmapper.h"
#include "palm/pilotrecord.h"
#include "palm/categoryinfo.h"
#include "sync/localfilebackend.h"
#include "sync/qsynccore/conflictrecord.h"

#include <QDebug>

namespace Sync {

CalendarConduit::CalendarConduit(QObject *parent)
    : SyncConduitBase(parent)
{
}

BackendRecord* CalendarConduit::palmToBackend(PilotRecord *palmRecord,
                                               SyncContext *context)
{
    if (!palmRecord) return nullptr;

    // Unpack Palm event
    CalendarMapper::Event event = CalendarMapper::unpackEvent(palmRecord);

    // Convert to iCalendar
    QString catName = categoryName(event.category);
    QString ical = CalendarMapper::eventToICal(event, catName);

    // Create backend record
    BackendRecord *record = new BackendRecord();
    record->data = ical.toUtf8();
    record->type = "event";
    record->contentHash = LocalFileBackend::calculateHash(record->data);
    record->lastModified = QDateTime::currentDateTime();

    // Set display name: "Title YYYY-MM-DD" or just date if no title
    QString displayName = event.description;
    if (displayName.isEmpty()) {
        displayName = "Event";
    }
    if (displayName.length() > 40) {
        displayName = displayName.left(40);
    }
    if (event.begin.isValid()) {
        displayName += " " + event.begin.toString("yyyy-MM-dd");
    }
    record->displayName = displayName;

    return record;
}

PilotRecord* CalendarConduit::backendToPalm(BackendRecord *backendRecord,
                                             SyncContext *context)
{
    if (!backendRecord) return nullptr;

    // Parse iCalendar content
    QString content = QString::fromUtf8(backendRecord->data);
    CalendarMapper::Event event = CalendarMapper::iCalToEvent(content);

    // Look up or create category from name
    if (!event.categoryName.isEmpty() && m_categories) {
        event.category = m_categories->getOrCreateCategory(event.categoryName);
        qDebug() << "[CalendarConduit] Category" << event.categoryName << "-> index" << event.category;
    }

    // Pack to Palm record
    PilotRecord *record = CalendarMapper::packEvent(event);

    return record;
}

bool CalendarConduit::recordsEqual(PilotRecord *palm, BackendRecord *backend) const
{
    if (!palm || !backend) return false;

    // Unpack Palm event
    CalendarMapper::Event palmEvent = CalendarMapper::unpackEvent(palm);

    // Parse backend content
    QString backendContent = QString::fromUtf8(backend->data);
    CalendarMapper::Event backendEvent = CalendarMapper::iCalToEvent(backendContent);

    // Compare key fields
    if (palmEvent.description != backendEvent.description) return false;
    if (palmEvent.begin != backendEvent.begin) return false;
    if (palmEvent.end != backendEvent.end) return false;
    if (palmEvent.isUntimed != backendEvent.isUntimed) return false;

    // Compare categories
    QString palmCategoryName = categoryName(palmEvent.category);

    // Normalize: "Unfiled" (index 0) and empty string are equivalent
    QString normalizedPalmCat = palmCategoryName;
    QString normalizedBackendCat = backendEvent.categoryName;

    if (normalizedPalmCat.compare("Unfiled", Qt::CaseInsensitive) == 0) {
        normalizedPalmCat.clear();
    }
    if (normalizedBackendCat.compare("Unfiled", Qt::CaseInsensitive) == 0) {
        normalizedBackendCat.clear();
    }

    if (normalizedPalmCat.compare(normalizedBackendCat, Qt::CaseInsensitive) != 0) {
        return false;
    }

    return true;
}

QString CalendarConduit::palmRecordDescription(PilotRecord *record) const
{
    if (!record) return QString();

    CalendarMapper::Event event = CalendarMapper::unpackEvent(record);

    QString desc = event.description;
    if (desc.isEmpty()) {
        desc = "<Untitled Event>";
    }

    // Add date info
    if (event.begin.isValid()) {
        desc += QString(" (%1)").arg(event.begin.toString("yyyy-MM-dd"));
    }

    return desc;
}

void CalendarConduit::enrichConflictSnapshot(QSyncCore::RecordSnapshot &snapshot,
                                               bool isSourceSide) const
{
    if (snapshot.content.isEmpty()) return;

    CalendarMapper::Event event;

    if (isSourceSide) {
        // Source: Palm binary — unpack via mapper, convert to iCal text
        PilotRecord tempRecord(0, 0, 0, snapshot.content);
        event = CalendarMapper::unpackEvent(&tempRecord);
        QString catName = categoryName(event.category);
        snapshot.content = CalendarMapper::eventToICal(event, catName).toUtf8();
    } else {
        // Target: already iCal text — parse for metadata
        event = CalendarMapper::iCalToEvent(QString::fromUtf8(snapshot.content));
    }

    // Populate metadata
    if (!event.description.isEmpty())
        snapshot.metadata[QStringLiteral("summary")] = event.description;
    if (event.begin.isValid())
        snapshot.metadata[QStringLiteral("dtstart")] = event.begin.toString(QStringLiteral("yyyy-MM-dd hh:mm"));
    if (event.end.isValid())
        snapshot.metadata[QStringLiteral("dtend")] = event.end.toString(QStringLiteral("yyyy-MM-dd hh:mm"));
    if (!event.note.isEmpty())
        snapshot.metadata[QStringLiteral("description")] = event.note;
    if (event.repeatType != CalendarMapper::RepeatNone)
        snapshot.metadata[QStringLiteral("recurrence")] = QStringLiteral("Yes");
    if (event.isUntimed)
        snapshot.metadata[QStringLiteral("all_day")] = QStringLiteral("Yes");

    snapshot.contentType = QStringLiteral("text/calendar");
}

QString CalendarConduit::formatConflictRecordHtml(const QSyncCore::RecordSnapshot &snapshot) const
{
    QString html;
    const QVariantMap &m = snapshot.metadata;

    QString summary = m.value(QStringLiteral("summary")).toString();
    if (!summary.isEmpty())
        html += QStringLiteral("<h3>%1</h3>").arg(summary.toHtmlEscaped());

    html += QStringLiteral("<table cellpadding='4'>");

    auto addRow = [&html](const QString &label, const QString &value) {
        if (!value.isEmpty())
            html += QStringLiteral("<tr><td><b>%1:</b></td><td>%2</td></tr>")
                .arg(label.toHtmlEscaped(), value.toHtmlEscaped());
    };

    addRow(QStringLiteral("Start"), m.value(QStringLiteral("dtstart")).toString());
    addRow(QStringLiteral("End"), m.value(QStringLiteral("dtend")).toString());
    addRow(QStringLiteral("All Day"), m.value(QStringLiteral("all_day")).toString());
    addRow(QStringLiteral("Recurrence"), m.value(QStringLiteral("recurrence")).toString());
    addRow(QStringLiteral("Description"), m.value(QStringLiteral("description")).toString());

    html += QStringLiteral("</table>");

    return html;
}

QWidget *CalendarConduit::createView(QWidget *parent)
{
    return new CalendarView(parent);
}

} // namespace Sync

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(CalendarConduitFactory, "calendar-conduit.json",
                           registerPlugin<Sync::CalendarConduit>();)

#include "calendarconduit.moc"
