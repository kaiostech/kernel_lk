/*
 * Copyright (c) 2014-2017, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdint.h>
#include <crypto_hash.h>
#include <boot_verifier.h>
#include <image_verify.h>
#include <mmc.h>
#include <oem_keystore.h>
#include <openssl/asn1t.h>
#include <openssl/x509.h>
#include <partition_parser.h>
#include <rsa.h>
#include <string.h>
#include <openssl/err.h>
#include <platform.h>
#include <qseecom_lk_api.h>
#include <secapp_loader.h>
#include <target.h>
#include <boot_stats.h>




void send_rot_command_minimal()
{
    int ret = 0;
    unsigned char *input = NULL;
    unsigned int digest[9] = {0}, final_digest[8] = {0};
    uint32_t auth_algo = CRYPTO_AUTH_ALG_SHA256;
    int app_handle = 0;
    km_set_rot_req_t *read_req;
    km_set_rot_rsp_t read_rsp;
    app_handle = get_secapp_handle();
    // Unlocked device and no verification done.
    // Send the hash of boot device state
    input = NULL;
    digest[0] = 1;
    hash_find((unsigned char *) digest, sizeof(digest), (unsigned char *)&final_digest, auth_algo);
    dprintf(SPEW, "Digest: ");
    for(uint8_t i = 0; i < 8; i++)
        dprintf(SPEW, "0x%x ", final_digest[i]);
    dprintf(SPEW, "\n");
    if(!(read_req = malloc(sizeof(km_set_rot_req_t) + sizeof(final_digest))))
    {
        dprintf(CRITICAL, "Failed to allocate memory for ROT structure\n");
        ASSERT(0);
    }

    void *cpy_ptr = (uint8_t *) read_req + sizeof(km_set_rot_req_t);
    // set ROT stucture
    read_req->cmd_id = KEYMASTER_SET_ROT;
    read_req->rot_ofset = (uint32_t) sizeof(km_set_rot_req_t);
    read_req->rot_size  = sizeof(final_digest);
    // copy the digest
    memcpy(cpy_ptr, (void *) &final_digest, sizeof(final_digest));
    dprintf(SPEW, "Sending Root of Trust to trustzone: start\n");

    ret = qseecom_send_command(app_handle, (void*) read_req, sizeof(km_set_rot_req_t) + sizeof(final_digest), (void*) &read_rsp, sizeof(read_rsp));
    if (ret < 0 || read_rsp.status < 0)
    {
        dprintf(CRITICAL, "QSEEcom command for Sending Root of Trust returned error: %d\n", read_rsp.status);
        if(input)
            free(input);
        free(read_req);
        return;
    }
    dprintf(SPEW, "Sending Root of Trust to trustzone: end\n");
    if(input)
        free(input);
    free(read_req);
}

