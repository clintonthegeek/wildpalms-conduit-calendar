#include "icstranscoder.h"

#include <KCalendarCore/Event>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>

#include "palm/calendar/datebookcodec.h"

namespace WildPalms::CalendarPlugin {

QByteArray encodePalmToIcs(const WildPalms::PalmSync::PalmRecord &record)
{
    auto decoded = WildPalms::PalmCalendar::DatebookCodec::decode(record);
    if (!decoded.isValid()) return {};

    auto cal = KCalendarCore::MemoryCalendar::Ptr(
        new KCalendarCore::MemoryCalendar(QTimeZone::utc()));
    if (!cal->addEvent(decoded.event)) return {};

    KCalendarCore::ICalFormat fmt;
    return fmt.toString(cal).toUtf8();
}

std::optional<WildPalms::PalmSync::PalmRecord>
decodeIcsToPalm(const QByteArray &icsBytes, int slotHint)
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

    return WildPalms::PalmCalendar::DatebookCodec::encode(event, slotHint);
}

} // namespace WildPalms::CalendarPlugin
