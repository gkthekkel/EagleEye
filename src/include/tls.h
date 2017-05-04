/*
 *	
 * Copyright (c) 2016 Cisco Systems, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following
 *   disclaimer in the documentation and/or other materials provided
 *   with the distribution.
 * 
 *   Neither the name of the Cisco Systems, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * \file tls.h
 *
 * \brief header file for TLS functionality
 */

#ifndef TLS_H
#define TLS_H

#include <pcap.h>
#include "output.h"
#include "utils.h"
#include "fingerprint.h"

/** usage string for tls */
#define tls_usage "  tls=1                      report TLS data (ciphersuites, record lengths and times, ...)\n"

/** tls filter key */
#define tls_filter(key) ((key->dp == 443 || key->sp == 443))

/* constants for TLS awareness */
#define MAX_CS 256
#define MAX_EXTENSIONS 256
#define MAX_SID_LEN 256
#define MAX_NUM_RCD_LEN 200
#define OUT "<"
#define IN  ">"
#define NUM_PKT_LEN_TLS 50
#define MAX_CERTIFICATES 4
#define MAX_RDN 12
#define MAX_SAN 12
#define MAX_CERT_EXTENSIONS 12
#define MAX_CERTIFICATE_BUFFER 11000
#define MAX_CKE_LEN 1024
/* The maimum size of string that we allow from OpenSSL */
#define MAX_OPENSSL_STRING 32

/** \remarks \verbatim
  structure for TLS awareness 

  From RFC 5246:

      struct {
          ContentType type;
          ProtocolVersion version;
          uint16 length;
          opaque fragment[TLSPlaintext.length];
      } TLSPlaintext;

      struct {
         HandshakeType msg_type;    
         uint24 length;             
         select (HandshakeType) {
            case hello_request:       HelloRequest;
            case client_hello:        ClientHello;
            case server_hello:        ServerHello;
            case certificate:         Certificate;
            case server_key_exchange: ServerKeyExchange;
            case certificate_request: CertificateRequest;
            case server_hello_done:   ServerHelloDone;
            case certificate_verify:  CertificateVerify;
            case client_key_exchange: ClientKeyExchange;
            case finished:            Finished;
         } body;
      } Handshake;

      struct {
          ProtocolVersion client_version;
          Random random;
          SessionID session_id;
          CipherSuite cipher_suites<2..2^16-2>;
          CompressionMethod compression_methods<1..2^8-1>;
          select (extensions_present) {
              case false:
                  struct {};
              case true:
                  Extension extensions<0..2^16-1>;
          };
      } ClientHello;

      struct {
          uint16 msg_length;
          uint8 msg_type;
          Version version;
          uint16 cipher_spec_length;
          uint16 session_id_length;
          uint16 challenge_length;
          V2CipherSpec cipher_specs[V2ClientHello.cipher_spec_length];
          opaque session_id[V2ClientHello.session_id_length];
          opaque challenge[V2ClientHello.challenge_length;
      } V2ClientHello;
 \endverbatim
 */

/*
 * Structures for storing relevant TLS information
 */

struct tls_type_code {
    unsigned char content;
    unsigned char handshake;
};

struct tls_extension {
    unsigned short int type;
    unsigned short int length;
    void *data;
};

struct tls_item_entry {
    char id[MAX_OPENSSL_STRING]; /**< Identification (string) */
    unsigned char *data; /**< Data encapsulated within the item */
    uint16_t data_length; /**< Length of the data in bytes */
};

struct tls_certificate {
    unsigned short length;
    unsigned char *serial_number; /**< Serial Number */
    uint8_t serial_number_length; /**< Length of the serial number in bytes */
    unsigned char *signature; /**< Signature */
    uint16_t signature_length; /**< Length of the signature in bytes */
    char signature_algorithm[MAX_OPENSSL_STRING]; /**< Signature algorithm (string) */
    uint16_t signature_key_size; /**< Length of the signature key in bits */
    struct tls_item_entry issuer[MAX_RDN]; /**< Array of item entries corresponding
                                                to the issuer information */
    uint8_t num_issuer_items;
    struct tls_item_entry subject[MAX_RDN]; /**< Array of item entries corresponding
                                                 to the subject information */
    uint8_t num_subject_items;
    struct tls_item_entry extensions[MAX_CERT_EXTENSIONS]; /**< Array of item entries corresponding
                                                                to the extension information */
    uint8_t num_extension_items;
    unsigned char *validity_not_before;
    uint16_t validity_not_before_length;
    unsigned char *validity_not_after;
    uint16_t validity_not_after_length;
    char subject_public_key_algorithm[MAX_OPENSSL_STRING]; /**< Subject public key algorithm (string) */
    uint16_t subject_public_key_size; /**< Length of the subject public key in bits */
};

