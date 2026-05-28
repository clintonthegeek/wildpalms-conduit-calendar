#include "palmtoicstransformation.h"

#include "icstranscoder.h"
#include "palm/sync/palmrecord.h"

using namespace Kalburator::Shape;

namespace WildPalms::CalendarPlugin {

PalmToIcsStage::PalmToIcsStage(const WildPalms::PalmCalendar::CategoryMappingStore *cats)
    : m_cats(cats)
{
}

QByteArray PalmToIcsStage::transform(const QByteArray &sourceBytes) const
{
    if (sourceBytes.isEmpty()) return {};
    const auto pr = WildPalms::PalmSync::PalmRecord::fromWireBytes(sourceBytes);
    return encodePalmToIcs(pr, m_cats, QStringLiteral("DatebookDB"));
}

IcsToPalmStage::IcsToPalmStage(const WildPalms::PalmCalendar::CategoryMappingStore *cats)
    : m_cats(cats)
{
}

QByteArray IcsToPalmStage::transform(const QByteArray &sourceBytes) const
{
    if (sourceBytes.isEmpty()) return {};
    // The category slot is derived from the iCalendar CATEGORIES property via
    // the borrowed CategoryMappingStore (name -> slot). With no store / no
    // categories the slot is 0 (Unfiled).
    const auto prOpt = decodeIcsToPalm(sourceBytes, m_cats, QStringLiteral("DatebookDB"));
    if (!prOpt) return {};
    return prOpt->toWireBytes();
}

LossProfile palmToIcsLoss()
{
    // lossless: every Palm DatebookDB field maps directly onto a VEVENT field;
    // nothing is dropped going palm -> ical. (Identity X- stamps are preserved
    // downstream by libkalburator's ical<->canon stage.)
    return {};
}

LossProfile icsToPalmLoss()
{
    LossProfile p;
    // Palm DatebookDB has no field for these (canon property vocabulary):
    p.affected.insert(PropertyId{QStringLiteral("location")},        LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("locations")},       LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("attendees")},       LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("organizer")},       LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("priority")},        LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("status")},          LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("url")},             LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("attachments")},     LossKind::Dropped);
    // categories is NO LONGER dropped: the Palm category slot carries the
    // canonical `categories` field (name-based) via CategoryMappingStore.
    p.affected.insert(PropertyId{QStringLiteral("timeTransparency")},LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("freeBusyStatus")},  LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("onlineMeeting")},   LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("eventType")},       LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("classification")},  LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("color")},           LossKind::Dropped);
    // Survive in reduced form: Palm holds one display alarm and a recurrence
    // subset (no BYSETPOS/BYWEEKNO/multi-rule).
    p.affected.insert(PropertyId{QStringLiteral("alarms")},          LossKind::Simplified);
    p.affected.insert(PropertyId{QStringLiteral("recurrence")},      LossKind::Simplified);
    return p;
}

} // namespace WildPalms::CalendarPlugin
