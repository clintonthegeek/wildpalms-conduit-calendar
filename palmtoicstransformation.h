#ifndef WILDPALMS_CALENDAR_PALMTOICSTRANSFORMATION_H
#define WILDPALMS_CALENDAR_PALMTOICSTRANSFORMATION_H

#include "transformationedge.h"

namespace WildPalms::PalmCalendar { class CategoryMappingStore; }

namespace WildPalms::CalendarPlugin {

// (calendar, palm) -> (calendar, ical): wraps DatebookCodec via icstranscoder.
// Lossless: a Palm appointment is a subset of a VEVENT (identity X- stamps
// are preserved downstream by libkalburator's ical<->canon stage).
// Borrows a (nullable) CategoryMappingStore to carry the Palm category slot
// into the iCalendar CATEGORIES property. The store must outlive the stage.
class PalmToIcsStage : public Kalburator::Shape::TransformationStage {
public:
    explicit PalmToIcsStage(const WildPalms::PalmCalendar::CategoryMappingStore *cats);
    QByteArray transform(const QByteArray &sourceBytes) const override;

private:
    const WildPalms::PalmCalendar::CategoryMappingStore *m_cats = nullptr;
};

// (calendar, ical) -> (calendar, palm): lossy; Palm DatebookDB cannot hold
// most VEVENT fields. Borrows a (nullable) CategoryMappingStore to map the
// iCalendar CATEGORIES name back to a Palm category slot.
class IcsToPalmStage : public Kalburator::Shape::TransformationStage {
public:
    explicit IcsToPalmStage(const WildPalms::PalmCalendar::CategoryMappingStore *cats);
    QByteArray transform(const QByteArray &sourceBytes) const override;

private:
    const WildPalms::PalmCalendar::CategoryMappingStore *m_cats = nullptr;
};

Kalburator::Shape::LossProfile palmToIcsLoss();
Kalburator::Shape::LossProfile icsToPalmLoss();

} // namespace WildPalms::CalendarPlugin

#endif // WILDPALMS_CALENDAR_PALMTOICSTRANSFORMATION_H
