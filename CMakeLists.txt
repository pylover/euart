set(sources
  "euart.c"
)


idf_component_register(
  SRCS "${sources}"
  INCLUDE_DIRS "include"
  REQUIRES esp_driver_uart esp_hw_support uaio ering
)


target_compile_options(${COMPONENT_LIB} PRIVATE -fms-extensions)
