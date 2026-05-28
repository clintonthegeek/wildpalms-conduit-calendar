#include "calendardomainextension.h"

#include "palmtoicstransformation.h"
#include <propertycatalogue.h>

using namespace Kalburator::Shape;

namespace WildPalms::CalendarPlugin {

namespace {

PropertyCatalogue makePalmCatalogue()
{
    PropertyCatalogue cat;
    // Palm DatebookDB native fields; used by loss-profile UI to describe a
    // palm-shape record.
    cat.addProperty({ PropertyId{"summary"},    PropertyKind::String,    QStringLiteral("Summary") });
    cat.addProperty({ PropertyId{"note"},       PropertyKind::String,    QStringLiteral("Note") });
    cat.addProperty({ PropertyId{"start"},      PropertyKind::Json,      QStringLiteral("Start") });
    cat.addProperty({ PropertyId{"end"},        PropertyKind::Json,      QStringLiteral("End") });
    cat.addProperty({ PropertyId{"allDay"},     PropertyKind::Boolean,   QStringLiteral("All Day") });
    cat.addProperty({ PropertyId{"recurrence"}, PropertyKind::StringList,QStringLiteral("Recurrence") });
    cat.addProperty({ PropertyId{"alarms"},     PropertyKind::Json,      QStringLiteral("Alarm") });
    cat.addProperty({ PropertyId{"category"},   PropertyKind::Integer,   QStringLiteral("Category Slot") });
    return cat;
}

} // namespace

CalendarPalmShapes::CalendarPalmShapes(
    const WildPalms::PalmCalendar::CategoryMappingStore *cats)
    : m_cats(cats)
{
}

DomainId CalendarPalmShapes::targetDomain() const
{
    return DomainId{QStringLiteral("calendar")};
}

QList<std::pair<Shape, PropertyCatalogue>> CalendarPalmShapes::peerShapes() const
{
    const Shape palm{ DomainId{"calendar"}, EncodingId{"palm"} };
    return { { palm, makePalmCatalogue() } };
}

QList<TransformationEdge> CalendarPalmShapes::edges() const
{
    const Shape palm{ DomainId{"calendar"}, EncodingId{"palm"} };
    const Shape ical{ DomainId{"calendar"}, EncodingId{"ical"} };
    // palm -> ical (lossless; identity X- stamps preserved by ical<->canon).
    // ical -> palm (lossy; Palm DatebookDB holds a subset of VEVENT).
    // The ical endpoint is registered by libkalburator's CalendarStockShapes,
    // which loads earlier in the same PluginManager batch.
    return {
        TransformationEdge{ palm, ical, palmToIcsLoss(), std::make_shared<PalmToIcsStage>(m_cats) },
        TransformationEdge{ ical, palm, icsToPalmLoss(), std::make_shared<IcsToPalmStage>(m_cats) },
    };
}

} // namespace WildPalms::CalendarPlugin
