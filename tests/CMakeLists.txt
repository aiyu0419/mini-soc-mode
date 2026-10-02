add_executable(test_memory
    test_memory.cpp
)

target_link_libraries(test_memory
    PRIVATE
        soc_model
)

add_test(
    NAME MemoryTest
    COMMAND test_memory
)
add_executable(test_interconnect_memory
    test_interconnect_memory.cpp
)

target_link_libraries(test_interconnect_memory
    PRIVATE soc_model
)

add_test(
    NAME InterconnectTest
    COMMAND test_interconnect_memory
)
add_executable(test_interconnect_dma_memory
    test_interconnect_dma_memory.cpp
)

target_link_libraries(test_interconnect_dma_memory
    PRIVATE soc_model
)

add_test(
    NAME DmaTest
    COMMAND test_interconnect_dma_memory
)
add_executable(test_accelerator
    test_accelerator.cpp
)

target_link_libraries(test_accelerator
    PRIVATE soc_model
)

add_test(
    NAME AcceleratorTest
    COMMAND test_accelerator
)

add_executable(test_soc
    test_soc.cpp
)

target_link_libraries(test_soc
    PRIVATE soc_model
)

add_test(
    NAME SocTest
    COMMAND test_soc
)
