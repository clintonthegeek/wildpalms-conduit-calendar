add_library(wildpalms_calendar_static STATIC
    calendarbackendplugin.cpp      calendarbackendplugin.h
    palmcalendarbackend.cpp        palmcalendarbackend.h
    calendarconflicthandler.cpp    calendarconflicthandler.h
    icstranscoder.cpp              icstranscoder.h
    palmtoicstransformation.cpp    palmtoicstransformation.h
    calendardomainextension.cpp    calendardomainextension.h
    calendarview.cpp               calendarview.h
    hubcalendarreader.cpp          hubcalendarreader.h
)

target_include_directories(wildpalms_calendar_static
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)
# Prepend libkalburator's calendar dir so #include "syncbackend.h"
# resolves to Kalburator::Sync::SyncBackend instead of the legacy
# WildPalms ::Sync::SyncBackend exposed by WildPalmsCore's PUBLIC
# src/sync include. Required because palmcalendarbackend.h includes
# "syncbackend.h" unqualified.
target_include_directories(wildpalms_calendar_static BEFORE
    PRIVATE
        $<TARGET_PROPERTY:Kalburator::Sync,INTERFACE_INCLUDE_DIRECTORIES>
)
set_target_properties(wildpalms_calendar_static PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(wildpalms_calendar_static
    PUBLIC
        Kalburator::Sync
    PRIVATE
        # K.8b T6: WildPalmsCore removed to break CMake autogen cycle
        # (WildPalmsCore→plugin_static→WildPalmsCore). Symbols resolved
        # when this static is pulled into WildPalmsCore.so at its link step.
        # Include path (${CMAKE_SOURCE_DIR}/src above) covers all headers.
        PalmDeviceAccessLib
        WildPalmsPalmSync
        WildPalmsPalmCalendar
        WildPalmsPalmCodecs
        WildPalmsPalmConflict
        KF6::CalendarCore
        KF6::I18n
        KF6::WidgetsAddons
        Qt::Widgets
)
