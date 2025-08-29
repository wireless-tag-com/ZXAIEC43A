set(requires nvs_flash)

idf_component_register(
	SRC_DIRS .
    INCLUDE_DIRS .
    REQUIRES ${requires}
)
