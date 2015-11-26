#include <sys/types.h>
#include <bmi055.h>
#include <stdio.h>

BMI055_Handle_Type bmi;

uint32_t writer(uint8_t address, uint8_t data) {

    printf("writing value of 0x%x to location 0x%x\n",address,data);
    return 0;
}


uint32_t reader(uint8_t address, uint8_t * data) {
    *data=address+1;
    return 0;
}

int main(void) {
    uint8_t data;

    bmi.spi_write = writer;
    bmi.spi_read = reader;

    bmi.spi_write(0xAD, 0xEF);
    bmi.spi_read(0x01,&data);

    printf("Data read returned %x\n",data);



}
