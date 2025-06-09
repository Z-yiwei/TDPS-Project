#include "Drv_Vision.h"

typedef struct
{
    uint8_t data1;  // Vision data 1
    uint8_t data2;  // Vision data 2
    uint8_t data3;  // Vision data 3
    uint8_t data4;  // Vision data 4
    uint8_t work_sta; // Work status, 1 means working, 0 means not working
} _vision_st;

typedef struct {
    int x;
    int y;
} Coordinate;

_vision_st vision;  // Used to store the state and data of the vision module
static uint8_t _datatemp[50];  // Used to store received data
static uint8_t _data_len = 0;
static uint8_t _data_cnt = 0;
static uint8_t rxstate = 0;  // Serial port reception state

// Function to check the status of the vision module
void Vision_Check_State(float dT_s)
{
    u8 tmp[2];
    // Data check 1
    if (_data_len == 4)  // If the received data length is 4 (valid data length for the vision module)
    {
        tmp[0] = 1;  // Data check passed
    }
    else
    {
        tmp[0] = 0;  // Data check failed
    }

    // Set work status
    if (tmp[0])  // If the data check passed
    {
        vision.work_sta = 1;  // Set work status to 1
    }
    else
    {
        vision.work_sta = 0;  // Otherwise set work status to 0
    }
}

// Vision_GetOneByte is the data reception function, handling each received byte
// This function is called every time a byte of data is received via the serial port
void Vision_GetOneByte(uint8_t data)
{
    if (rxstate == 0 && data == 0xEE)  // Look for the start frame 0xEE
    {
        rxstate = 1;
        _data_len = 1; 
        _datatemp[0] = data;  // Store the start frame
    }
    else if (rxstate == 1 && (data == 0x01 || data == 0x00))  // After receiving the start frame, receive four data bytes
    {
        _datatemp[rxstate] = data;
        rxstate++;
        _data_len++;
    }
    else if (1 < rxstate < 5 )
    {
        _datatemp[rxstate] = data;
        rxstate++;
        _data_len++;
    }
    else if (rxstate == 5 && data == 0xFF)  // Look for the end frame 0xFF
    {
        _datatemp[rxstate] = data;
        _data_len++;
        // Call the data analysis function to process the received data
        Vision_DataAnl(_datatemp, _data_len);  // A total of 5 bytes received (start frame + data + end frame)
        _data_len = 0;
        rxstate = 0;  // Reinitialize the state machine, ready to receive the next data frame
    }
    else
    {
        rxstate = 0;  // If the data format does not match, reinitialize the state machine
    }
}

// Vision_DataAnl is the data analysis function used to parse the data transmitted from the vision module
static void Vision_DataAnl(uint8_t *data, uint8_t len)
{
    if (len != 5)  // Data length must be 5 (start frame + 4 data bytes + end frame)
    {
        return;  // Exit if the data length is incorrect
    }

    // Check if the data is valid
    if (data[0] != 0xEE || data[4] != 0xFF)  // Check the start and end frames
    {
        return;  // Exit if the start or end frames are incorrect
    }

    // Parse the data (assuming the four data bytes represent different vision information)
    vision.data1 = data[1];  // Assume data[1] is the first vision data
    vision.data2 = data[2];  // Assume data[2] is the second vision data
    vision.data3 = data[3];  // Assume data[3] is the third vision data
    vision.data4 = data[4];  // Assume data[4] is the fourth vision data (note that the end frame does not affect the data)

    // You can modify the parsing logic based on the actual vision data format
}

// ConvertCoordinates function converts the coordinates based on the sign and value of x and y
Coordinate ConvertCoordinates(uint8_t x_sign, uint8_t x_value, uint8_t y_sign, uint8_t y_value) {
    Coordinate result;

    result.x = (x_sign == 0x01) ? -(int)x_value : (int)x_value;
    result.y = (y_sign == 0x01) ? -(int)y_value : (int)y_value;

    return result;
}

// ConvertCoordinates function call example:
// int main() {
//     uint8_t x_sign = 0x01;
//     uint8_t x_value = 0x09;
//     uint8_t y_sign = 0x00;
//     uint8_t y_value = 0x14;
//     Coordinate pos = ConvertCoordinates(x_sign, x_value, y_sign, y_value);
//     printf("x = %d, y = %d\n", pos.x, pos.y);  // Output: x = -10, y = 20
//
//     return 0;
// }

