#include "icstranscoder.h"

#include <KCalendarCore/Event>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>

#include "palm/calendar/datebookcodec.h"
#include "palm/calendar/categorymappingstore.h"

namespace WildPalms::CalendarPlugin {

QByteArray encodePalmToIcs(const WildPalms::PalmSync::PalmRecord &record,
                           const WildPalms::PalmCalendar::CategoryMappingStore *cats,
                           const QString &dbName)
{
    auto decoded = WildPalms::PalmCalendar::DatebookCodec::decode(record);
    if (!decoded.isValid()) return {};

    // Carry the Palm category slot as the iCalendar CATEGORIES property
    // (name-based). libkalburator's ical<->canon stage lifts it into
    // canon `categories`.
    if (cats && record.category != 0) {
        const QString nm = cats->slotName(dbName, record.category);
        if (!nm.isEmpty()) decoded.event->setCategories(QStringList{nm});
    }

    auto cal = KCalendarCore::MemoryCalendar::Ptr(
        new KCalendarCore::MemoryCalendar(QTimeZone::utc()));
    if (!cal->addEvent(decoded.event)) return {};

    KCalendarCore::ICalFormat fmt;
    return fmt.toString(cal).toUtf8();
}

std::optional<WildPalms::PalmSync::PalmRecord>
decodeIcsToPalm(const QByteArray &icsBytes,
                const WildPalms::PalmCalendar::CategoryMappingStore *cats,
                const QString &dbName)
{
    if (icsBytes.isEmpty()) return std::nullopt;

    KCalendarCore::ICalFormat fmt;
    auto cal = KCalendarCore::MemoryCalendar::Ptr(
        new KCalendarCore::MemoryCalendar(QTimeZone::utc()));
    if (!fmt.fromString(cal, QString::fromUtf8(icsBytes))) {
        return std::nullopt;
    }

    auto events = cal->events();
    if (events.isEmpty()) return std::nullopt;
    auto event = events.first();
    if (!event) return std::nullopt;

    // Map the first CATEGORIES name back to a Palm slot. No store or no
    // categories => slot 0 (Unfiled).
    const int slot = (cats && !event->categories().isEmpty())
        ? cats->slotForName(dbName, event->categories().constFirst())
        : 0;

    return WildPalms::PalmCalendar::DatebookCodec::encode(event, slot);
}

} // namespace WildPalms::CalendarPlugin
