check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(examples_as_tests_sources)
if(${ENABLE_EXAMPLES})
    set(examples_as_tests_sources
        #test/ris-module-examples-test-suite.cc
        )
endif()

build_lib(
    LIBNAME ris-module
    SOURCE_FILES model/ris-module.cc
                 helper/ris-module-helper.cc
    HEADER_FILES model/ris-module.h
                 helper/ris-module-helper.h
    LIBRARIES_TO_LINK ${libcore}
    TEST_SOURCES test/ris-module-test-suite.cc
                 ${examples_as_tests_sources}
)
