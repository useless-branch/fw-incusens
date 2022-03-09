/* Linker script for ATSAMC21G17A cortex-m0plus */
/* App */
MEMORY{
    flash  (xr ) : ORIGIN = 0x00002000, LENGTH = 120K
    eeprom ( r ) : ORIGIN = 0x00400000, LENGTH =   4K /*RWW flash eeprom emulation*/
    ram    (xrw) : ORIGIN = 0x20000000, LENGTH =  16K
}

INCLUDE common_flash.ld
INCLUDE common_eeprom.ld
INCLUDE common_ram.ld

INCLUDE common.ld

