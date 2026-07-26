// Host-Unit-Test der GNSS-Fix-Bewertung (NFR-TST-03). Laeuft ohne ESP32:
//   pio test -e native
#include <unity.h>
#include "gnss_fix.h"
#include "config.h"

using namespace logic;

void test_all_criteria_met_yields_fix_ok() {
  auto status = gnssFixStatus(true, 0, GNSS_MIN_SATS, GNSS_MIN_CHARS_PROCESSED);
  TEST_ASSERT_EQUAL_INT((int)GnssFixStatus::FIX_OK, (int)status);
}

void test_boundary_sats_and_age_still_fix_ok() {
  // sats genau am Minimum, age knapp unter dem Maximum: beide Grenzen bestanden.
  auto status = gnssFixStatus(true, GNSS_MAX_AGE_MS - 1, GNSS_MIN_SATS,
                               GNSS_MIN_CHARS_PROCESSED);
  TEST_ASSERT_EQUAL_INT((int)GnssFixStatus::FIX_OK, (int)status);
}

void test_too_few_satellites_yields_no_fix() {
  auto status = gnssFixStatus(true, 0, 3, GNSS_MIN_CHARS_PROCESSED);
  TEST_ASSERT_EQUAL_INT((int)GnssFixStatus::NO_FIX, (int)status);
}

void test_stale_fix_yields_no_fix() {
  auto status = gnssFixStatus(true, GNSS_MAX_AGE_MS + 1, GNSS_MIN_SATS,
                               GNSS_MIN_CHARS_PROCESSED);
  TEST_ASSERT_EQUAL_INT((int)GnssFixStatus::NO_FIX, (int)status);
}

void test_invalid_location_yields_no_fix() {
  auto status = gnssFixStatus(false, 0, GNSS_MIN_SATS, GNSS_MIN_CHARS_PROCESSED);
  TEST_ASSERT_EQUAL_INT((int)GnssFixStatus::NO_FIX, (int)status);
}

void test_no_nmea_data_yields_no_data() {
  auto status = gnssFixStatus(false, 0, 0, 0);
  TEST_ASSERT_EQUAL_INT((int)GnssFixStatus::NO_DATA, (int)status);
}

void test_chars_processed_just_below_threshold_yields_no_data() {
  auto status = gnssFixStatus(true, 0, GNSS_MIN_SATS, GNSS_MIN_CHARS_PROCESSED - 1);
  TEST_ASSERT_EQUAL_INT((int)GnssFixStatus::NO_DATA, (int)status);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_all_criteria_met_yields_fix_ok);
  RUN_TEST(test_boundary_sats_and_age_still_fix_ok);
  RUN_TEST(test_too_few_satellites_yields_no_fix);
  RUN_TEST(test_stale_fix_yields_no_fix);
  RUN_TEST(test_invalid_location_yields_no_fix);
  RUN_TEST(test_no_nmea_data_yields_no_data);
  RUN_TEST(test_chars_processed_just_below_threshold_yields_no_data);
  return UNITY_END();
}
