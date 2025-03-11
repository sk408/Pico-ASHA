#pragma once

#include <cstdint>
#include <etl/string.h>
#include <btstack.h>

#include "hearing_aid.hpp"
#include "asha_logging.h"

namespace asha
{

// Structure to track test results for a specific device
struct DeviceTestResult {
    bd_addr_t addr;
    etl::string<32> device_name;
    etl::string<32> manufacturer;
    etl::string<32> model;
    uint16_t manufacturer_id;

    bool connection_successful;
    bool service_discovery_successful;
    bool read_rop_successful;
    bool read_psm_successful;
    bool l2cap_connection_successful;
    bool streaming_successful;

    // Detailed error information
    etl::string<64> error_reason;
    uint8_t error_status;
    uint8_t l2cap_error_reason;

    // Initialize to default values
    DeviceTestResult() :
        addr{},
        device_name{},
        manufacturer{},
        model{},
        manufacturer_id(0),
        connection_successful(false),
        service_discovery_successful(false),
        read_rop_successful(false),
        read_psm_successful(false),
        l2cap_connection_successful(false),
        streaming_successful(false),
        error_reason{},
        error_status(0),
        l2cap_error_reason(0)
    {}
};

// Class for handling compatibility testing
class CompatibilityTester {
public:
    // Start a comprehensive test on a connected device
    static void start_test(HearingAid* ha);
    
    // Print the results of a test run
    static void print_test_results(const DeviceTestResult& result);
    
    // Save test results to flash for future reference
    static void save_test_results(const DeviceTestResult& result);
    
    // Check a device against known compatibility issues based on manufacturer ID
    static bool check_known_issues(uint16_t manufacturer_id, etl::string<64>& issue_description);
    
    // Get recommended fix for a specific device based on its test results
    static void get_recommended_fix(const DeviceTestResult& result, etl::string<128>& recommendation);

private:
    // Private test methods
    static void test_service_discovery(HearingAid* ha);
    static void test_rop_read(HearingAid* ha);
    static void test_psm_read(HearingAid* ha);
    static void test_l2cap_connection(HearingAid* ha);
    static void test_audio_streaming(HearingAid* ha);
    
    // Maintain a list of test results for different devices
    inline static etl::vector<DeviceTestResult, 10> test_results;
};

// Enable compatibility test mode - logs more details about the connection process
extern bool compatibility_test_mode;

// Set to true to enable automatic testing of connected devices
void enable_compatibility_testing(bool enable);

// Check if compatibility testing is enabled
bool is_compatibility_testing_enabled();

} // namespace asha 