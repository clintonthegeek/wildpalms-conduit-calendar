#ifndef WILDPALMS_CALENDAR_PALMTOICSTRANSFORMATION_H
#define WILDPALMS_CALENDAR_PALMTOICSTRANSFORMATION_H

#include "transformationedge.h"

namespace WildPalms::CalendarPlugin {

// (calendar, palm) -> (calendar, ical): wraps DatebookCodec via icstranscoder.
// Lossless: a Palm appointment is a subset of a VEVENT (identity X- stamps
// are preserved downstream by libkalburator's ical<->canon stage).
class PalmToIcsStage : public Kalburator::Shape::TransformationStage {
public:
    QByteArray transform(const QByteArray &sourceBytes) const override;
};

// (calendar, ical) -> (calendar, palm): lossy; Palm DatebookDB cannot hold
// most VEVENT fields.
class IcsToPalmStage : public Kalburator::Shape::TransformationStage {
public:
    QByteArray transform(const QByteArray &sourceBytes) const override;
};

Kalburator::Shape::LossProfile palmToIcsLoss();
Kalburator::Shape::LossProfile icsToPalmLoss();

} // namespace WildPalms::CalendarPlugin

#endif // WILDPALMS_CALENDAR_PALMTOICSTRANSFORMATION_H
