kcoreaddons_add_plugin(wildpalms_calendar_v2
    SOURCES
        calendarbackendplugin.cpp   calendarbackendplugin.h
        calendarblobbackend.cpp     calendarblobbackend.h
        calendarconflicthandler.cpp calendarconflicthandler.h
        icstranscoder.cpp           icstranscoder.h
        calendarview.cpp            calendarview.h
    INSTALL_NAMESPACE "wildpalms/plugins"
)
target_include_directories(wildpalms_calendar_v2
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)
# Prepend libkalburator's calendar dir so #include "syncbackend.h"
# resolves to Kalburator::Sync::SyncBackend instead of the legacy
# WildPalms ::Sync::SyncBackend exposed by WildPalmsCore's PUBLIC
# src/sync include. Required because palmcalendarbackend.h includes
# "syncbackend.h" unqualified.
target_include_directories(wildpalms_calendar_v2 BEFORE
    PRIVATE
        $<TARGET_PROPERTY:Kalburator::Sync,INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries(wildpalms_calendar_v2
    PRIVATE
        WildPalmsCore
        WildPalmsPalmSync
        WildPalmsPalmCalendar
        WildPalmsPalmCodecs
        WildPalmsPalmConflict
        KF6::CoreAddons
        KF6::CalendarCore
        KF6::I18n
        KF6::WidgetsAddons
        Qt::Widgets
        Kalburator::Sync
)
