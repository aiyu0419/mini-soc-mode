#include <systemc>

#include <iostream>

class TestModule : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(TestModule);

    explicit TestModule(sc_core::sc_module_name name)
        : sc_core::sc_module(name) {
        SC_THREAD(run);
    }

private:
    void run() {
        std::cout
            << "[" << sc_core::sc_time_stamp() << "] "
            << "Simulation started\n";

        wait(10, sc_core::SC_NS);

        std::cout
            << "[" << sc_core::sc_time_stamp() << "] "
            << "Simulation finished\n";
    }
};

int sc_main(int argc, char* argv[]) {
    static_cast<void>(argc);
    static_cast<void>(argv);

    TestModule test_module{"test_module"};

    sc_core::sc_start();

    return 0;
}