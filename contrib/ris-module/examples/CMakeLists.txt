build_lib_example(
    NAME ris-module-example
    SOURCE_FILES ris-module-example.cc
    LIBRARIES_TO_LINK ${libris-module}
                      ${libcore}         # For core ns-3 functionality
                      ${libnetwork}      # For network-related functionality
                      ${libinternet}     # For Internet-related functionality
                      ${libwifi}         # For Wi-Fi related functionality
)
