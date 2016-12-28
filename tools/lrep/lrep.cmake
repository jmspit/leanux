set( LREP_DESCR "generate HTML reports with javascript Google charts from a lard database" )

# generate configure header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lrep/lrep-config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/lrep-config.hpp @ONLY)
