#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

int main() {
    int file;
    char *filename = "/dev/i2c-0";

    // I2C address of the device (joystick I/O expander)
    int addr = 0x70;

    // Register address for the Input Port
    char reg[1] = {0x00};

    // Buffer for the value read from the device
    char data[1] = {0};

    // 1. Open the I2C bus device
    if ((file = open(filename, O_RDWR)) < 0) {
        perror("Error: Cannot open I2C bus");
        return 1;
    }

    // 2. Set the slave device address
    if (ioctl(file, I2C_SLAVE, addr) < 0) {
        perror("Error: Cannot access device at address 0x70");
        return 1;
    }

    printf("Ready! Reading joystick state... Press CTRL+C to exit.\n");

    // 3. Continuously read the input register
    while (1) {

        // Select register 0x00 (Input Port)
        if (write(file, reg, 1) != 1) {
            perror("Error writing register address");
        }

        // Read one byte from the device
        if (read(file, data, 1) != 1) {
            perror("Error reading data");
        } else {
            // Print the current state in hex format
            printf("Current state: 0x%02X\n", data[0]);
        }

        // Delay to avoid excessive terminal output
        usleep(200000);
    }

    // Close the I2C device
    close(file);

    return 0;
}