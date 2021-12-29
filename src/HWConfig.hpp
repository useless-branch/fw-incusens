#pragma once

static bool dbgpres();

struct DebuggerPresentFunction {
    bool operator()() { return dbgpres(); }
};

#include "kvasir/Util/DefaultPrintBackend.hpp"

namespace Kvasir { namespace Log {
    template<>
    struct PrintBackend<Kvasir::Tag::User>
      : public DefaultPrintBackend<DebuggerPresentFunction, 512> {};
}}   // namespace Kvasir::Log
// need to be included first

#include "chip/Interrupt.hpp"
#include "core/core.hpp"
#include "kvasir/Util/log.hpp"

namespace HW {
static constexpr auto ClockSpeed   = 48'000'000;
static constexpr auto CrystalSpeed = 8'000'000;

struct SystickClockConfig {
    static constexpr auto clockBase = Kvasir::Systick::useProcessorClock;

    static constexpr auto clockSpeed     = ClockSpeed;
    static constexpr auto minOverrunTime = std::chrono::hours(24 * 365) * 20;
};
using SystickClock = Kvasir::Systick::SystickClockBase<SystickClockConfig>;
}   // namespace HW

namespace Kvasir { namespace Log {
    template<>
    struct LogClock<Kvasir::Tag::User> : public HW::SystickClock {};
}}   // namespace Kvasir::Log

#include "chip/chip.hpp"

static bool dbgpres() {
    return apply(read(Kvasir::Peripheral::DSU::Registers<>::STATUSB::dbgpres));
}

namespace HW {
using PrintBackend = Kvasir::Log::PrintBackend<Kvasir::Tag::User>;

namespace Pin {
    using sw_5v      = decltype(makePinLocation(Kvasir::Io::portA, Kvasir::Io::pin0));
    using sw_vdd     = decltype(makePinLocation(Kvasir::Io::portA, Kvasir::Io::pin1));
    using gpio0      = decltype(makePinLocation(Kvasir::Io::portA, Kvasir::Io::pin2));
    using gpio1      = decltype(makePinLocation(Kvasir::Io::portA, Kvasir::Io::pin3));
    using sercom_0_0 = decltype(makePinLocation(Kvasir::Io::portA, Kvasir::Io::pin4));
    using sercom_0_1 = decltype(makePinLocation(Kvasir::Io::portA, Kvasir::Io::pin5));
    using sercom_0_2 = decltype(makePinLocation(Kvasir::Io::portA, Kvasir::Io::pin6));
    using sercom_0_3 = decltype(makePinLocation(Kvasir::Io::portA, Kvasir::Io::pin7));
    using sda1       = decltype(makePinLocation(Kvasir::Io::portA, Kvasir::Io::pin8));
    using scl1       = decltype(makePinLocation(Kvasir::Io::portA, Kvasir::Io::pin9));
    using can_tx     = decltype(makePinLocation(Kvasir::Io::portA, Kvasir::Io::pin24));
    using can_rx     = decltype(makePinLocation(Kvasir::Io::portA, Kvasir::Io::pin25));
}   // namespace Pin

struct ClockSettings {
    static void coreClockInit() {
        using namespace Kvasir::GCLK;
        using KNR = Kvasir::Peripheral::NVMCTRL::Registers<>;
        using osc = Kvasir::Peripheral::OSCCTRL::Registers<>;
        using Kvasir::Register::overrideDefaults;

        using Kvasir::Register::value;

        static constexpr auto flash_waitstates = 1;
        static constexpr auto DPLL_ldr         = 3095;
        static constexpr auto DPLL_ldrfrac     = 0;
        static constexpr auto DPLL_div         = 128;
        static constexpr auto GENDIV_div       = 2;

        static_assert(
          ClockSpeed
            == std::uint64_t(
              (CrystalSpeed * (1.0 / (2.0 * (double(DPLL_div) + 1.0))))
              * (double(DPLL_ldr) + 1.0 + (double(DPLL_ldrfrac) / 16.0)) / double(GENDIV_div)),
          "ClockSpeed wrong");

        apply(osc::XOSCCTRL::overrideDefaults(
          set(osc::XOSCCTRL::enable),
          set(osc::XOSCCTRL::xtalen),
          write(osc::XOSCCTRL::GAINValC::gain8),
          clear(osc::XOSCCTRL::ondemand),
          set(osc::XOSCCTRL::runstdby),
          set(osc::XOSCCTRL::ampgc),
          write(osc::XOSCCTRL::STARTUPValC::cycle64)));

        while(!apply(read(osc::STATUS::xoscrdy))) {
        }

        apply(
          write(osc::DPLLRATIO::ldr, value<DPLL_ldr>()),
          write(osc::DPLLRATIO::ldrfrac, value<DPLL_ldrfrac>()),
          osc::DPLLCTRLB::overrideDefaults(
            write(osc::DPLLCTRLB::div, value<DPLL_div>()),
            write(osc::DPLLCTRLB::REFCLKValC::xosc)),
          Kvasir::Register::sequencePoint,
          osc::DPLLCTRLA::overrideDefaults(
            set(osc::DPLLCTRLA::enable),
            set(osc::DPLLCTRLA::runstdby),
            clear(osc::DPLLCTRLA::ondemand)));

        apply(KNR::CTRLB::overrideDefaults(write(KNR::CTRLB::rws, value<flash_waitstates>())));

        while(!apply(read(osc::DPLLSTATUS::clkrdy))) {
        }

        apply(GenericClockGenerator<0, GeneratorSource::dpll96m, GENDIV_div>::enable());
    }

    static void peripheryClockInit() {
        using namespace Kvasir::GCLK;
        apply(
          GenericClockGenerator<2, GeneratorSource::xosc, 1 << 20>::enable(),
          GenericClockGenerator<4, GeneratorSource::xosc, 1>::enable(),
          Kvasir::Register::sequencePoint,
          PeripheralChannelController<0, Peripheral::sercom0_core>::enable(),
          PeripheralChannelController<0, Peripheral::sercom2_core>::enable(),
          PeripheralChannelController<4, Peripheral::can0>::enable());
    }
};

//TODO Configure Busses and IO
struct I2CConfig {
    static constexpr auto clockSpeed = ClockSpeed;

    static constexpr auto instance       = 2;
    static constexpr auto sdaPinLocation = Pin::sda1{};
    static constexpr auto sclPinLocation = Pin::scl1{};
    static constexpr auto baudRate       = 1'000'000;
    //    static constexpr auto maxBaudRateError = std::ratio<1,2>{};
    static constexpr auto isrPriority = 3;
};

struct CANConfig {
    static constexpr auto clockSpeed = CrystalSpeed;

    static constexpr auto instance      = 0;
    static constexpr auto rxPinLocation = Pin::can_rx{};
    static constexpr auto txPinLocation = Pin::can_tx{};
    static constexpr auto baudRate      = 500'000;
    static constexpr auto isrPriority   = 3;
};

}   // namespace HW
