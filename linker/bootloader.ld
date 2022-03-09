/* Linker script for ATSAMC21G17A cortex-m0plus */
/* Bootloader */
MEMORY{
    flash  (xr ) : ORIGIN = 0x00000000, LENGTH =  7936
    eeprom ( r ) : ORIGIN = 0x00001F00, LENGTH =   256 /* main flash */
    ram    (xrw) : ORIGIN = 0x20000000, LENGTH =   16K
}

INCLUDE common_flash.ld
INCLUDE common_eeprom.ld
INCLUDE common_ram.ld

INCLUDE common.ld

