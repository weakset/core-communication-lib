#include "spark_protocol.h"
#include "handshake.h"
#include <string.h>

int SparkProtocol::init(const unsigned char *private_key,
                        const unsigned char *pubkey,
                        const unsigned char *encrypted_credentials,
                        const unsigned char *signature)
{
  unsigned char credentials[40];
  unsigned char hmac[20];

  if (0 != decipher_aes_credentials(private_key, encrypted_credentials, credentials))
    return 1;

  calculate_ciphertext_hmac(encrypted_credentials, credentials, hmac);

  if (0 == verify_signature(signature, pubkey, hmac))
  {
    memcpy(key,  credentials,      16);
    memcpy(iv,   credentials + 16, 16);
    memcpy(salt, credentials + 32,  8);
    return 0;
  }
  else return 2;
}

CoAPMessageType::Enum
  SparkProtocol::received_message(unsigned char *buf)
{
  aes_setkey_dec(&aes, key, 128);
  aes_crypt_cbc(&aes, AES_DECRYPT, 32, iv, buf, buf);

  switch (CoAP::code(buf))
  {
    case CoAPCode::GET:
      return CoAPMessageType::VARIABLE_REQUEST;
    case CoAPCode::POST:
      switch (buf[6])
      {
        case 'f': return CoAPMessageType::FUNCTION_CALL;
        case 'u': return CoAPMessageType::UPDATE_BEGIN;
        case 'c': return CoAPMessageType::CHUNK;
        default: return CoAPMessageType::ERROR;
      }
    case CoAPCode::PUT:
      return CoAPMessageType::KEY_CHANGE;
    default:
      return CoAPMessageType::ERROR;
  }
}