#include "kvasir/Common/Core.hpp"
#include "kvasir/Common/Tags.hpp"

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

using PrintBackend = Kvasir::Log::PrintBackend<Kvasir::Tag::User>;

#include "chip/Interrupt.hpp"
#include "core/core.hpp"
#include "kvasir/Util/log.hpp"

static constexpr std::size_t ClockSpeed = 4'000'000;
struct SystickClockConfig {
    static constexpr auto clockBase = Kvasir::Systick::useProcessorClock;

    static constexpr auto clockSpeed     = ClockSpeed;
    static constexpr auto minOverrunTime = std::chrono::hours(24);
};
using SystickClock = Kvasir::Systick::SystickClockBase<SystickClockConfig>;

namespace Kvasir { namespace Log {
    template<>
    struct LogClock<Kvasir::Tag::User> : public SystickClock {};
}}   // namespace Kvasir::Log

#include "chip/chip.hpp"

static bool dbgpres() {
    return apply(read(Kvasir::Peripheral::DSU::Registers<>::STATUSB::dbgpres));
}

#include "kvasir/StartUp/StartUp.hpp"
#include "kvasir/Util/FaultHandler.hpp"
#include "kvasir/Util/StackProtector.hpp"
#include "kvasir/Util/log.hpp"
#include "kvasir/Util/version.hpp"

struct ClockSettings {
    static void coreClockInit() {
        using osc = Kvasir::Peripheral::OSCCTRL::Registers<>;
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
    }
    static void peripheryClockInit() {
        using namespace Kvasir::GCLK;
        apply(
          GenericClockGenerator<1, GeneratorSource::xosc, 1>::enable(),
          PeripheralChannelController<1, Peripheral::can0>::enable());
    }
};

using can_rx = decltype(makePinLocation(Kvasir::Io::portA, Kvasir::Io::pin25));
using can_tx = decltype(makePinLocation(Kvasir::Io::portA, Kvasir::Io::pin24));

struct CANConfig {
    static constexpr auto clockSpeed = 8000000;

    static constexpr auto instance      = 0;
    static constexpr auto rxPinLocation = can_rx{};
    static constexpr auto txPinLocation = can_tx{};
    static constexpr auto baudRate      = 500'000;
    static constexpr auto isrPriority   = 3;
};

using Clock        = SystickClock;
using FaultHandler = Kvasir::Fault::Handler<>;
using Can          = Kvasir::CAN::CANBehavior<CANConfig, Clock>;

using Startup = Kvasir::Startup::Startup<ClockSettings, PrintBackend, Clock, FaultHandler, Can>;

struct PeripheryReset {
    static void reset() {
        using namespace Kvasir::GCLK;
        apply(Kvasir::Nvic::makeDisable(Can::base::InterruptIndexs{}));
        apply(set(Can::Regs::CCCR::init));
        apply(makeInput(can_rx{}), makeInput(can_tx{}));
        apply(set(Kvasir::Peripheral::GCLK::Registers<>::CTRLA::swrst));
    }
};

#include "aglio/packager.hpp"
#include "aglio/serializer.hpp"
#include "kvasir/Util/StaticString.hpp"
#include "kvasir/Util/StaticVector.hpp"
//
#include "kvasir/Util/Bootloader.hpp"

static constexpr std::size_t BootLoaderSize     = 0x2000;   //TODO parse from linker file!!!!!!
static constexpr bool        EnableSelfOverride = false;

using Crc = Kvasir::Bootloader::Crc;

struct WDReset {
    constexpr void operator()() {}
};

using Com = Kvasir::Bootloader::CAN::
  Com<Clock, Can, Kvasir::Bootloader::RequestSet, Kvasir::Bootloader::ResponseSet, WDReset>;

template<typename T>
struct Eeprom : Kvasir::EepromEmulator<Clock, T, 1, true> {};

using Flash = Kvasir::Bootloader::Flash<Clock, EnableSelfOverride ? BootLoaderSize : 0, WDReset>;

struct ID {
    auto operator()() { return Kvasir::serial_number(); }
};
struct ProductType {
    auto operator()() { return Kvasir::Version::NameTargetVersion; }
};

int main() {
    KL_T("{}", Kvasir::Version::FullVersion);
    Kvasir::Bootloader::Bootloader<
      Clock,
      Com,
      Crc,
      Eeprom,
      Flash,
      PeripheryReset,
      ID,
      ProductType,
      BootLoaderSize,
      EnableSelfOverride>
      bootloader{};

    bootloader.run(bootloader.app_valid() ? 500ms : 1min);

    if(bootloader.app_valid()) {
        bootloader.start_app();
    }

    while(true) {
        apply(Kvasir::SystemControl::SystemReset{});
    }
}

KVASIR_START(Startup)
