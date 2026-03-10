#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <signal.h>

#define LWHPSFPGA_BASE 0xff200000
#define LED_OFFSET     0x70
#define SPAN           0x1000

volatile unsigned int *led_ptr;
void *virtual_base;

// Cleanup function to turn off LEDs on exit
void cleanup(int sig) {
    if (led_ptr) *led_ptr = 0;
    if (virtual_base) munmap(virtual_base, SPAN);
    printf("\nApplication closed, LEDs turned off.\n");
    exit(0);
}

int main() {
    int i2c_file, mem_file;
    unsigned char val;
    unsigned int current_leds = 0x1;

    // Hvatanje signala za prekid
    signal(SIGINT, cleanup);

    // 1. Map memory for FPGA LEDs
    mem_file = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_file < 0) { 
	perror("Error: Cannot open /dev/mem"); 
	return 1; 
    }
    
    virtual_base = mmap(NULL, SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, mem_file, LWHPSFPGA_BASE);
    if (virtual_base == MAP_FAILED) { 
	perror("Error: mmap failed"); 
	close(mem_file); 
	return 1; 
    }

    // Set pointer to LED offset
    led_ptr = (unsigned int *)(virtual_base + LED_OFFSET);
    *led_ptr = current_leds; // Inicijalno stanje na ploci

    // 2. Initialize I2C bus
    i2c_file = open("/dev/i2c-0", O_RDWR);
    if (i2c_file < 0) { 
	perror("Error: Cannot open I2C bus"); 
	return 1; 
    }
    ioctl(i2c_file, I2C_SLAVE, 0x70);

    printf("Ready! Joystick control active. Press CTRL+C to exit.\n");

    while(1) {
        // Read input register from joystick
        unsigned char reg = 0x00;
        write(i2c_file, &reg, 1);
        read(i2c_file, &val, 1);

        // Process joystick input
        switch(val & 0x1F) {
            case 0x17: // Up
                current_leds = 0x3FF; 
                break;
            case 0x1B: // Down
                current_leds = 0x000; 
                break;
            case 0x1E: // Left
                if (current_leds == 0) current_leds = 0x1;
                else current_leds = (current_leds << 1) & 0x3FF;
                break;
            case 0x0F: // Right
                if (current_leds == 0) current_leds = 0x200;
                else current_leds = (current_leds >> 1);
                break;
            case 0x1F: // Center
                break;
            default:
                *led_ptr = 0x2AA; 
        }

	// Update LEDs
	*led_ptr = current_leds;
        usleep(200000);
    }

    munmap(virtual_base, SPAN);
    close(mem_file);
    close(i2c_file);

    return 0;
}