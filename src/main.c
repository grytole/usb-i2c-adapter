// usb-i2c-adapter / 23-Mar-2016 / grytole@gmail.com

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

// Echo payload (command ID = 03h):
//  TX:         03h, numbytes, [byte1, ..., byteN]
//  RX:         numbytes, [byte1, ..., byteN]

#include "stm8s.h"

//#define DEFAULT_UART_BAUDRATE (9600)
#define DEFAULT_UART_BAUDRATE (115200)
//#define DEFAULT_I2C_BITRATE (I2C_MAX_STANDARD_FREQ)
#define DEFAULT_I2C_BITRATE (I2C_MAX_FAST_FREQ)
#define MAX_PAYLOAD_SIZE (255)
#define MIN_DETECT_ADDRESS (0x03)
#define MAX_DETECT_ADDRESS (0x77)
#define RESPOND_FAILURE (0x00)
#define TIMEOUT_1MS (16000)

enum {
    STATE_STARTUP = 0,
    STATE_RECEIVE,
    STATE_EXECUTE,
    STATE_RESPOND
};

enum {
    COMMAND_WRITE = 0,
    COMMAND_READ,
    COMMAND_DETECT,
    COMMAND_ECHO
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

static uint16_t timeout = 0;

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
    I2C_Init( DEFAULT_I2C_BITRATE,
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
        timeout = TIMEOUT_1MS;
        while( ( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) ) && (--timeout) );
        if( 0 == timeout )
        {
            adapter.payloadSize = RESPOND_FAILURE;
            adapter.state = STATE_RESPOND;
            break;
        }
        adapter.deviceAddress = UART1_ReceiveData8();

