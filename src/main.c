// usb-i2c-adapter / 08-Mar-2016 / grytole@gmail.com

// Write byte to I2C device (command ID = 00h):
//  TX:         00h, address, register, numbytes, byte1, [..., byteN]
//  RX SUCCESS: numbytes, byte1, [..., byteN]
//  RX FAILURE: 00h

// Read byte from I2C device (command ID = 01h):
//  TX:         01h, address, register, numbytes
//  RX SUCCESS: numbytes, byte1, [..., byteN]
//  RX FAILURE: 00h

// Detect presence of I2C devices (command ID = 02h):
//  TX:         02h, numbytes, [address1, ..., addressN]
//  RX SUCCESS: numbytes, address1, [..., addressN]
//  RX FAILURE: 00h

#include "stm8s.h"

#define DEFAULT_UART_BAUDRATE (115200)
#define MAX_PAYLOAD_SIZE (255)
#define MIN_DETECT_ADDRESS (0x03)
#define MAX_DETECT_ADDRESS (0x77)
#define RESPOND_FAILURE (0x00)

enum {
    STATE_STARTUP = 0,
    STATE_RECEIVE,
    STATE_EXECUTE,
    STATE_RESPOND
};

enum {
    COMMAND_WRITE = 0,
    COMMAND_READ,
    COMMAND_DETECT
};

static struct {
    uint8_t state;
    uint8_t command;
    uint8_t deviceAddress;
    uint8_t deviceRegister;
    uint8_t payloadSize;
    uint8_t payload[ MAX_PAYLOAD_SIZE ];
} adapter = {
    .state = STATE_STARTUP
};

static void adapterInit( void )
{
    CLK_HSIPrescalerConfig( CLK_PRESCALER_HSIDIV1 );
    UART1_DeInit();
    UART1_Init( DEFAULT_UART_BAUDRATE,
                UART1_WORDLENGTH_8D,
                UART1_STOPBITS_1,
                UART1_PARITY_NO,
                UART1_SYNCMODE_CLOCK_DISABLE,
                UART1_MODE_TXRX_ENABLE );
    I2C_DeInit();
    I2C_Init( I2C_MAX_STANDARD_FREQ,
              0x0000,
              I2C_DUTYCYCLE_2,
              I2C_ACK_CURR,
              I2C_ADDMODE_7BIT,
              I2C_MAX_INPUT_FREQ );
    adapter.state = STATE_RECEIVE;
}

static void adapterReceive( void )
{
    while( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) );
    adapter.command = UART1_ReceiveData8();
    switch( adapter.command )
    {
    case COMMAND_WRITE:
        while( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) );
        adapter.deviceAddress = UART1_ReceiveData8();
        while( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) );
        adapter.deviceRegister = UART1_ReceiveData8();
        while( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) );
        adapter.payloadSize = UART1_ReceiveData8();
        for( uint8_t i = 0; i < adapter.payloadSize; i++ )
        {
            while( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) );
            adapter.payload[ i ] = UART1_ReceiveData8();
        }
        break;
    case COMMAND_READ:
        while( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) );
        adapter.deviceAddress = UART1_ReceiveData8();
        while( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) );
        adapter.deviceRegister = UART1_ReceiveData8();
        while( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) );
        adapter.payloadSize = UART1_ReceiveData8();
        break;
    case COMMAND_DETECT:
        while( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) );
        adapter.payloadSize = UART1_ReceiveData8();
        for( uint8_t i = 0; i < adapter.payloadSize; i++ )
        {
            while( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) );
            adapter.payload[ i ] = UART1_ReceiveData8();
        }
        break;
    default:
        break;
    }
    adapter.state = STATE_EXECUTE;
}

