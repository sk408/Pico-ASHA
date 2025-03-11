#include "asha_test_util.hpp"
#include "bt_status_err.hpp"
#include <pico/time.h>

namespace asha
{

bool compatibility_test_mode = false;
static bool compatibility_testing_enabled = false;

void enable_compatibility_testing(bool enable) {
    compatibility_testing_enabled = enable;
    if (enable) {
        LOG_INFO("Compatibility testing mode enabled");
    } else {
        LOG_INFO("Compatibility testing mode disabled");
    }
}

bool is_compatibility_testing_enabled() {
    return compatibility_testing_enabled;
}

void CompatibilityTester::start_test(HearingAid* ha) {
    if (!ha || !ha->is_connected()) {
        LOG_ERROR("Cannot start test: no hearing aid connected");
        return;
    }

    LOG_INFO("%s: Starting compatibility test for device", ha->get_side_str());
    
    DeviceTestResult result;
    
    // Copy basic device info
    bd_addr_copy(result.addr, ha->addr);
    result.device_name = ha->device_name;
    result.manufacturer = ha->manufacturer;
    result.model = ha->model;
    result.manufacturer_id = ha->rop.id.manufacturer_id;
    
    // Connection is successful if we got this far
    result.connection_successful = true;
    
    // Log manufacturer-specific information
    LOG_COMPAT("%s: Testing device from %s (ID: 0x%04X)", ha->get_side_str(), 
               get_manufacturer_name(result.manufacturer_id), result.manufacturer_id);

    // Check for known issues with this manufacturer
    etl::string<64> issues;
    if (check_known_issues(result.manufacturer_id, issues)) {
        LOG_COMPAT("%s: Manufacturer has known issues: %s", ha->get_side_str(), issues.c_str());
    }
    
    // Run individual tests
    test_service_discovery(ha);
    test_rop_read(ha);
    test_psm_read(ha);
    test_l2cap_connection(ha);
    test_audio_streaming(ha);
    
    // Save the test results
    test_results.push_back(result);
    
    // Print results
    print_test_results(result);
    
    // Get recommended fix
    etl::string<128> recommendation;
    get_recommended_fix(result, recommendation);
    LOG_COMPAT("%s: Recommended fix: %s", ha->get_side_str(), recommendation.c_str());
}

void CompatibilityTester::print_test_results(const DeviceTestResult& result) {
    LOG_COMPAT("Test results for %s:", bd_addr_to_str(result.addr));
    LOG_COMPAT("  Device name: %s", result.device_name.c_str());
    LOG_COMPAT("  Manufacturer: %s (ID: 0x%04X)", result.manufacturer.c_str(), result.manufacturer_id);
    LOG_COMPAT("  Model: %s", result.model.c_str());
    LOG_COMPAT("  Connection successful: %s", result.connection_successful ? "YES" : "NO");
    LOG_COMPAT("  Service discovery successful: %s", result.service_discovery_successful ? "YES" : "NO");
    LOG_COMPAT("  ROP read successful: %s", result.read_rop_successful ? "YES" : "NO");
    LOG_COMPAT("  PSM read successful: %s", result.read_psm_successful ? "YES" : "NO");
    LOG_COMPAT("  L2CAP connection successful: %s", result.l2cap_connection_successful ? "YES" : "NO");
    LOG_COMPAT("  Streaming successful: %s", result.streaming_successful ? "YES" : "NO");
    
    if (!result.error_reason.empty()) {
        LOG_COMPAT("  Error reason: %s (Status: 0x%02X)", 
                    result.error_reason.c_str(), result.error_status);
    }
}

void CompatibilityTester::save_test_results(const DeviceTestResult& result) {
    // In a real implementation, this would save results to flash memory
    // For now, just log
    LOG_COMPAT("Saved test results for %s (%s)", 
                result.device_name.c_str(), bd_addr_to_str(result.addr));
}

bool CompatibilityTester::check_known_issues(uint16_t manufacturer_id, etl::string<64>& issue_description) {
    bool has_issues = has_known_issues(manufacturer_id);
    if (has_issues) {
        issue_description = get_known_issues_description(manufacturer_id);
    }
    return has_issues;
}

void CompatibilityTester::get_recommended_fix(const DeviceTestResult& result, etl::string<128>& recommendation) {
    // Base recommendation on which stage failed
    if (!result.service_discovery_successful) {
        recommendation = "Check if device properly advertises ASHA service. May need modified service discovery.";
    } else if (!result.read_rop_successful) {
        recommendation = "ROP format may be non-standard. Try flexible ROP interpretation.";
    } else if (!result.read_psm_successful) {
        recommendation = "PSM characteristic may not be standard. Try default PSM value (25).";
    } else if (!result.l2cap_connection_successful) {
        // Different recommendations based on L2CAP error reason
        switch (result.l2cap_error_reason) {
            case L2CAP_CONNECTION_RESPONSE_RESULT_REFUSED_PSM:
                recommendation = "PSM value refused. Try different PSM or disable L2CAP CoC.";
                break;
            case L2CAP_CONNECTION_RESPONSE_RESULT_REFUSED_SECURITY:
                recommendation = "Security requirements not met. Try different security level.";
                break;
            case L2CAP_CONNECTION_RESPONSE_RESULT_REFUSED_RESOURCES:
                recommendation = "Resource limitations. Try smaller data packets or retry later.";
                break;
            default:
                recommendation = "General L2CAP issue. Try different connection parameters.";
                break;
        }
    } else if (!result.streaming_successful) {
        recommendation = "Streaming protocol may differ. Check codec support and data packet format.";
    } else {
        recommendation = "Device appears compatible. Minor optimizations may improve performance.";
    }
}

void CompatibilityTester::test_service_discovery(HearingAid* ha) {
    // This would be used for real-time testing
    // For this implementation, we'll consider service discovery successful if ASHA service was found
    
    DeviceTestResult& result = test_results.back();
    result.service_discovery_successful = gatt_service_valid(&ha->services.asha.service);
    
    LOG_COMPAT("%s: Service discovery %s", ha->get_side_str(), 
                result.service_discovery_successful ? "SUCCESSFUL" : "FAILED");
    
    if (!result.service_discovery_successful) {
        result.error_reason = "ASHA service not found or invalid";
    }
}

void CompatibilityTester::test_rop_read(HearingAid* ha) {
    // Consider ROP read successful if side has been set
    DeviceTestResult& result = test_results.back();
    result.read_rop_successful = (ha->rop.side != Side::Unset);
    
    LOG_COMPAT("%s: ROP read %s", ha->get_side_str(), 
                result.read_rop_successful ? "SUCCESSFUL" : "FAILED");
    
    if (!result.read_rop_successful) {
        result.error_reason = "Unable to read or parse ROP characteristic";
    }
}

void CompatibilityTester::test_psm_read(HearingAid* ha) {
    // Consider PSM read successful if PSM value is non-zero
    DeviceTestResult& result = test_results.back();
    result.read_psm_successful = (ha->psm > 0);
    
    LOG_COMPAT("%s: PSM read %s (PSM value: %d)", ha->get_side_str(), 
                result.read_psm_successful ? "SUCCESSFUL" : "FAILED", ha->psm);
    
    if (!result.read_psm_successful) {
        result.error_reason = "Unable to read PSM characteristic or value is 0";
    }
}

void CompatibilityTester::test_l2cap_connection(HearingAid* ha) {
    // Consider L2CAP connection successful if CID is non-zero
    DeviceTestResult& result = test_results.back();
    result.l2cap_connection_successful = (ha->cid > 0);
    
    LOG_COMPAT("%s: L2CAP connection %s (CID: %d)", ha->get_side_str(), 
                result.l2cap_connection_successful ? "SUCCESSFUL" : "FAILED", ha->cid);
    
    if (!result.l2cap_connection_successful) {
        result.error_reason = "Failed to establish L2CAP connection";
        // In real implementation, would capture the actual error reason
    }
}

void CompatibilityTester::test_audio_streaming(HearingAid* ha) {
    // Consider streaming successful if we've entered the streaming state at any point
    DeviceTestResult& result = test_results.back();
    result.streaming_successful = ha->is_streaming();
    
    LOG_COMPAT("%s: Audio streaming %s", ha->get_side_str(), 
                result.streaming_successful ? "SUCCESSFUL" : "FAILED");
    
    if (!result.streaming_successful) {
        result.error_reason = "Unable to establish audio streaming";
    }
}

} // namespace asha 