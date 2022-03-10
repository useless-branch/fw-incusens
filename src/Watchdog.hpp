//
// Created by patrick on 3/10/22.
//

#pragma once

struct WDReset {
    void enable(){
        apply(set(Kvasir::Peripheral::WDT::Registers<>::CTRLA::enable));
    }
    void operator()() const {
        apply(write(Kvasir::Peripheral::WDT::Registers<>::CLEAR::CLEARValC::key));
    }
};
