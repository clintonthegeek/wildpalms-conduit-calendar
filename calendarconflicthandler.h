#ifndef WILDPALMS_CALENDAR_CALENDARCONFLICTHANDLER_H
#define WILDPALMS_CALENDAR_CALENDARCONFLICTHANDLER_H

#include "conflictpolicy.h"   // brings in ConflictHandler base

#include <memory>

namespace WildPalms::PalmSync { class IPalmDatabaseAccess; }
namespace WildPalms::PalmConflict {
class PalmConflictHandler;
struct PalmBackendConfig;
}

namespace WildPalms::CalendarPlugin {

/**
 * @brief ConflictHandler with calendar-aware overlays + Palm delegation.
 *
 * Resolution order:
 *   1. Decode both sides as KCalendarCore::Event::Ptr from iCal bytes.
 *      If either side fails to decode, delegate straight to the inner
 *      PalmConflictHandler (which will fall through to base
 *      ConflictPolicy resolution).
 *   2. Detect calendar-specific shapes:
 *        a. **Alarm-only diff** -> ConflictDecision::Merge with merged
 *           alarms (mergedContent = re-serialised iCal with the union
 *           of both sides' alarms).
 *        b. **EXDATE-only diff** -> Merge with EXDATE-list union.
 *        c. **DTSTART tz-only** with one floating + one zoned -> Merge
 *           preferring the floating side (Palm semantics).
 *   3. Otherwise -> delegate to PalmConflictHandler::handleConflict.
 *
 * Owns its inner PalmConflictHandler — constructed from the
 * (device, config) pair the plugin passes through.
 *
 * Lifetime: does NOT own device or config (forwarded to the inner
 * PalmConflictHandler which also borrows). Both must outlive this
 * handler.
 */
class CalendarConflictHandler : public Kalburator::Conflict::ConflictHandler
{
public:
    CalendarConflictHandler(WildPalms::PalmSync::IPalmDatabaseAccess *device,
                            const WildPalms::PalmConflict::PalmBackendConfig *config);
    ~CalendarConflictHandler() override;

    Kalburator::Conflict::ConflictDecision handleConflict(
        Kalburator::Conflict::ConflictRecord &conflict,
        const Kalburator::Conflict::ConflictPolicy &policy) override;

    bool canPrompt() const override { return false; }

    /// Test hook: which overlay (if any) was last applied.
    /// Values: "", "alarm", "exdate", "tz", "delegated".
    const QString &lastOverlay() const { return m_lastOverlay; }

private:
    std::unique_ptr<WildPalms::PalmConflict::PalmConflictHandler> m_palm;
    QString m_lastOverlay;
};

} // namespace WildPalms::CalendarPlugin

#endif // WILDPALMS_CALENDAR_CALENDARCONFLICTHANDLER_H
