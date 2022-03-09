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
    AppBootloaderPart<Can> bootloader;

    auto next1s  = Clock::now();

    Kvasir::SHT30<I2C, Clock>  TemperatureSensor{0x44}; //Address in DEC: 68
    Kvasir::SGP30<I2C, Clock>  AirQualitySensor{0x58}; //Address in DEC: 88
    Kvasir::BMP384<I2C, Clock> PressureSensor{0x77}; //Address in DEC: 119
    Kvasir::BH1751<I2C, Clock> LightSensor{0x23}; //Address in DEC: 35
    CANCommunicator<Can, Clock> canCommunicator;


    auto i2cPowerManager{Kvasir::make_I2CPowerManager<I2C, Clock, HW::Pin::sw_vdd, true>(
      TemperatureSensor,
      AirQualitySensor,
      LightSensor,
      PressureSensor)};

    while(true) {
        std::optional<float> CurrentTemperature{TemperatureSensor.t()};
        std::optional<float> CurrentRelativeHumidity{TemperatureSensor.rh()};
        std::optional<float> CurrentAbsoluteHumidity{TemperatureSensor.ah()};
        std::optional<float> CurrentLight{LightSensor.lux()};
        std::optional<float> CurrentAirPressure{PressureSensor.p()};

        if(Clock::now() >= next1s) {
            next1s = Clock::now() + 1s;
            KL_T(
              "Temp:{:.1f} HumidRel:{:.1f} HumidAbs:{:.1f} VOC:{} CO2Eq:{} Light:{} Pressure:{:.1f} {:.1f}",
              TemperatureSensor.t(),
              TemperatureSensor.rh(),
              TemperatureSensor.ah(),
              AirQualitySensor.vocraw_,
              AirQualitySensor.co2eqraw_,
              LightSensor.luxraw_,
              PressureSensor.p(),
              PressureSensor.t());
        }

        canCommunicator.update(CurrentTemperature,
                               CurrentRelativeHumidity,
                               CurrentAbsoluteHumidity,
                               AirQualitySensor.vocraw_,
                               AirQualitySensor.co2eqraw_,
                               CurrentLight,
                               CurrentAirPressure);

        canCommunicator.handler();
        //Goes 0...70
        if(auto msg = Can::recv(); msg) {
            bootloader.handler(*msg);
        }


        //TODO CAN communication!
        /*
        if(auto msg = Can::recv(); msg) {
            if(msg->data[0] == 0x02_b) {
                KL_T("revc {} {}", msg->size(), msg->data);

                msg->F0 = ((((msg->F0 & 0x1ffc0000_u32) >> 18_u32) - 2_u32) % (1_u32 << 11_u32))
                       << 18_u32;
                KL_D("CAN: id={:x} s={:x} - {}", msg->F0 >> 2U, msg->F1 , msg->data);
                if(!Can::send(*msg)) {
                    KL_E("send fail");
                }
            }
        }*/

        i2cPowerManager.handler();
        StackProtector::handler();
    }
}

KVASIR_START(Startup)
