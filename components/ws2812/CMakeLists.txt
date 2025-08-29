set(requires esp_driver_rmt qmsd_network)

idf_component_register(
	SRC_DIRS .
    INCLUDE_DIRS .
    REQUIRES ${requires}
)
