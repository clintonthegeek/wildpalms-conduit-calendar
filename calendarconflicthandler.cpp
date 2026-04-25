#include "calendarconflicthandler.h"

#include <algorithm>

#include <KCalendarCore/Alarm>
#include <KCalendarCore/Event>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>
#include <KCalendarCore/Recurrence>

#include "conflictrecord.h"

#include "palm/conflict/palmconflicthandler.h"
#include "palm/conflict/palmbackendconfig.h"

namespace WildPalms::CalendarPlugin {

namespace {

KCalendarCore::Event::Ptr decodeFirstEvent(const QByteArray &icsBytes)
{
    if (icsBytes.isEmpty()) return {};
    KCalendarCore::ICalFormat fmt;
    auto cal = KCalendarCore::MemoryCalendar::Ptr(
        new KCalendarCore::MemoryCalendar(QTimeZone::utc()));
    if (!fmt.fromString(cal, QString::fromUtf8(icsBytes))) return {};
    auto events = cal->events();
    return events.isEmpty() ? KCalendarCore::Event::Ptr() : events.first();
}

QByteArray serialiseEvent(const KCalendarCore::Event::Ptr &event)
{
    auto cal = KCalendarCore::MemoryCalendar::Ptr(
        new KCalendarCore::MemoryCalendar(QTimeZone::utc()));
    cal->addEvent(event);
    return KCalendarCore::ICalFormat().toString(cal).toUtf8();
}

// Compare events ignoring fields named in `ignored`. Returns true if
// events match on every other significant field.
struct EventDiff {
    bool summaryDiffers = false;
    bool dtStartTimeDiffers = false;     // wall-clock differs
    bool dtStartTzDiffers = false;       // only timezone differs
    bool dtEndDiffers = false;
    bool descriptionDiffers = false;
    bool locationDiffers = false;
    bool alarmsDiffer = false;
    bool exdatesDiffer = false;
    bool recurrenceShapeDiffers = false;
};

EventDiff diffEvents(const KCalendarCore::Event::Ptr &a,
                     const KCalendarCore::Event::Ptr &b)
{
    EventDiff d;
    d.summaryDiffers = a->summary() != b->summary();

    const QDateTime as = a->dtStart(), bs = b->dtStart();
    if (as.toUTC() != bs.toUTC()) {
        d.dtStartTimeDiffers = true;
    } else if (as.timeSpec() != bs.timeSpec()
               || (as.timeSpec() == Qt::TimeZone && as.timeZone() != bs.timeZone())) {
        d.dtStartTzDiffers = true;
    }
    d.dtEndDiffers       = a->dtEnd() != b->dtEnd();
    d.descriptionDiffers = a->description() != b->description();
    d.locationDiffers    = a->location() != b->location();

    auto sortedAlarms = [](const KCalendarCore::Event::Ptr &e) {
        QList<int> offsets;
        for (const auto &al : e->alarms()) {
            offsets.append(al->startOffset().asSeconds());
        }
        std::sort(offsets.begin(), offsets.end());
        return offsets;
    };
    d.alarmsDiffer = sortedAlarms(a) != sortedAlarms(b);

    auto sortedExdates = [](const KCalendarCore::Event::Ptr &e) {
        QList<QDateTime> ds;
        for (const auto &dt : e->recurrence()->exDateTimes()) ds.append(dt.toUTC());
        std::sort(ds.begin(), ds.end());
        return ds;
    };
    d.exdatesDiffer = sortedExdates(a) != sortedExdates(b);

    // Recurrence "shape" = duration, frequency, type. EXDATE handled
    // separately above so it doesn't trip the shape check.
    auto ar = a->recurrence();
    auto br = b->recurrence();
    d.recurrenceShapeDiffers =
        ar->recurrenceType() != br->recurrenceType()
        || ar->frequency() != br->frequency()
        || ar->duration() != br->duration();

    return d;
}

bool onlyAlarmsDiffer(const EventDiff &d)
{
    return d.alarmsDiffer
        && !d.summaryDiffers && !d.dtStartTimeDiffers && !d.dtStartTzDiffers
        && !d.dtEndDiffers && !d.descriptionDiffers && !d.locationDiffers
        && !d.exdatesDiffer && !d.recurrenceShapeDiffers;
}

bool onlyExdatesDiffer(const EventDiff &d)
{
    return d.exdatesDiffer
        && !d.summaryDiffers && !d.dtStartTimeDiffers && !d.dtStartTzDiffers
        && !d.dtEndDiffers && !d.descriptionDiffers && !d.locationDiffers
        && !d.alarmsDiffer && !d.recurrenceShapeDiffers;
}

bool onlyTzDiffers(const EventDiff &d)
{
    return d.dtStartTzDiffers
        && !d.dtStartTimeDiffers && !d.summaryDiffers && !d.dtEndDiffers
        && !d.descriptionDiffers && !d.locationDiffers
        && !d.alarmsDiffer && !d.exdatesDiffer && !d.recurrenceShapeDiffers;
}

KCalendarCore::Event::Ptr mergeAlarms(const KCalendarCore::Event::Ptr &base,
                                      const KCalendarCore::Event::Ptr &other)
{
    auto out = KCalendarCore::Event::Ptr(base->clone());
    out->clearAlarms();
    QList<int> seen;
    auto addUnique = [&](const KCalendarCore::Event::Ptr &src) {
        for (const auto &al : src->alarms()) {
            const int s = al->startOffset().asSeconds();
            if (seen.contains(s)) continue;
            seen.append(s);
            auto dup = out->newAlarm();
            dup->setType(al->type());
            dup->setStartOffset(al->startOffset());
        }
    };
    addUnique(base);
    addUnique(other);
    return out;
}

KCalendarCore::Event::Ptr mergeExdates(const KCalendarCore::Event::Ptr &base,
                                       const KCalendarCore::Event::Ptr &other)
{
    auto out = KCalendarCore::Event::Ptr(base->clone());
    for (const auto &dt : other->recurrence()->exDateTimes()) {
        if (!out->recurrence()->exDateTimes().contains(dt)) {
            out->recurrence()->addExDateTime(dt);
        }
    }
    return out;
}

KCalendarCore::Event::Ptr pickFloatingSide(const KCalendarCore::Event::Ptr &a,
                                           const KCalendarCore::Event::Ptr &b)
{
    const bool aFloating = (a->dtStart().timeSpec() == Qt::LocalTime);
    return aFloating ? KCalendarCore::Event::Ptr(a->clone())
                     : KCalendarCore::Event::Ptr(b->clone());
}

} // namespace

CalendarConflictHandler::CalendarConflictHandler(
    WildPalms::PalmSync::IPalmDatabaseAccess *device,
    const WildPalms::PalmConflict::PalmBackendConfig *config)
    : m_palm(std::make_unique<WildPalms::PalmConflict::PalmConflictHandler>(
          device, config))
{
}

CalendarConflictHandler::~CalendarConflictHandler() = default;

Kalburator::Sync::QSyncCore::ConflictDecision
CalendarConflictHandler::handleConflict(
    Kalburator::Sync::QSyncCore::ConflictRecord &conflict,
    const Kalburator::Sync::QSyncCore::ConflictPolicy &policy)
{
    auto src = decodeFirstEvent(conflict.source.content);
    auto tgt = decodeFirstEvent(conflict.target.content);
    if (!src || !tgt) {
        m_lastOverlay = QStringLiteral("delegated");
        return m_palm->handleConflict(conflict, policy);
    }

    const EventDiff d = diffEvents(src, tgt);

    if (onlyAlarmsDiffer(d)) {
        auto merged = mergeAlarms(src, tgt);
        conflict.mergedContent = serialiseEvent(merged);
        m_lastOverlay = QStringLiteral("alarm");
        return Kalburator::Sync::QSyncCore::ConflictDecision::Merge;
    }
    if (onlyExdatesDiffer(d)) {
        auto merged = mergeExdates(src, tgt);
        conflict.mergedContent = serialiseEvent(merged);
        m_lastOverlay = QStringLiteral("exdate");
        return Kalburator::Sync::QSyncCore::ConflictDecision::Merge;
    }
    if (onlyTzDiffers(d)) {
        auto chosen = pickFloatingSide(src, tgt);
        conflict.mergedContent = serialiseEvent(chosen);
        m_lastOverlay = QStringLiteral("tz");
        return Kalburator::Sync::QSyncCore::ConflictDecision::Merge;
    }

    m_lastOverlay = QStringLiteral("delegated");
    return m_palm->handleConflict(conflict, policy);
}

} // namespace WildPalms::CalendarPlugin
