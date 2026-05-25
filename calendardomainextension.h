#ifndef WILDPALMS_CALENDAR_CALENDARDOMAINEXTENSION_H
#define WILDPALMS_CALENDAR_CALENDARDOMAINEXTENSION_H

namespace Kalburator::Shape { class TransformationRegistry; }

namespace WildPalms::CalendarPlugin {

// Registers the (calendar, palm) peer shape and palm<->ical edges with the
// shape graph. The ical<->canon hop is libkalburator's (CalendarStockShapes).
class CalendarDomainExtension {
public:
    static void registerWith(Kalburator::Shape::TransformationRegistry &registry);
};

} // namespace WildPalms::CalendarPlugin

#endif // WILDPALMS_CALENDAR_CALENDARDOMAINEXTENSION_H
