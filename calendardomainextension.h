#ifndef WILDPALMS_CALENDAR_CALENDARDOMAINEXTENSION_H
#define WILDPALMS_CALENDAR_CALENDARDOMAINEXTENSION_H

#include <shapecontribution.h>

namespace WildPalms::PalmCalendar { class CategoryMappingStore; }

namespace WildPalms::CalendarPlugin {

// O7: contributes the (calendar, palm) peer shape and palm<->ical edges to the
// shape graph. The ical<->canon hop is libkalburator's (CalendarStockShapes).
// PluginManager registers this into the injected ShapeRegistries.
//
// Borrows a (nullable) CategoryMappingStore, threaded into the palm<->ical
// stages so the Palm category slot rides as the canonical `categories` field.
// The store is owned by the plugin and outlives this contribution.
class CalendarPalmShapes : public Kalburator::Shape::ShapeContribution {
public:
    explicit CalendarPalmShapes(const WildPalms::PalmCalendar::CategoryMappingStore *cats);

    Kalburator::Shape::DomainId targetDomain() const override;
    QList<std::pair<Kalburator::Shape::Shape, Kalburator::Shape::PropertyCatalogue>>
        peerShapes() const override;
    QList<Kalburator::Shape::TransformationEdge> edges() const override;

private:
    const WildPalms::PalmCalendar::CategoryMappingStore *m_cats = nullptr;
};

} // namespace WildPalms::CalendarPlugin

#endif // WILDPALMS_CALENDAR_CALENDARDOMAINEXTENSION_H
