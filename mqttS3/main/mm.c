#include "esp_spi_flash.h"

void app_main() {
    // uint32_t flash_size = spi_flash_get_chip_size();
    // printf("Flash size: %u bytes\n", flash_size);
    //     size_t free_ram = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    // printf("Free RAM: %u bytes\n", free_ram);
    spi_flash_mmap_dump();
}
