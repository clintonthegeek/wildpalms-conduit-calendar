#ifndef WILDPALMS_CALENDAR_CATEGORYAPPINFOREADER_H
#define WILDPALMS_CALENDAR_CATEGORYAPPINFOREADER_H

#include <array>
#include <optional>

#include <QByteArray>
#include <QString>

namespace WildPalms::PalmCalendar {
class CategoryMappingStore;
}

namespace WildPalms::CalendarPlugin {

/**
 * @brief 16 Palm category names parsed from a CategoryAppInfo block.
 *
 * Slot 0 is forced to "Unfiled" if blank; slots 1..15 are returned
 * verbatim (may be empty when the user hasn't named the slot).
 */
struct CategoryNames {
    std::array<QString, 16> names;
};

/**
 * @brief Parse a Datebook AppInfo block into 16 category names.
 *
 * Wraps pisock's `unpack_CategoryAppInfo` (pi-appinfo.h). Returns
 * std::nullopt if `appInfoBytes.size()` < the minimum CategoryAppInfo
 * size or if the underlying unpack fails.
 *
 * Slot 0 is normalised to "Unfiled" when the unpacked name is empty.
 *
 * Pure function — no Qt event loop, no I/O, no global state. Safe to
 * call from any thread.
 */
std::optional<CategoryNames>
parseDatebookAppInfo(const QByteArray &appInfoBytes);

/**
 * @brief Populate `store` with the named slots from `appInfoBytes`.
 *
 * Calls `parseDatebookAppInfo`, then for every non-empty name in
 * slots 1..15 invokes `store.setSlotName(dbName, slot, name)`. Slot 0
 * is intentionally skipped — `CategoryMappingStore` treats slot 0 as
 * implicit "Unfiled".
 *
 * Returns false if parsing failed (store left untouched), true
 * otherwise (even if no slots were named — empty AppInfo is valid).
 */
bool populateFromAppInfo(WildPalms::PalmCalendar::CategoryMappingStore &store,
                         const QString &dbName,
                         const QByteArray &appInfoBytes);

} // namespace WildPalms::CalendarPlugin

#endif // WILDPALMS_CALENDAR_CATEGORYAPPINFOREADER_H
