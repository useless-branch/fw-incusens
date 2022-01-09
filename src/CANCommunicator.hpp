//
// Created by patrick on 1/6/22.
//
#pragma once
#include <chrono>
#include <optional>
#include "serializer.hpp"


template<typename CAN, typename Clock>
struct CANCommunicator {
    using tp = typename Clock::time_point;
    std::optional<float>         Temperature;
    std::optional<float>         RelativeHumidity;
    std::optional<float>         AbsoluteHumidity;
    std::optional<std::uint32_t> AirQualityVOC;
    std::optional<std::uint32_t> AirQualityCO2;
    std::optional<float>         Light;
    std::optional<float>         AirPressure;

    bool   busy_;
    tp     waitTime_;
    std::uint32_t errorCounter{0};

    static constexpr auto sendInterval{std::chrono::seconds(1)};

    enum class State : std::uint8_t  {
        reset,
        idle,
        sendTemperature,
        sendRelHumidity,
        sendAbsHumidity,
        sendAQVOC,
        sendAQCO2,
        sendLight,
        sendAirPressure,
        error
    };

    State st_ = State::reset;

    void handler() {
        auto const currentTime = Clock::now();
        if(errorCounter > 1000) {
            st_ = State::error;
            KL_E("can is not working... shutting can down!");
        }

        switch(st_) {
            case State::reset:
            {
                errorCounter = 0;
                st_          = State::idle;
                Temperature.reset();
                RelativeHumidity.reset();
                AbsoluteHumidity.reset();
                AirQualityVOC.reset();
                AirQualityCO2.reset();
                Light.reset();
                AirPressure.reset();
                //waitTime_ = currentTime + std::chrono::seconds(5);
            }
                break;
        case State::idle:
            {
                busy_        = false;
                errorCounter = 0;
                if(currentTime > waitTime_) {
                    st_   = State::sendTemperature;
                    busy_ = true;
                }
            }
            break;



        case State::sendTemperature:
            {

                st_ = State::sendRelHumidity;
                if(Temperature) {
                    auto                     value      = Temperature.value();
                    constexpr size_t         dataSize   = sizeof(value);
                    constexpr std::uint8_t   identifier = 0x1;
                    std::array<std::byte, 8> buffer{};
                    std::memcpy(
                            &buffer,
                            &value,
                            dataSize
                    );
                    Kvasir::CAN::CanMessage msg{identifier << 18U, (dataSize << 16U) | 0xFF000000, buffer};
                    if (!Can::send(msg)) {
                        st_ = State::sendTemperature;
                        ++errorCounter;
                        KL_W("Could not send temperature");
                    }

                }
            }
            break;

        case State::sendRelHumidity:
            {
                st_ = State::sendAbsHumidity;
                if(RelativeHumidity) {
                    auto value = RelativeHumidity.value();
                    constexpr size_t dataSize = sizeof(value);
                    constexpr std::uint8_t identifier = 0x2;
                    std::array<std::byte, 8> buffer{};
                    std::memcpy(&buffer, &value, dataSize);
                    Kvasir::CAN::CanMessage msg{identifier << 18U, (dataSize << 16U) | 0xFF000000, buffer};
                    if(!Can::send(msg)) {
                        st_ = State::sendRelHumidity;
                        KL_W("Could not send relative humidity");
                    }
                }
            }
            break;
        case State::sendAbsHumidity:
            {
                st_ = State::sendAQVOC;
                if(AbsoluteHumidity) {
                    auto value = AbsoluteHumidity.value();
                    constexpr size_t dataSize = sizeof(value);
                    constexpr std::uint8_t identifier = 0x3;
                    std::array<std::byte, 8> buffer{};
                    std::memcpy(&buffer, &value, dataSize);
                    Kvasir::CAN::CanMessage msg{identifier << 18U, (dataSize << 16U) | 0xFF000000, buffer};
                    if(!Can::send(msg)) {
                        st_ = State::sendAbsHumidity;
                        ++errorCounter;
                        KL_W("Could not send absolute humidity");
                    }
                }
            }
            break;

        case State::sendAQVOC:
            {
                st_ = State::sendAQCO2;
                if(AirQualityVOC) {
                    auto value = AirQualityVOC.value();
                    constexpr size_t dataSize = sizeof(value);
                    constexpr std::uint8_t identifier = 0x4;
                    std::array<std::byte, 8> buffer{};
                    std::memcpy(&buffer, &value, dataSize);
                    Kvasir::CAN::CanMessage msg{identifier << 18U, (dataSize << 16U) | 0xFF000000, buffer};
                    if(!Can::send(msg)) {
                        st_ = State::sendAQVOC;
                        ++errorCounter;
                        KL_W("Could not send VOC");
                    }
                }
            }
            break;

        case State::sendAQCO2:
            {
                st_ = State::sendLight;
                if(AirQualityCO2) {
                    auto value = AirQualityCO2.value();
                    constexpr size_t dataSize = sizeof(value);
                    constexpr std::uint8_t identifier = 0x5;
                    std::array<std::byte, 8> buffer{};
                    std::memcpy(&buffer, &value, dataSize);
                    Kvasir::CAN::CanMessage msg{identifier << 18U, (dataSize << 16U) | 0xFF000000, buffer};
                    if(!Can::send(msg)) {
                        st_ = State::sendAQCO2;
                        ++errorCounter;
                        KL_W("Could not send CO2");
                    }
                }
            }
            break;

        case State::sendLight:
            {
                st_ = State::sendAirPressure;
                if(Light) {
                    auto value = Light.value();
                    constexpr size_t dataSize = sizeof(value);
                    constexpr std::uint8_t identifier = 0x6;
                    std::array<std::byte, 8> buffer{};
                    std::memcpy(&buffer, &value, dataSize);
                    Kvasir::CAN::CanMessage msg{identifier << 18U, (dataSize << 16U) | 0xFF000000, buffer};
                    if(!Can::send(msg)) {
                        st_ = State::sendLight;
                        ++errorCounter;
                        KL_W("Could not send light");
                    }
                }
            }
            break;

        case State::sendAirPressure:
            {
                st_ = State::idle;
                if(AirPressure) {
                    auto value = AirPressure.value();
                    constexpr size_t dataSize = sizeof(value);
                    constexpr std::uint8_t identifier = 0x7;
                    std::array<std::byte, 8> buffer{};
                    std::memcpy(&buffer, &value, dataSize);
                    Kvasir::CAN::CanMessage msg{identifier << 18U, (dataSize << 16U) | 0xFF000000, buffer};
                    if(!Can::send(msg)) {
                        st_ = State::sendAirPressure;
                        ++errorCounter;
                        KL_W("Could not send air pressure");
                    }
                }
                waitTime_ = currentTime + sendInterval;
            }
            break;

        case State::error:
            {
                //st_ = State::reset;
            }
            break;
        }
    }

    void update(
      std::optional<float>         temp,
      std::optional<float>         relHumid,
      std::optional<float>         absHumid,
      std::optional<std::uint32_t> airQVOC,
      std::optional<std::uint32_t> airQCO2,
      std::optional<float>         light,
      std::optional<float>         airPres) {
        if(!busy_) {
            Temperature      = temp;
            RelativeHumidity = relHumid;
            AbsoluteHumidity = absHumid;
            AirQualityVOC    = airQVOC;
            AirQualityCO2    = airQCO2;
            Light            = light;
            AirPressure      = airPres;
        }
    }
};
