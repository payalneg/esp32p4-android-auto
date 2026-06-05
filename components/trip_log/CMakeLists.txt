# Raw circular trip time-series log on the "triplog" flash partition.
# Lives in its own component (not main/) so the trip-statistics UI in the
# vesc_ui component can call the reader API without a main<-vesc_ui cycle.
#
# REQUIRES vesc_can is PUBLIC because trip_log.h includes vesc_datatypes.h
# (vesc_setup_values_t in trip_log_tick) — it propagates transitively to
# anyone that requires trip_log (e.g. vesc_ui).
idf_component_register(
    SRCS "trip_log.c"
    INCLUDE_DIRS "include"
    REQUIRES vesc_can
    PRIV_REQUIRES esp_partition esp_timer freertos dev_settings nvs_flash
)
