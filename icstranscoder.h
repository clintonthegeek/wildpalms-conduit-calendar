#ifndef WILDPALMS_CALENDAR_ICSTRANSCODER_H
#define WILDPALMS_CALENDAR_ICSTRANSCODER_H

#include <optional>

#include <QByteArray>

#include "palm/sync/palmrecord.h"

namespace WildPalms::CalendarPlugin {

/**
 * @brief Encode a Palm Datebook record into iCalendar VCALENDAR bytes.
 *
 * Composes DatebookCodec::decode (Palm bytes -> Event::Ptr) with
 * KCalendarCore::ICalFormat::toString. Returns empty QByteArray on
 * decode failure or on empty event.
 *
 * Pure function. The `record.category` field is preserved on the
 * Event via DatebookCodec's existing X-WP-PALM-CATEGORY-SLOT
 * property, so re-encoding round-trips the slot.
 */
QByteArray encodePalmToIcs(const WildPalms::PalmSync::PalmRecord &record);

/**
 * @brief Decode VCALENDAR bytes into a PalmRecord with the given slot.
 *
 * `slotHint` is forwarded to DatebookCodec::encode and stamped into
 * `PalmRecord::category`. The Event's X-WP-PALM-RECORDID property
 * (if present) populates `PalmRecord::recordId`; otherwise recordId
 * stays 0 and the device assigns on write.
 *
 * Returns std::nullopt if `icsBytes` doesn't parse as a single
 * VEVENT, or if encoding to Palm bytes fails.
 */
std::optional<WildPalms::PalmSync::PalmRecord>
decodeIcsToPalm(const QByteArray &icsBytes, int slotHint);

} // namespace WildPalms::CalendarPlugin

#endif // WILDPALMS_CALENDAR_ICSTRANSCODER_H
