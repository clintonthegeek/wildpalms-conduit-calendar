#ifndef WILDPALMS_CALENDAR_CALENDARDOMAINEXTENSION_H
#define WILDPALMS_CALENDAR_CALENDARDOMAINEXTENSION_H

#include <shapecontribution.h>

namespace WildPalms::CalendarPlugin {

// O7: contributes the (calendar, palm) peer shape and palm<->ical edges to the
// shape graph. The ical<->canon hop is libkalburator's (CalendarStockShapes).
// PluginManager registers this into the injected ShapeRegistries.
class CalendarPalmShapes : public Kalburator::Shape::ShapeContribution {
public:
    Kalburator::Shape::DomainId targetDomain() const override;
    QList<std::pair<Kalburator::Shape::Shape, Kalburator::Shape::PropertyCatalogue>>
        peerShapes() const override;
    QList<Kalburator::Shape::TransformationEdge> edges() const override;
};

} // namespace WildPalms::CalendarPlugin

#endif // WILDPALMS_CALENDAR_CALENDARDOMAINEXTENSION_H