typedef struct tls_information {
    enum role role; /**< client, server, or unknown */
    unsigned int   tls_op;
    unsigned short tls_len[MAX_NUM_RCD_LEN]; /**< TLS record lengths */
    struct timeval tls_time[MAX_NUM_RCD_LEN]; /**< Arrival times */
    struct tls_type_code tls_type[MAX_NUM_RCD_LEN]; /**< Record type codes */
    unsigned short int num_ciphersuites; /**< Number of ciphersuites */
    unsigned short int ciphersuites[MAX_CS]; /**< Ciphersuites */
    unsigned short int num_tls_extensions; /**< Number of extensions */
    unsigned short int num_server_tls_extensions; /**< Number of server extensions */
    struct tls_extension tls_extensions[MAX_EXTENSIONS]; /**< Extensions */
    struct tls_extension server_tls_extensions[MAX_EXTENSIONS]; /**< Extensions of server */
    unsigned char tls_v; /**< TLS version */
    unsigned int tls_client_key_length; /**< clientKeyExchange key length */
    unsigned char clientKeyExchange[MAX_CKE_LEN]; /**< clientKeyExchange data */
    unsigned char tls_sid_len; /**< Session ID length */
    unsigned char tls_sid[MAX_SID_LEN]; /**< Session ID */
    unsigned char tls_random[32]; /**< Random field from hello */
    struct tls_certificate certificates[MAX_CERTIFICATES]; /**< X.509 certificates */
    unsigned char num_certificates; /**< Number of certificates */
    unsigned char start_cert;
    void *sni; /**< SNI a.k.a Server name indication */
    unsigned short int sni_length; /**< Length of SNI */
    void *certificate_buffer; /**< Certificate(s) data */
    unsigned short certificate_offset;
    fingerprint_t *tls_fingerprint;
} tls_t;


/*
 * Structures for parsing TLS content
 */

struct tls_protocol_version {
    unsigned char major;
    unsigned char minor;
};


struct tls_ciphertext {
    struct tls_protocol_version protocol_version;
    unsigned char lengthMid;
    unsigned char lengthLo;
};

struct tls_handshake {
    unsigned char msg_type; /**< Handshake message type */
    unsigned char lengthHi; /**< First byte of Handshake length (big endian) */
    unsigned char lengthMid; /**< Middle byte of Handshake length (big endian) */
    unsigned char lengthLo; /**< Last byte of Handshake length (big endian) */
    unsigned char body; /**< Body, a.k.a payload of the message */
};

struct tls_header {
    unsigned char content_type;
    struct tls_protocol_version protocol_version;
    unsigned char lengthMid;
    unsigned char lengthLo;
    union {
        struct tls_ciphertext ciphertext;
        struct tls_handshake handshake;
    };
};

struct tls_random {
    unsigned int gmt_unix_time;
    unsigned char random_bytes[28];
};

struct tls_client_hello {
    struct tls_protocol_version protocol_version;
    struct tls_random random;
    unsigned char session_id_length;
    uint16_t count_cipher_suites;
};

/*
 * @brief Enumeration representing TLS versions internal to Joy.
 */
enum tls_version {
    TLS_VERSION_UNKNOWN = 0,
    TLS_VERSION_SSLV2 = 1,
    TLS_VERSION_SSLV3 = 2,
    TLS_VERSION_1_0 = 3,
    TLS_VERSION_1_1 = 4,
    TLS_VERSION_1_2 = 5,
    TLS_VERSION_1_3 = 6
};

/*
 * @brief Enumeration representing TLS HandshakeTypes.
 */
enum tls_handshake_type {
    TLS_HANDSHAKE_HELLO_REQUEST = 0,
    TLS_HANDSHAKE_CLIENT_HELLO = 1,
    TLS_HANDSHAKE_SERVER_HELLO = 2,
    TLS_HANDSHAKE_CERTIFICATE = 11,
    TLS_HANDSHAKE_SERVER_KEY_EXCHANGE = 12,
    TLS_HANDSHAKE_CERTIFICATE_REQUEST = 13,
    TLS_HANDSHAKE_SERVER_HELLO_DONE = 14,
    TLS_HANDSHAKE_CERTIFICATE_VERIFY = 15,
    TLS_HANDSHAKE_CLIENT_KEY_EXCHANGE = 16,
    TLS_HANDSHAKE_FINISHED = 20
};

/*
 * @brief Enumeration representing TLS ContentTypes.
 */
enum tls_content_type {
    TLS_CONTENT_CHANGE_CIPHER_SPEC = 20,
    TLS_CONTENT_ALERT = 21,
    TLS_CONTENT_HANDSHAKE = 22,
    TLS_CONTENT_APPLICATION_DATA = 23
};

/*
 * TLS module public functions
 */

/** initialize TLS structure */
void tls_init(struct tls_information *r);

/** free data associated with TLS record */
void tls_delete(struct tls_information *r);

/** process TLS packet for consumption */
void tls_update(struct tls_information *r,
                const struct pcap_pkthdr *header,
                const void *data,
                unsigned int data_len,
                unsigned int report_tls);

/** print out the TLS information to the destination file */
void tls_print_json(const struct tls_information *data, const struct tls_information *data_twin, zfile f);

void tls_unit_test();

int tls_load_fingerprints(void);

#endif /* TLS_H */

