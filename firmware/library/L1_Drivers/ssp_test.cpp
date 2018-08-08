// this is the ssp test file

#include <assert.h>
#include <cstdio>
#include "L0_LowLevel/LPC40xx.h"
#include "L1_Drivers/pin_configure.hpp"
#include "L1_Drivers/ssp.hpp"
#include "L5_Testing/testing_frameworks.hpp"

TEST_CASE("Testing SSP dependency injection", "[ssp]")
{
    // Simulate local version of LPC_SSP
    LPC_SSP_TypeDef local_ssp[2] =
    {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    };

    // Only SSP0 will be used in this unit testing
    Ssp::ssp_registers[0] = &local_ssp[0];

    // Simulate local version of LPC_IOCON
    LPC_IOCON_TypeDef local_iocon;

    // Redirect LPC_IOCON mem map to local_iocon structure
    PinConfigure::pin_map =
        reinterpret_cast<PinConfigure::PinMap_t *>(&local_iocon);

    // Set up Mock for PinCongiure
    fakeit::Mock<PinConfigureInterface> mock_mosi;
    fakeit::Mock<PinConfigureInterface> mock_miso;
    fakeit::Mock<PinConfigureInterface> mock_sck;

    fakeit::Fake(Method(mock_mosi, SetPinFunction));
    fakeit::Fake(Method(mock_miso, SetPinFunction));
    fakeit::Fake(Method(mock_sck, SetPinFunction));

    PinConfigureInterface &mosi = mock_mosi.get();
    PinConfigureInterface &miso = mock_miso.get();
    PinConfigureInterface &sck = mock_sck.get();

    Ssp test_spi(SspInterface::Peripheral::kSsp0, mosi, miso, sck);

    SECTION("Test Peripheral Parameters")
    {
    }
    SECTION("Test Pin Numbers")
    {
        fakeit::Verify(Method(
            mock_mosi, SetPinFunction).Using(Ssp::kEnableSspPins));
        fakeit::Verify(Method(
            mock_miso, SetPinFunction).Using(Ssp::kEnableSspPins));
        fakeit::Verify(Method(
            mock_sck, SetPinFunction).Using(Ssp::kEnableSspPins));
    }
}