        timeout = TIMEOUT_1MS;
        while( ( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) ) && (--timeout) );
        if( 0 == timeout )
        {
            adapter.payloadSize = RESPOND_FAILURE;
            adapter.state = STATE_RESPOND;
            break;
        }
        adapter.deviceRegister = UART1_ReceiveData8();

        timeout = TIMEOUT_1MS;
        while( ( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) ) && (--timeout) );
        if( 0 == timeout )
        {
            adapter.payloadSize = RESPOND_FAILURE;
            adapter.state = STATE_RESPOND;
            break;
        }
        adapter.payloadSize = UART1_ReceiveData8();

        for( uint8_t i = 0; i < adapter.payloadSize; i++ )
        {
            timeout = TIMEOUT_1MS;
            while( ( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) ) && (--timeout) );
            if( 0 == timeout )
            {
                adapter.payloadSize = RESPOND_FAILURE;
                adapter.state = STATE_RESPOND;
                break;
            }
            adapter.payload[ i ] = UART1_ReceiveData8();
        }
        break;
    case COMMAND_READ:
        timeout = TIMEOUT_1MS;
        while( ( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) ) && (--timeout) );
        if( 0 == timeout )
        {
            adapter.payloadSize = RESPOND_FAILURE;
            adapter.state = STATE_RESPOND;
            break;
        }
        adapter.deviceAddress = UART1_ReceiveData8();

        timeout = TIMEOUT_1MS;
        while( ( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) ) && (--timeout) );
        if( 0 == timeout )
        {
            adapter.payloadSize = RESPOND_FAILURE;
            adapter.state = STATE_RESPOND;
            break;
        }
        adapter.deviceRegister = UART1_ReceiveData8();

        timeout = TIMEOUT_1MS;
        while( ( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) ) && (--timeout) );
        if( 0 == timeout )
        {
            adapter.payloadSize = RESPOND_FAILURE;
            adapter.state = STATE_RESPOND;
            break;
        }
        adapter.payloadSize = UART1_ReceiveData8();
        break;
    case COMMAND_DETECT:
        timeout = TIMEOUT_1MS;
        while( ( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) ) && (--timeout) );
        if( 0 == timeout )
        {
            adapter.payloadSize = RESPOND_FAILURE;
            adapter.state = STATE_RESPOND;
            break;
        }
        adapter.payloadSize = UART1_ReceiveData8();

        for( uint8_t i = 0; i < adapter.payloadSize; i++ )
        {
            timeout = TIMEOUT_1MS;
            while( ( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) ) && (--timeout) );
            if( 0 == timeout )
            {
                adapter.payloadSize = RESPOND_FAILURE;
                adapter.state = STATE_RESPOND;
                break;
            }
            adapter.payload[ i ] = UART1_ReceiveData8();
        }
        break;
    case COMMAND_ECHO:
        timeout = TIMEOUT_1MS;
        while( ( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) ) && (--timeout) );
        if( 0 == timeout )
        {
            adapter.payloadSize = RESPOND_FAILURE;
            adapter.state = STATE_RESPOND;
            break;
        }
        adapter.payloadSize = UART1_ReceiveData8();

        for( uint8_t i = 0; i < adapter.payloadSize; i++ )
        {
            timeout = TIMEOUT_1MS;
            while( ( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) ) && (--timeout) );
            if( 0 == timeout )
            {
                adapter.payloadSize = RESPOND_FAILURE;
                adapter.state = STATE_RESPOND;
                break;
            }
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
        timeout = TIMEOUT_1MS;
        while( ( RESET != I2C_GetFlagStatus( I2C_FLAG_BUSBUSY ) ) && (--timeout) );
        if( 0 == timeout )
        {
            adapter.payloadSize = RESPOND_FAILURE;
            adapter.state = STATE_RESPOND;
            break;
        }
        I2C_GenerateSTART( ENABLE );

        timeout = TIMEOUT_1MS;
        while( ( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_MODE_SELECT ) ) && (--timeout) );
        if( 0 == timeout )
        {
            adapter.payloadSize = RESPOND_FAILURE;
            adapter.state = STATE_RESPOND;
            I2C_GenerateSTOP( ENABLE );
            break;
        }
        I2C_Send7bitAddress( adapter.deviceAddress << 1, I2C_DIRECTION_TX );

        timeout = TIMEOUT_1MS;
        while( ( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED ) ) &&
               ( RESET == I2C_GetFlagStatus( I2C_FLAG_ACKNOWLEDGEFAILURE ) ) &&
               ( --timeout ) );
        if( RESET == I2C_GetFlagStatus( I2C_FLAG_ACKNOWLEDGEFAILURE ) )
        {
            I2C_SendData( adapter.deviceRegister );

            timeout = TIMEOUT_1MS;
            while( ( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_BYTE_TRANSMITTING ) ) && (--timeout) );
            if( 0 == timeout )
            {
                adapter.payloadSize = RESPOND_FAILURE;
                adapter.state = STATE_RESPOND;
                I2C_GenerateSTOP( ENABLE );
                break;
            }
            for( uint8_t i = 0; i < adapter.payloadSize; i++ )
            {
                I2C_SendData( adapter.payload[ i ] );

                timeout = TIMEOUT_1MS;
                while( ( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_BYTE_TRANSMITTING ) ) && (--timeout) );
                if( 0 == timeout )
                {
                    adapter.payloadSize = RESPOND_FAILURE;
                    adapter.state = STATE_RESPOND;
                    I2C_GenerateSTOP( ENABLE );
                    break;
                }
                if( ( adapter.payloadSize - 1 ) == i )
                {
                    I2C_GenerateSTOP( ENABLE );
                }
            }
            adapter.payload[ 0 ] = adapter.payloadSize;
            adapter.payloadSize = 1;
        }
        else
        {
            I2C_ClearFlag( I2C_FLAG_ACKNOWLEDGEFAILURE );
            I2C_GenerateSTOP( ENABLE );
            adapter.payloadSize = RESPOND_FAILURE;
        }
        break;
    case COMMAND_READ:
        timeout = TIMEOUT_1MS;
        while( ( RESET != I2C_GetFlagStatus( I2C_FLAG_BUSBUSY ) ) && (--timeout) );
        if( 0 == timeout )
        {
            adapter.payloadSize = RESPOND_FAILURE;
            adapter.state = STATE_RESPOND;
            break;
        }
        I2C_GenerateSTART( ENABLE );

        timeout = TIMEOUT_1MS;
        while( ( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_MODE_SELECT ) ) && (--timeout) );
        I2C_Send7bitAddress( adapter.deviceAddress << 1, I2C_DIRECTION_TX );

        timeout = TIMEOUT_1MS;
        while( ( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED ) ) &&
               ( RESET == I2C_GetFlagStatus( I2C_FLAG_ACKNOWLEDGEFAILURE ) ) &&
               (--timeout) );
        if( RESET == I2C_GetFlagStatus( I2C_FLAG_ACKNOWLEDGEFAILURE ) )
        {
            I2C_SendData( adapter.deviceRegister );

            timeout = TIMEOUT_1MS;
            while( ( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_BYTE_TRANSMITTING ) ) && (--timeout) );
            if( 0 == timeout )
            {
                adapter.payloadSize = RESPOND_FAILURE;
                adapter.state = STATE_RESPOND;
                break;
            }
            I2C_GenerateSTART( ENABLE );

            timeout = TIMEOUT_1MS;
            while( ( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_MODE_SELECT ) ) && (--timeout) );
            I2C_Send7bitAddress( adapter.deviceAddress << 1, I2C_DIRECTION_RX );

            timeout = TIMEOUT_1MS;
            while( ( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED ) ) && (--timeout) );
            for( uint8_t i = 0; i < adapter.payloadSize; i++ )
            {
                if( ( adapter.payloadSize - 1 ) == i )
                {
                    I2C_AcknowledgeConfig( I2C_ACK_NONE );
                }

                timeout = TIMEOUT_1MS;
                while( ( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_BYTE_RECEIVED ) ) && (--timeout) );
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
            timeout = TIMEOUT_1MS;
            while( ( RESET != I2C_GetFlagStatus( I2C_FLAG_BUSBUSY ) ) && (--timeout) );
            if( 0 == timeout )
            {
                adapter.payloadSize = RESPOND_FAILURE;
                adapter.state = STATE_RESPOND;
                break;
            }
            I2C_GenerateSTART( ENABLE );

            timeout = TIMEOUT_1MS;
            while( (SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_MODE_SELECT ) ) && (--timeout) );
            if( 0 == timeout )
            {
                adapter.payloadSize = RESPOND_FAILURE;
                adapter.state = STATE_RESPOND;
                I2C_GenerateSTOP( ENABLE );
                break;
            }
            I2C_Send7bitAddress( adapter.payload[ i ] << 1, I2C_DIRECTION_TX );

            timeout = TIMEOUT_1MS;
            while( ( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED ) ) &&
                   ( RESET == I2C_GetFlagStatus( I2C_FLAG_ACKNOWLEDGEFAILURE ) ) &&
                   (--timeout) );
            if( 0 == timeout )
            {
                adapter.payloadSize = RESPOND_FAILURE;
                adapter.state = STATE_RESPOND;
                I2C_GenerateSTOP( ENABLE );
                break;
            }

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
    case COMMAND_ECHO:
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
