#include "app.h"
#include "bsp.h"
#include "u2f.h"

#ifndef U2F_DISABLE


#ifdef DEBUG_PC
#include <stdio.h>
#else
#define printf(x)
#endif

// void u2f_response_writeback(uint8_t * buf, uint8_t len);
static int16_t u2f_register(struct u2f_register_request * req);

void u2f_request(struct u2f_request_apdu * req)
{
    uint16_t * rcode = (uint16_t *)req;
    u2f_response_start();
    switch(req->ins)
    {
        case U2F_REGISTER:
            *rcode = u2f_register((struct u2f_register_request*)req->payload);
            break;
        case U2F_AUTHENTICATE:
        case U2F_VERSION:
        case U2F_VENDOR_FIRST:
        case U2F_VENDOR_LAST:
        default:

        	break;
    }
    u2f_response_writeback((uint8_t*)rcode,2);
    u2f_response_flush();
}

static int16_t u2f_register(struct u2f_register_request * req)
{
    uint8_t i[] = {0x5,U2F_EC_FMT_UNCOMPRESSED};

    uint8_t key_handle[U2F_KEY_HANDLE_SIZE];
    uint8_t pubkey[64];

    u2f_prints("u2f_register\r\n");

    if (u2f_get_user_feedback() != 0)
    {
    	u2f_prints("u2f_get_user_feedback fail\r\n");
        return U2F_SW_CONDITIONS_NOT_SATISFIED;
    }

    u2f_prints("u2f_new_keypair\r\n");
    if ( u2f_new_keypair(key_handle, pubkey) == -1)
    {
    	u2f_prints("u2f_new_keypair fail\r\n");
    	return U2F_SW_CONDITIONS_NOT_SATISFIED;
    }

    u2f_prints("u2f_sha\r\n");
    u2f_sha256_start();
    u2f_sha256_update(i,1);
    u2f_sha256_update(req->app,32);
    u2f_sha256_update(req->chal,32);
    u2f_sha256_update(key_handle,U2F_KEY_HANDLE_SIZE);
    u2f_sha256_update(i+1,1);
    u2f_sha256_update(pubkey,64);
    u2f_sha256_finish();
    
    u2f_prints("u2f_ecdsa_sign\r\n");

    if (u2f_ecdsa_sign((uint8_t*)req, U2F_ATTESTATION_HANDLE) == -1)
	{
    	return SW_WRONG_DATA;
	}
    u2f_prints("u2f_response_writeback\r\n");
    u2f_hid_set_len(131 + U2F_KEY_HANDLE_SIZE + u2f_attestation_cert_size());
    u2f_response_writeback(i,2);
    u2f_response_writeback(pubkey,64);
    i[0] = U2F_KEY_HANDLE_SIZE;
    u2f_response_writeback(i,1);
    u2f_response_writeback(key_handle,U2F_KEY_HANDLE_SIZE);
    u2f_response_writeback(u2f_get_attestation_cert(),u2f_attestation_cert_size()-254/2);
    u2f_response_writeback(u2f_get_attestation_cert()+u2f_attestation_cert_size(),254/2);

    u2f_response_writeback((uint8_t*)req, 64);

    return U2F_SW_NO_ERROR;
}

#endif
