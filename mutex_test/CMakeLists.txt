project (optee_example_cl_mutex_test C)

set (SRC host/main.c)

add_executable (${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME}
			   PRIVATE ta/include
			   PRIVATE include)

target_link_libraries (${PROJECT_NAME} PRIVATE teec -lpthread)

install (TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
