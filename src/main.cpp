#include "HWConfig.hpp"
// need to be included first

#include "kvasir/StartUp/StartUp.hpp"
#include "kvasir/Util/FaultHandler.hpp"
#include "kvasir/Util/StackProtector.hpp"
#include "kvasir/Util/log.hpp"
#include "kvasir/Util/version.hpp"

using Clock = HW::SystickClock;

using I2C              = Kvasir::Sercom::I2C::I2CBehavior<HW::I2CConfig, Clock, 32>;
using StackProtector   = Kvasir::StackProtector<>;
using HardFaultHandler = Kvasir::Fault::Handler<>;
using Can              = Kvasir::CAN::CANBehavior<HW::CANConfig, Clock>;

using Startup
  = Kvasir::Startup::Startup<HW::ClockSettings, Clock, StackProtector, HardFaultHandler, I2C, Can>;

#include "kvasir/Devices/I2CPowerManager.hpp"
#include "kvasir/Devices/bh1751.hpp"
#include "kvasir/Devices/bmp384.hpp"
#include "kvasir/Devices/sgp30.hpp"
#include "kvasir/Devices/sht30.hpp"
#include "CANCommunicator.hpp"
#include "aglio/packager.hpp"
#include "aglio/serializer.hpp"
#include "kvasir/Util/AppBootloader.hpp"
#include "Watchdog.hpp"

template<typename Can>
struct AppBootloaderPart {
    using RequestSet = Kvasir::Bootloader::RequestSet;
    using Packager   = Kvasir::Bootloader::Packager;

    struct ID {
        auto operator()() { return Kvasir::serial_number(); }
    };

    struct ProductType {
        auto operator()() { return Kvasir::Version::NameTargetVersion; }
    };

    Kvasir::Bootloader::AppBootloader<Clock, ID, ProductType> appBootloader{};
    Kvasir::StaticVector<std::byte, 128>                      recvBuffer;
    void handler(Kvasir::CAN::CanMessage const& newMsg) {
        if(newMsg.id() == 1) {
            auto ret = Kvasir::Bootloader::parse<RequestSet>(newMsg, recvBuffer);
            if(ret) {
                appBootloader.handler(*ret, [](auto const& response, std::uint8_t channel) {
                    Kvasir::Bootloader::CAN::packAndSend<Can>(response, channel);
                });
            }
        }
    }
};


int main() {
    KL_I("{}", Kvasir::Version::FullVersion);
    WDReset{}();
    WDReset{}.enable();
    AppBootloaderPart<Can> bootloader;

    auto next1s  = Clock::now();

    Kvasir::SHT30<I2C, Clock>  TemperatureSensor{0x44}; //Address in DEC: 68

    //Currently not working -> Error in Footprint
    //Kvasir::BH1751<I2C, Clock> LightSensor{0x23}; //Address in DEC: 35
    CANCommunicator<Can, Clock> canCommunicator;


    auto i2cPowerManager{Kvasir::make_I2CPowerManager<I2C, Clock, HW::Pin::sw_vdd, true>(
      TemperatureSensor
     )};

    while(true) {
        std::optional<float> CurrentTemperature{TemperatureSensor.t()};
        std::optional<float> CurrentRelativeHumidity{TemperatureSensor.rh()};
        std::optional<float> CurrentAbsoluteHumidity{TemperatureSensor.ah()};

        if(Clock::now() >= next1s) {
            next1s = Clock::now() + 1s;
            KL_T(
              "Temp:{:.1f} HumidRel:{:.1f} HumidAbs:{:.1f}",
              TemperatureSensor.t(),
              TemperatureSensor.rh(),
              TemperatureSensor.ah()
              );
        }

        canCommunicator.update(static_cast<std::optional<float>>(TemperatureSensor.t()),
                               static_cast<std::optional<float>>(TemperatureSensor.rh()),
                               static_cast<std::optional<float>>(TemperatureSensor.ah()),
                               std::nullopt,
                               std::nullopt,
                               std::nullopt,
                               std::nullopt
                               );

        canCommunicator.handler();
        //Goes 0...70
        if(auto msg = Can::recv(); msg) {
            bootloader.handler(*msg);
        }

        i2cPowerManager.handler();
        StackProtector::handler();
    }
}

KVASIR_START(Startup)
