
#include "DXFeed.h"
#include "DXErrorCodes.h"
#include "Logger.h"
#include "pthread.h"
#include "DXMemory.h"
#include "DXThreads.h"
#include <stdio.h>
#include <Windows.h>
#include <math.h>
#include "EventData.h"

const char dxfeed_host[] = "demo.dxfeed.com:7300";

static dxf_const_string_t g_symbols[] = { {L"IBM"}, {L"MSFT"}, {L"YHOO"}, {L"C"} };
static const dxf_int_t g_symbols_size = sizeof (g_symbols) / sizeof (g_symbols[0]);
static const int g_event_type = DXF_ET_TRADE;
static int g_iteration_count = 10;

/* -------------------------------------------------------------------------- */

dxf_const_string_t dx_event_type_to_string(int event_type){
    switch (event_type){
    case DXF_ET_TRADE: return L"Trade"; 
    case DXF_ET_QUOTE: return L"Quote"; 
    case DXF_ET_SUMMARY: return L"Summary"; 
    case DXF_ET_PROFILE: return L"Profile"; 
    case DXF_ET_ORDER: return L"Order";
    case DXF_ET_TIME_AND_SALE: return L"Time&Sale";
    default: return L"";
    }	
}

/* -------------------------------------------------------------------------- */

void process_last_error () {
    int error_code = dx_ec_success;
    dxf_const_string_t error_descr = NULL;
    int res;

    res = dxf_get_last_error(&error_code, &error_descr);

    if (res == DXF_SUCCESS) {
        if (error_code == dx_ec_success) {
            printf("WTF - no error information is stored");

            return;
        }

        wprintf(L"Error occurred and successfully retrieved:\n"
            L"error code = %d, description = \"%s\"\n",
            error_code, error_descr);
        return;
    }

    printf("An error occurred but the error subsystem failed to initialize\n");
}

/* -------------------------------------------------------------------------- */

void listener (int event_type, dxf_const_string_t symbol_name,const dxf_event_data_t* data, int data_count){
    dxf_int_t i = 0;
    dx_event_id_t eid = dx_eid_begin;
    
    for (; (DX_EVENT_BIT_MASK(eid) & event_type) == 0; ++eid);

    wprintf(L"First listener. Event: %s Symbol: %s\n",dx_event_type_to_string(event_type), symbol_name);

    if (event_type == DXF_ET_TRADE) {
        dxf_trade_t* trades = (dx_trade_t*)data;

        for (; i < data_count; ++i) {
            wprintf(L"time=%i, exchange code=%C, price=%f, size=%i, day volume=%f\n",
                    trades[i].time, trades[i].exchange_code, trades[i].price, trades[i].size, trades[i].day_volume);
        }
    }
}


/* -------------------------------------------------------------------------- */

int main (int argc, char* argv[]) {
    dxf_connection_t connection;
    dxf_subscription_t subscription;
    int i = 0;
    bool test_result = true;

    dxf_initialize_logger( "log.log", true, true, true );

    wprintf(L"LastEvent test started.\n");    
    printf("Connecting to host %s...\n", dxfeed_host);

    if (!dxf_create_connection(dxfeed_host, NULL, &connection)) {
        process_last_error();
        return -1;
    }

    wprintf(L"Connection successful!\n");

    wprintf(L"Creating subscription to: Trade\n");
    if (!dxf_create_subscription(connection, g_event_type, &subscription )) {
        process_last_error();

        return -1;
    };

    wprintf(L"Adding symbols:");
    for (i = 0; i < g_symbols_size; ++i) {
        wprintf(L" %S", g_symbols[i]);
    }
    wprintf(L"\n");

    if (!dxf_add_symbols(subscription, g_symbols, g_symbols_size)) {
        process_last_error();

        return -1;
    }; 

    wprintf(L"Listener attached\n");
    if (!dxf_attach_event_listener(subscription, listener)) {
        process_last_error();

        return -1;
    };

    while (g_iteration_count--) {
        wprintf(L"\nLast event data:");
        for (i = 0; i < g_symbols_size; ++i) {
            dxf_event_data_t data;
            dxf_trade_t* trade;
            if (!dxf_get_last_event(connection, g_event_type, g_symbols[i], &data)) {
                return -1;
            }

            if (data) {
                trade = (dxf_trade_t*)data;
                
                wprintf(L"\nSymbol: %s; time=%i, exchange code=%C, price=%f, size=%i, day volume=%f\n",
                        g_symbols[i], trade->time, trade->exchange_code, trade->price, trade->size, trade->day_volume);
                
            }
        }
        wprintf(L"\n");
        Sleep (5000); 
    }

    wprintf(L"\n Test %s \n", test_result ? L"complete successfully" : L"failed");

    wprintf(L"Disconnecting from host...\n");

    if (!dxf_close_connection(connection)) {
        process_last_error();

        return -1;
    }

    wprintf(L"Disconnect successful!\n"
        L"Connection test completed successfully!\n");

    Sleep(5000);

    return 0;
}

