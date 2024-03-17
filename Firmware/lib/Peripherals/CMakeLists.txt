
# Libarary name - Try and keep the same as directory name
project(Peripherals)

# source files for this libarary
add_library(${PROJECT_NAME}
   flashPicoLED.c
   i2cPico.c
   i2cPicoBusScan.c
   BH1730.c
   ads1x1x.c
   Average.c
   CIE1931.c
   )

# Add any extra libraries
target_link_libraries(${PROJECT_NAME} 
   pico_stdlib
   hardware_i2c
   )

# Allow this library and the parent add_subdirectory access to headder files
target_include_directories(${PROJECT_NAME}  
   PUBLIC ${PROJECT_SOURCE_DIR}/include
   )
