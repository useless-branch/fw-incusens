#include "HWConfig.hpp"
// need to be included first

#include "kvasir/StartUp/StartUp.hpp"
#include "kvasir/Util/FaultHandler.hpp"
#include "kvasir/Util/StackProtector.hpp"
#include "kvasir/Util/log.hpp"
#include "kvasir/Util/version.hpp"

using Clock   = HW::SystickClock;

using I2C             = Kvasir::Sercom::I2C::I2CBehavior<HW::I2CConfig, Clock, 32>;
using StackProtector   = Kvasir::StackProtector<>;
using HardFaultHandler = Kvasir::Fault::Handler<>;
using Can              = Kvasir::CAN::CANBehavior<HW::CANConfig, Clock>;

using Startup = Kvasir::Startup::Startup<HW::ClockSettings, Clock, StackProtector, HardFaultHandler, I2C, Can>;

int main() {
    KL_T("{}", Kvasir::Version::FullVersion);

    auto        next = Clock::now();
    auto lastTime = next;

    while(true) {

        if(Clock::now() >= next) {
            next = Clock::now() + 100ms;
        }

        if(auto msg = Can::recv(); msg) {
            if(msg->data[0] == 0x02_b) {
                KL_T("revc {} {}", msg->size(), msg->data);

                msg->F0 = ((((msg->F0 & 0x1ffc0000_u32) >> 18_u32) - 2_u32) % (1_u32 << 11_u32))
                       << 18_u32;

                if(!Can::send(*msg)) {
                    KL_T("send fail");
                }
            }
        }

        StackProtector::handler();
    }
}

KVASIR_START(Startup)
