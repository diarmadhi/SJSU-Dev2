// SSP provides the ability for serial communication over SPI, SSI, or
// Microwire on the LPC407x chipset. NOTE: The SSP2 peripheral is not
// available on the SJTwo board, as such this driver does not support
// it. Only one set of pins are available for either SSP0 or SSP1 as
// follows:
//      Peripheral  |   MISO    |   MOSI    |   SCK
//          SSP0    |   P0.17   |   P0.18   |   P0.15
//          SSP1    |   P0.8    |   P0.9    |   P0.7
#pragma once

#include <cstdint>
#include "L0_LowLevel/LPC40xx.h"
#include "L1_Drivers/pin_configure.hpp"

class SspInterface
{
 public:
     // SSP peripherals
    enum class Peripheral
    {
        kSsp0 = 0,
        kSsp1 = 1
    };

    enum PowerOn
    {
        kPconp0 = 21,
        kPconp1 = 10
    };

    // SSP frame formats
    enum FrameMode
    {
        kSpi     = 0,
        kTi      = 1,
        kMicro   = 2
    };

    // SSP Master/slave modes
    enum MasterSlaveMode
    {
        kMaster  = 0,
        kSlave   = 1
    };

    // SSP data size for frame packets
    enum DataSize
    {
        kFour        = 0b0011,  // 4-bit  transfer
        kFive        = 0b0100,  // 5-bit  transfer
        kSix         = 0b0101,  // 6-bit  transfer
        kSeven       = 0b0110,  // 7-bit  transfer
        kEight       = 0b0111,  // 8-bit  transfer
        kNine        = 0b1000,  // 9-bit  transfer
        kTen         = 0b1001,  // 10-bit transfer
        kEleven      = 0b1010,  // 11-bit transfer
        kTwelve      = 0b1011,  // 12-bit transfer
        kThirteen    = 0b1100,  // 13-bit transfer
        kFourteen    = 0b1101,  // 14-bit transfer
        kFifteen     = 0b1110,  // 15-bit transfer
        kSixteen     = 0b1111,  // 16-bit transfer
    };

    virtual bool Initialize()                                       = 0;
    virtual bool GetTransferStatus()                                = 0;
    virtual uint16_t Transfer(uint16_t data)                        = 0;
    virtual void SetPeripheralPower()                               = 0;
    virtual void SetPeripheralMode(
            MasterSlaveMode mode, FrameMode frame, DataSize size)   = 0;
    // NOTE: "divider" is the number of prescaler-output clocks per bit.
    // See page 611 of UM10562 for SCR-Serial Clock Rate for more info.
    virtual void SetClock(bool polarity, bool phase,
            uint8_t prescaler, uint8_t divider)                     = 0;
};

class Ssp : public SspInterface
{
 public:
    // SSP register lookup table
    static LPC_SSP_TypeDef * ssp_registers[2];
    static constexpr uint8_t kEnableSspPins = 0b010;

    // Default constructor sets up SSP0 peripheral as SPI master
    constexpr Ssp() : mosi_(mosi_pin_),
        miso_(miso_pin_),
        sck_(sck_pin_),

        mosi_pin_(0, 18),   // P0.18
        miso_pin_(0, 17),   // P0.17
        sck_pin_(0, 15),    // P0.15

        pssp_(Peripheral::kSsp0), pconp_bits_(kPconp0), master_mode_(kMaster),
        data_size_(kEight), frame_format_(kSpi), clock_polarity_(1),
        clock_phase_(0), clock_divider_(0), clock_prescaler_(2)
    {
    }

    // User modified constructor. **MUST** be followed by the setter functions
    // for Power, Clock, and Frame format found above.
    explicit constexpr Ssp(Peripheral set_pssp) :
        mosi_(mosi_pin_),
        miso_(miso_pin_),
        sck_(sck_pin_),

        mosi_pin_(0, ((set_pssp == Peripheral::kSsp0) ? 18 : 9)),
        miso_pin_(0, ((set_pssp == Peripheral::kSsp0) ? 17 : 8)),
        sck_pin_(0, ((set_pssp == Peripheral::kSsp0) ? 15 : 7)),

        pssp_(set_pssp), pconp_bits_(static_cast<PowerOn>(0)),
        master_mode_(static_cast<MasterSlaveMode>(0)),
        data_size_(static_cast<DataSize>(0)),
        frame_format_(static_cast<FrameMode>(0)), clock_polarity_(0),
        clock_phase_(0), clock_divider_(0), clock_prescaler_(0)
    {
    }

    // Unit-testing constructor
    constexpr Ssp(Peripheral pssp,

        PinConfigureInterface & mosi_pin,
        PinConfigureInterface & miso_pin,
        PinConfigureInterface & sck_pin) :

        mosi_(mosi_pin),
        miso_(miso_pin),
        sck_(sck_pin),

        mosi_pin_(PinConfigure::CreateInactivePin()),
        miso_pin_(PinConfigure::CreateInactivePin()),
        sck_pin_(PinConfigure::CreateInactivePin()),

        pssp_(pssp), pconp_bits_(kPconp0), master_mode_(kMaster),
        data_size_(kEight), frame_format_(kSpi), clock_polarity_(1),
        clock_phase_(0), clock_divider_(0), clock_prescaler_(2)
    {
    }

