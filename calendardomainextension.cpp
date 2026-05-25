#include "calendardomainextension.h"

#include "palmtoicstransformation.h"
#include "propertycatalogue.h"
#include "transformationregistry.h"

using namespace Kalburator::Shape;

namespace WildPalms::CalendarPlugin {

namespace {

PropertyCatalogue makePalmCatalogue()
{
    PropertyCatalogue cat;
    // Palm DatebookDB native fields; used by loss-profile UI to describe a
    // palm-shape record.
    cat.addProperty({ PropertyId{"summary"},    PropertyKind::String,   QStringLiteral("Summary") });
    cat.addProperty({ PropertyId{"note"},       PropertyKind::String,   QStringLiteral("Note") });
    cat.addProperty({ PropertyId{"start"},      PropertyKind::Json,     QStringLiteral("Start") });
    cat.addProperty({ PropertyId{"end"},        PropertyKind::Json,     QStringLiteral("End") });
    cat.addProperty({ PropertyId{"allDay"},     PropertyKind::Boolean,  QStringLiteral("All Day") });
    cat.addProperty({ PropertyId{"recurrence"}, PropertyKind::StringList,QStringLiteral("Recurrence") });
    cat.addProperty({ PropertyId{"alarms"},     PropertyKind::Json,     QStringLiteral("Alarm") });
    cat.addProperty({ PropertyId{"category"},   PropertyKind::Integer,  QStringLiteral("Category Slot") });
    return cat;
}

} // namespace

void CalendarDomainExtension::registerWith(TransformationRegistry &registry)
{
    const Shape palm{ DomainId{"calendar"}, EncodingId{"palm"} };
    const Shape ical{ DomainId{"calendar"}, EncodingId{"ical"} };

    // Defensive: libkalburator's calendar domain plugin registers the ical peer
    // shape at PluginManager load time (via registerStockPlugins). If that
    // static-init registrar didn't run in this address space — e.g. because the
    // calendar stock shapes are in a static lib linked without --whole-archive,
    // or in a unit test that skips the full plugin-init path — the ical shape is
    // absent and registerEdge below would assert "to-shape not registered".
    // Register a minimal placeholder catalogue; registerShape is idempotent, so
    // libkalburator's real catalogue replaces it under the same key when it runs.
    if (registry.catalogueFor(ical) == nullptr) {
        registry.registerShape(ical, {});
    }
    registry.registerShape(palm, makePalmCatalogue());

    // palm -> ical (lossless; identity X- stamps preserved by ical<->canon)
    registry.registerEdge(TransformationEdge{
        palm, ical, palmToIcsLoss(), std::make_shared<PalmToIcsStage>() });

    // ical -> palm (lossy; Palm DatebookDB holds a subset of VEVENT)
    registry.registerEdge(TransformationEdge{
        ical, palm, icsToPalmLoss(), std::make_shared<IcsToPalmStage>() });
}

} // namespace WildPalms::CalendarPlugin