static void adapterExecute( void )
{
    switch( adapter.command )
    {
    case COMMAND_WRITE:
        while( RESET != I2C_GetFlagStatus( I2C_FLAG_BUSBUSY ) );
        I2C_GenerateSTART( ENABLE );
        while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_MODE_SELECT ) );
        I2C_Send7bitAddress( adapter.deviceAddress << 1, I2C_DIRECTION_TX );
        while( ( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED ) ) &&
               ( RESET == I2C_GetFlagStatus( I2C_FLAG_ACKNOWLEDGEFAILURE ) ) );
        if( RESET == I2C_GetFlagStatus( I2C_FLAG_ACKNOWLEDGEFAILURE ) )
        {
            I2C_SendData( adapter.deviceRegister );
            while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_BYTE_TRANSMITTING ) );
            for( uint8_t i = 0; i < adapter.payloadSize; i++ )
            {
                I2C_SendData( adapter.payload[ i ] );
                while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_BYTE_TRANSMITTING ) );
                if( ( adapter.payloadSize - 1 ) == i )
                {
                    I2C_GenerateSTOP( ENABLE );
                }
            }
        }
        else
        {
            I2C_ClearFlag( I2C_FLAG_ACKNOWLEDGEFAILURE );
            I2C_GenerateSTOP( ENABLE );
            adapter.payloadSize = RESPOND_FAILURE;
        }
        break;
    case COMMAND_READ:
        while( RESET != I2C_GetFlagStatus( I2C_FLAG_BUSBUSY ) );
        I2C_GenerateSTART( ENABLE );
        while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_MODE_SELECT ) );
        I2C_Send7bitAddress( adapter.deviceAddress << 1, I2C_DIRECTION_TX );
        while( ( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED ) ) &&
               ( RESET == I2C_GetFlagStatus( I2C_FLAG_ACKNOWLEDGEFAILURE ) ) );
        if( RESET == I2C_GetFlagStatus( I2C_FLAG_ACKNOWLEDGEFAILURE ) )
        {
            I2C_SendData( adapter.deviceRegister );
            while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_BYTE_TRANSMITTING ) );
            I2C_GenerateSTART( ENABLE );
            while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_MODE_SELECT ) );
            I2C_Send7bitAddress( adapter.deviceAddress << 1, I2C_DIRECTION_RX );
            while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED ) );
            for( uint8_t i = 0; i < adapter.payloadSize; i++ )
            {
                if( ( adapter.payloadSize - 1 ) == i )
                {
                    I2C_AcknowledgeConfig( I2C_ACK_NONE );
                }
                while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_BYTE_RECEIVED ) );
                adapter.payload[ i ] = I2C_ReceiveData();
                if( ( adapter.payloadSize - 1 ) == i )
                {
                    I2C_GenerateSTOP( ENABLE );
                    I2C_AcknowledgeConfig( I2C_ACK_CURR );
                }
            }
        }
        else
        {
            I2C_ClearFlag( I2C_FLAG_ACKNOWLEDGEFAILURE );
            I2C_GenerateSTOP( ENABLE );
            adapter.payloadSize = RESPOND_FAILURE;
        }
        break;
    case COMMAND_DETECT:
        if( 0 == adapter.payloadSize )
        {
            for( uint8_t i = MIN_DETECT_ADDRESS; i <= MAX_DETECT_ADDRESS; i++  )
            {
                adapter.payload[ adapter.payloadSize++ ] = i;
            }
        }
        for( uint8_t i = 0, j = 0, cnt = adapter.payloadSize; i < cnt; i++, adapter.payloadSize = j )
        {
            while( RESET != I2C_GetFlagStatus( I2C_FLAG_BUSBUSY ) );
            I2C_GenerateSTART( ENABLE );
            while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_MODE_SELECT ) );
            I2C_Send7bitAddress( adapter.payload[ i ] << 1, I2C_DIRECTION_TX );
            while( ( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED ) ) &&
                   ( RESET == I2C_GetFlagStatus( I2C_FLAG_ACKNOWLEDGEFAILURE ) ) );
            if( RESET == I2C_GetFlagStatus( I2C_FLAG_ACKNOWLEDGEFAILURE ) )
            {
                adapter.payload[ j++ ] = adapter.payload[ i ];
            }
            else
            {
                I2C_ClearFlag( I2C_FLAG_ACKNOWLEDGEFAILURE );
            }
            I2C_GenerateSTOP( ENABLE );
        }
        break;
    default:
        adapter.payloadSize = RESPOND_FAILURE;
        break;
    }
    adapter.state = STATE_RESPOND;
}

static void adapterRespond( void )
{
    while( RESET == UART1_GetFlagStatus( UART1_FLAG_TXE ) );
    UART1_SendData8( adapter.payloadSize );
    for( uint8_t i = 0; i < adapter.payloadSize; i++ )
    {
        while( RESET == UART1_GetFlagStatus( UART1_FLAG_TXE ) );
        UART1_SendData8( adapter.payload[ i ] );
    }
    while( RESET == UART1_GetFlagStatus( UART1_FLAG_TC ) );
    adapter.state = STATE_RECEIVE;
}

void main( void )
{
    while( 1 )
    {
        switch( adapter.state )
        {
        default:
        case STATE_STARTUP:
            adapterInit();
            break;
        case STATE_RECEIVE:
            adapterReceive();
            break;
        case STATE_EXECUTE:
            adapterExecute();
            break;
        case STATE_RESPOND:
            adapterRespond();
            break;
        }
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed( uint8_t* file, uint32_t line )
{
    while( 1 );
}
#endif
