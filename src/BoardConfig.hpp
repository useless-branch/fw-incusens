//
// Created by patrick on 2/20/22.
//

#pragma once

#include <string>

struct BoardConfig {
    static constexpr auto name{"Incubator"};
    static constexpr auto canBaseAddress{77};
    struct Sensors {
        struct Temperature {
        private:
            static constexpr auto canBlockOffsetTemp{0};
            static constexpr auto canBlockOffsetAbsoluteHumid{1};
            static constexpr auto canBlockOffsetRelativeHumid{2};

        public:
            static constexpr auto name{"Temperature"};
            static constexpr auto address{0x44};
            static constexpr auto canAddressTemp{canBaseAddress + canBlockOffsetTemp};
            static constexpr auto canAddressAbsoluteHumid{
              canBaseAddress + canBlockOffsetAbsoluteHumid};
            static constexpr auto canAddressRelativeHumid{
              canBaseAddress + canBlockOffsetRelativeHumid};
        };
        struct AirQuality {
        private:
            static constexpr auto canBlockOffsetVOC{3};
            static constexpr auto canBlockOffsetCO2Eq{4};

        public:
            static constexpr auto name{"AirQuality"};
            static constexpr auto address{0x58};
            static constexpr auto canAddressVOC{canBaseAddress + canBlockOffsetVOC};
            static constexpr auto canAddressCO2Eq{canBaseAddress + canBlockOffsetCO2Eq};
        };
        struct Pressure {
        private:
            static constexpr auto canBlockAddress{5};

        public:
            static constexpr auto name{"Pressure"};
            static constexpr auto address{0x77};
            static constexpr auto canAddress{canBaseAddress + canBlockAddress};
        };
        struct Light {
        private:
            static constexpr auto canBlockAddress{6};

        public:
            static constexpr auto name{"Light"};
            static constexpr auto address{0x23};
            static constexpr auto canAddress{canBaseAddress + canBlockAddress};
        };
    };
};
