#include "palmtoicstransformation.h"

#include "icstranscoder.h"
#include "palm/sync/palmrecord.h"

using namespace Kalburator::Shape;

namespace WildPalms::CalendarPlugin {

QByteArray PalmToIcsStage::transform(const QByteArray &sourceBytes) const
{
    if (sourceBytes.isEmpty()) return {};
    const auto pr = WildPalms::PalmSync::PalmRecord::fromWireBytes(sourceBytes);
    return encodePalmToIcs(pr);
}

QByteArray IcsToPalmStage::transform(const QByteArray &sourceBytes) const
{
    if (sourceBytes.isEmpty()) return {};
    // slotHint = -1 is clamped to 0 by DatebookCodec::encode, so the category
    // field of this stage's output is NOT authoritative. The backend
    // (createRecord/updateRecord) sets the real slot from collection context on
    // write; X-WP-PALM-CATEGORY-SLOT rides on the Event but encode() does not
    // read it back. Treat the wire bytes' category as undefined after this stage.
    const auto prOpt = decodeIcsToPalm(sourceBytes, /*slotHint*/ -1);
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
    p.affected.insert(PropertyId{QStringLiteral("categories")},      LossKind::Dropped);
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
