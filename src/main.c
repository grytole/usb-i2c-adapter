// usb-i2c-adapter / 06-Mar-2016 / grytole@gmail.com

// Write byte to I2C device (command ID = 00h):
//  TX:         [00h] [add] [reg] [val]
//  RX SUCCESS: [01h] [val]
//  RX FAILURE: [00h]

// Read byte from I2C device (command ID = 01h):
//  TX:         [01h] [add] [reg]
//  RX SUCCESS: [01h] [val]
//  RX FAILURE: [00h]

// Detect presence of I2C device (command ID = 02h):
//  TX:         [01h] [add]
//  RX SUCCESS: [01h] [add]
//  RX FAILURE: [00h]

#include "stm8s.h"

#define DEFAULT_UART_BAUDRATE (115200)
#define RESPOND_FAILURE (0x00)
#define RESPOND_SUCCESS (0x01)

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
    uint8_t cmd;
    uint8_t add;
    uint8_t reg;
    uint8_t val;
    uint8_t err;
} adapter = {
    .state = STATE_STARTUP
};

static void adapterInit( void )
{
    CLK_HSIPrescalerConfig( CLK_PRESCALER_HSIDIV1 );
    UART1_DeInit();
    UART1_Init( DEFAULT_UART_BAUDRATE, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO, UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE );
    I2C_DeInit();
    I2C_Init( I2C_MAX_STANDARD_FREQ, 0x0000, I2C_DUTYCYCLE_2, I2C_ACK_CURR, I2C_ADDMODE_7BIT, I2C_MAX_INPUT_FREQ );
    adapter.state = STATE_RECEIVE;
}

static void adapterReceive( void )
{
    while( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) );
    adapter.cmd = UART1_ReceiveData8();
    switch( adapter.cmd )
    {
    case COMMAND_WRITE:
        while( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) );
        adapter.add = UART1_ReceiveData8();
        while( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) );
        adapter.reg = UART1_ReceiveData8();
        while( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) );
        adapter.val = UART1_ReceiveData8();
        break;
    case COMMAND_READ:
        while( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) );
        adapter.add = UART1_ReceiveData8();
        while( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) );
        adapter.reg = UART1_ReceiveData8();
        break;
    case COMMAND_DETECT:
        while( RESET == UART1_GetFlagStatus( UART1_FLAG_RXNE ) );
        adapter.add = UART1_ReceiveData8();
        break;
    default:
        break;
    }
    adapter.state = STATE_EXECUTE;
}

static void adapterExecute( void )
{
    switch( adapter.cmd )
    {
    case COMMAND_WRITE:
        while( RESET != I2C_GetFlagStatus( I2C_FLAG_BUSBUSY ) );
        I2C_GenerateSTART( ENABLE );
        while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_MODE_SELECT ) );
        I2C_Send7bitAddress( adapter.add << 1, I2C_DIRECTION_TX );
        while( ( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED ) ) &&
               ( RESET == I2C_GetFlagStatus( I2C_FLAG_ACKNOWLEDGEFAILURE ) ) );
        if( RESET == I2C_GetFlagStatus( I2C_FLAG_ACKNOWLEDGEFAILURE ) )
        {
            I2C_SendData( adapter.reg );
            while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_BYTE_TRANSMITTING ) );
            I2C_SendData( adapter.val );
            while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_BYTE_TRANSMITTING ) );
            I2C_GenerateSTOP( ENABLE );
            adapter.err = RESPOND_SUCCESS;
        }
        else
        {
            I2C_ClearFlag( I2C_FLAG_ACKNOWLEDGEFAILURE );
            I2C_GenerateSTOP( ENABLE );
            adapter.err = RESPOND_FAILURE;
        }
        break;
    case COMMAND_READ:
        while( RESET != I2C_GetFlagStatus( I2C_FLAG_BUSBUSY ) );
        I2C_GenerateSTART( ENABLE );
        while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_MODE_SELECT ) );
        I2C_Send7bitAddress( adapter.add << 1, I2C_DIRECTION_TX );
        while( ( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED ) ) &&
               ( RESET == I2C_GetFlagStatus( I2C_FLAG_ACKNOWLEDGEFAILURE ) ) );
        if( RESET == I2C_GetFlagStatus( I2C_FLAG_ACKNOWLEDGEFAILURE ) )
        {
            I2C_SendData( adapter.reg );
            while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_BYTE_TRANSMITTING ) );
            I2C_GenerateSTART( ENABLE );
            while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_MODE_SELECT ) );
            I2C_Send7bitAddress( adapter.add << 1, I2C_DIRECTION_RX );
            while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED ) );
            I2C_AcknowledgeConfig( I2C_ACK_NONE );
            I2C_GenerateSTOP( ENABLE );
            while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_BYTE_RECEIVED ) );
            I2C_AcknowledgeConfig( I2C_ACK_CURR );
            adapter.val = I2C_ReceiveData();
            adapter.err = RESPOND_SUCCESS;
        }
        else
        {
            I2C_ClearFlag( I2C_FLAG_ACKNOWLEDGEFAILURE );
            I2C_GenerateSTOP( ENABLE );
            adapter.err = RESPOND_FAILURE;
        }
        break;
    case COMMAND_DETECT:
        while( RESET != I2C_GetFlagStatus( I2C_FLAG_BUSBUSY ) );
        I2C_GenerateSTART( ENABLE );
        while( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_MODE_SELECT ) );
        I2C_Send7bitAddress( adapter.add << 1, I2C_DIRECTION_TX );
        while( ( SUCCESS != I2C_CheckEvent( I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED ) ) &&
               ( RESET == I2C_GetFlagStatus( I2C_FLAG_ACKNOWLEDGEFAILURE ) ) );
        if( RESET == I2C_GetFlagStatus( I2C_FLAG_ACKNOWLEDGEFAILURE ) )
        {
            adapter.val = adapter.add;
            I2C_GenerateSTOP( ENABLE );
            adapter.err = RESPOND_SUCCESS;
        }
        else
        {
            I2C_ClearFlag( I2C_FLAG_ACKNOWLEDGEFAILURE );
            I2C_GenerateSTOP( ENABLE );
            adapter.err = RESPOND_FAILURE;
        }
        break;
    default:
        adapter.err = RESPOND_FAILURE;
        break;
    }
    adapter.state = STATE_RESPOND;
}

static void adapterRespond( void )
{
    while( RESET == UART1_GetFlagStatus( UART1_FLAG_TXE ) );
    UART1_SendData8( adapter.err );
    if( RESPOND_FAILURE != adapter.err )
    {
        while( RESET == UART1_GetFlagStatus( UART1_FLAG_TXE ) );
        UART1_SendData8( adapter.val );
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
