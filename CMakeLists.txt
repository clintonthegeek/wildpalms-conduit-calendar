kcoreaddons_add_plugin(wildpalms_calendar
    SOURCES
        calendarconduit.cpp
        calendarconduit.h
        calendarmapper.cpp
        calendarmapper.h
        calendarview.cpp
        calendarview.h
    INSTALL_NAMESPACE "wildpalms/conduits"
)

target_link_libraries(wildpalms_calendar
    WildPalmsCore
    KF6::CoreAddons
    KF6::CalendarCore
    KF6::I18n
    KF6::WidgetsAddons
    Qt::Widgets
)