    // Powers on the desired peripheral and sets the peripheral clock. Also,
    // enables the SSP data lines, format, data size, and more. See page 601
    // of user manual UM10562 LPC408x/407x for more details.
    bool Initialize() override
    {
        bool peripheral_initialized;

        if (pssp_ > Peripheral::kSsp1)
        {
            peripheral_initialized = 0;
        }
        else
        {
            // if frame mode is set for Microwire, force data_size_ = 8 bits
            DataSize check_data_size = (frame_format_ == kMicro)
                                        ? kEight : data_size_;

            // Power up peripheral and set Pclk
            LPC_SC->PCONP &= ~(1 << pconp_bits_);
            LPC_SC->PCONP |= (1 << pconp_bits_);
            LPC_SC->PCLKSEL &= ~(0x1F << 0);
            LPC_SC->PCLKSEL = 1;
            ssp_registers[static_cast<uint32_t>(pssp_)]->CPSR &= ~(0xFF);
            ssp_registers[static_cast<uint32_t>(pssp_)]->CPSR |=
                (clock_prescaler_);

            // Enable SSP pins
            mosi_.SetPinFunction(kEnableSspPins);
            miso_.SetPinFunction(kEnableSspPins);
            sck_.SetPinFunction(kEnableSspPins);

            // Clear and Set Control Register values
            ssp_registers[static_cast<uint32_t>(pssp_)]->CR0 &=
                ~((0xF << 0) | (0x3 << 4) | (0x3 << 6) | (0xFF << 8));
            ssp_registers[static_cast<uint32_t>(pssp_)]->CR0 |=
                (check_data_size << 0) | (frame_format_ << 4);

            // set clk polarity = 1, clk phase set to 0
            ssp_registers[static_cast<uint32_t>(pssp_)]->CR0 |=
                (clock_polarity_ << 6) | (clock_phase_ << 7);
            ssp_registers[static_cast<uint32_t>(pssp_)]->CR0 |=
                (clock_divider_ << 8);
            ssp_registers[static_cast<uint32_t>(pssp_)]->CR1 &= ~(1 << 2);
            ssp_registers[static_cast<uint32_t>(pssp_)]->CR1 |=
                (master_mode_ << 2);
            peripheral_initialized = 1;
        }

        return peripheral_initialized;
    }

    // Renamed from get_status()
    // Checks if the SSP controller is idle.
    // @param sspx - the perpheral SSP0-2 to check
    // @return 1 - the controller is sending or receiving a data frame.
    // @return 0 - the controller is idle.
    bool GetTransferStatus() override
    {
        return (ssp_registers[static_cast<uint32_t>(pssp_)]->SR & (1<<4));
    }

    // Transfers a data frame to an external device using the SSP
    // data register. This functions for both transmitting and
    // receiving data. It is recommended this region be protected
    // by a mutex.
    // @param sspx - the perpheral SSP0-2 to use
    // @param data - information to be placed in data register
    // @return - received data from external device
    uint16_t Transfer(uint16_t data) override
    {
        ssp_registers[static_cast<uint32_t>(pssp_)]->DR = data;
        while (GetTransferStatus()) { continue; }
        return static_cast<uint16_t>(
                ssp_registers[static_cast<uint32_t>(pssp_)]->DR);
    }

    // Set the PCONP bits to power on peripheral
    // Check the passed Peripheral against that of the struct Peripheral
    // before setting these bits.
    // @param sspx - the peripheral to turn on
    void SetPeripheralPower() override
    {
        pconp_bits_ =
            (pssp_ == Peripheral::kSsp0) ? kPconp0 : kPconp1;
    }

    // Sets the various modes for the Peripheral
    // @param mode - master or slave mode
    // @param frame - format for Peripheral data to use
    // @param size - number of bits per frame
    void SetPeripheralMode(
        MasterSlaveMode mode, FrameMode frame, DataSize size) override
    {
        master_mode_ = mode;
        frame_format_ = frame;
        data_size_ = size;
    }

    // Sets the clock rate for the Peripheral
    void SetClock(bool polarity, bool phase,
        uint8_t divider, uint8_t prescaler) override
    {
        clock_polarity_ = polarity;
        clock_phase_ = phase;
        clock_divider_ = divider;
        clock_prescaler_ = prescaler;
    }


    ~Ssp()
    {
        // do nothing
    }

 private:
    PinConfigureInterface & mosi_;
    PinConfigureInterface & miso_;
    PinConfigureInterface & sck_;

    PinConfigure mosi_pin_;
    PinConfigure miso_pin_;
    PinConfigure sck_pin_;

    // SSP member variables
    Peripheral pssp_;                // SSP interfaces
    PowerOn pconp_bits_;
    MasterSlaveMode master_mode_;
    DataSize data_size_;             // data size, number of bits
    FrameMode frame_format_;         // frame format
    bool clock_polarity_;            // clock polarity
    bool clock_phase_;               // clock phase
    // clock_divider_ is the number of prescaler-output clocks per bit
    // see page 611 of UM10562 for SCR-Serial Clock Rate for more info
    uint8_t clock_divider_;
    uint8_t clock_prescaler_;        // clock prescaler
};
