/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file defines types, objects, and methods for working with
 *      Elliptic Curve (EC) Cryptography (ECC) Diffie-Hellman (DH) and
 *      digital signature algorithms (DSA) public key infrastructure,
 *      including keys and signatures.
 *
 */

#ifndef ELLIPTICCURVE_H_
#define ELLIPTICCURVE_H_

#include "WeaveCrypto.h"
#include "HashAlgos.h"
#include <Weave/Support/ASN1.h>

#if WEAVE_CONFIG_USE_OPENSSL_ECC && !WEAVE_WITH_OPENSSL
#error "INVALID WEAVE CONFIG: OpenSSL ECC implementation enabled but OpenSSL not available (WEAVE_CONFIG_USE_OPENSSL_ECC == 1 && WEAVE_WITH_OPENSSL == 0)."
#endif

#if WEAVE_WITH_OPENSSL
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#endif

#define WEAVE_IS_ECJPAKE_ENABLED  ( WEAVE_CONFIG_SUPPORT_PASE_CONFIG2 || \
                                    WEAVE_CONFIG_SUPPORT_PASE_CONFIG3 || \
                                    WEAVE_CONFIG_SUPPORT_PASE_CONFIG4 || \
                                    WEAVE_CONFIG_SUPPORT_PASE_CONFIG5 )

#if WEAVE_CONFIG_USE_MICRO_ECC
#define uECC_WORD_SIZE 4
#define uECC_ENABLE_VLI_API 1
#include <micro-ecc/uECC.h>
#include <micro-ecc/uECC_vli.h>
#endif

#if WEAVE_CONFIG_USE_OPENSSL_ECC && WEAVE_IS_ECJPAKE_ENABLED
struct ECJPAKE_CTX;
#endif

namespace nl {
namespace Weave {
namespace Crypto {

using nl::Weave::ASN1::OID;

class EncodedECPublicKey
{
public:
    uint8_t *ECPoint;                   // X9.62 format
    uint16_t ECPointLen;

    bool IsEqual(const EncodedECPublicKey& other) const;
};

class EncodedECDSASignature
{
public:
    enum
    {
        kMaxValueLength = ((WEAVE_CONFIG_MAX_EC_BITS + 7) / 8) + 1
    };

    uint8_t *R;                         // ASN.1 DER Integer value format
    uint8_t *S;                         // ASN.1 DER Integer value format
    uint8_t RLen;
    uint8_t SLen;

    bool IsEqual(const EncodedECDSASignature& other) const;
};

class EncodedECPrivateKey
{
public:
    uint8_t *PrivKey;                   // Integer in big-endian format
    uint16_t PrivKeyLen;

    bool IsEqual(const EncodedECPrivateKey& other) const;
};

#if WEAVE_CONFIG_USE_MICRO_ECC
enum
{
    kuECC_WordSize                = uECC_WORD_SIZE,
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP256R1
    kuECC_MaxWordCount            = 8,
#elif WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP224R1
    kuECC_MaxWordCount            = 7,
#else // SECP192R1 or SECP160R1
    kuECC_MaxWordCount            = 6,
#endif
    kuECC_MaxByteCount            = kuECC_MaxWordCount * kuECC_WordSize,
};
#endif // WEAVE_CONFIG_USE_MICRO_ECC

enum X963EncodedPointFormat
{
    kX963EncodedPointFormat_PointAtInfinity       = 0x00,
    kX963EncodedPointFormat_Compressed_EvenY      = 0x02,
    kX963EncodedPointFormat_Compressed_OddY       = 0x03,
    kX963EncodedPointFormat_Uncompressed          = 0x04,
    kX963EncodedPointFormat_Hybrid_EvenY          = 0x06,
    kX963EncodedPointFormat_Hybrid_OddY           = 0x07,
};


// =============================================================
// Primary elliptic curve functions used by Weave security code.
// =============================================================

/**
 * Get Elliptic Curve size (in bytes).
 *
 * @param[in]  curveOID     Specified Elliptic Curve OID.
 *
 * @retval     Returns 0 if specified curveOID is unsupported Elliptic Curve.
 *             Otherwise, returns curve size in bytes.
 *
 */
extern int GetCurveSize(const OID curveOID);

extern WEAVE_ERROR GenerateECDSASignature(OID curveOID,
                                          const uint8_t *msgHash, uint8_t msgHashLen,
                                          const EncodedECPrivateKey& encodedPrivKey,
                                          EncodedECDSASignature& encodedSig);

extern WEAVE_ERROR VerifyECDSASignature(OID curveOID,
                                        const uint8_t *msgHash, uint8_t msgHashLen,
                                        const EncodedECDSASignature& encodedSig,
                                        const EncodedECPublicKey& encodedPubKey);

extern WEAVE_ERROR GenerateECDSASignature(OID curveOID,
                                          const uint8_t *msgHash, uint8_t msgHashLen,
                                          const EncodedECPrivateKey& encodedPrivKey,
                                          uint8_t *fixedLenSig);

extern WEAVE_ERROR VerifyECDSASignature(OID curveOID,
                                        const uint8_t *msgHash, uint8_t msgHashLen,
                                        const uint8_t *fixedLenSig,
                                        const EncodedECPublicKey& encodedPubKey);

extern WEAVE_ERROR GenerateECDHKey(OID curveOID, EncodedECPublicKey& encodedPubKey, EncodedECPrivateKey& encodedPrivKey);

extern WEAVE_ERROR ECDHComputeSharedSecret(OID curveOID, const EncodedECPublicKey& encodedPubKey, const EncodedECPrivateKey& encodedPrivKey,
                                           uint8_t *sharedSecretBuf, uint16_t sharedSecretBufSize, uint16_t& sharedSecretLen);

extern WEAVE_ERROR GetCurveG(OID curveOID, EncodedECPublicKey& encodedPubKey);

// ============================================================
// OpenSSL-specific elliptic curve utility functions.
// ============================================================

#if WEAVE_WITH_OPENSSL

extern int GetCurveSize(const OID curveOID, const EC_GROUP *ecGroup);
extern WEAVE_ERROR GetECGroupForCurve(OID curveOID, EC_GROUP *& ecGroup);

extern WEAVE_ERROR ECDHComputeSharedSecret(OID curveOID, const EC_GROUP *ecGroup,
                                           const EC_POINT *pubKeyPoint, const BIGNUM *privKeyBN,
                                           uint8_t *sharedSecretBuf, uint16_t sharedSecretBufSize, uint16_t& sharedSecretLen);

extern WEAVE_ERROR EncodeX962ECPoint(OID curveOID, EC_GROUP *ecGroup, const EC_POINT *point, uint8_t *buf, uint16_t bufSize, uint16_t& encodedPointLen);
extern WEAVE_ERROR DecodeX962ECPoint(const uint8_t *encodedPoint, uint16_t encodedPointLen, EC_GROUP *group, EC_POINT *& point);
extern WEAVE_ERROR DecodeX962ECPoint(const uint8_t *encodedPoint, uint16_t encodedPointLen, BIGNUM *& x, BIGNUM *& y);

extern WEAVE_ERROR DecodeECKey(OID curveOID, const EncodedECPrivateKey *encodedPrivKey, const EncodedECPublicKey *encodedPubKey, EC_KEY *& ecKey);

extern WEAVE_ERROR EncodeECDSASignature(const ECDSA_SIG *sig, EncodedECDSASignature& encodedSig);
extern WEAVE_ERROR DecodeECDSASignature(const EncodedECDSASignature& encodedSig, ECDSA_SIG *& sig);

// EC Utility Functions with Fixed-Length ECDSA Signature Parameter
extern WEAVE_ERROR ECDSASigToFixedLenSig(OID curveOID, const ECDSA_SIG *ecSig, uint8_t *fixedLenSig);
extern WEAVE_ERROR FixedLenSigToECDSASig(OID curveOID, const uint8_t *fixedLenSig, ECDSA_SIG *& ecSig);

#endif // WEAVE_WITH_OPENSSL


// ============================================================
// Elliptic Curve JPAKE Class Declaration
// ============================================================

#if WEAVE_IS_ECJPAKE_ENABLED

#if WEAVE_CONFIG_USE_MICRO_ECC

class ECJPAKEStepPart
{
public:
    uECC_word_t *Gx;                     /* G*x in step 1, G*(xa + xc + xd) in step 2 */
                                         /* ZKP(x) or ZKP(xb * s) */
    uECC_word_t *Gr;                     /* ZKP: G*r (r random) */
    uECC_word_t *b;                      /* ZKP: b = r - x*h, h=hash(G, G*r, G*x, name) */
};

enum
{
    kECJPAKE_HashLength           = Platform::Security::SHA256::kHashLength,
    kECJPAKE_MaxPasswordLength    = 48,
    kECJPAKE_MaxNameLength        = 43,
};

// This check is needed because of the current limitation of uECC_vli_mmod function
#if kECJPAKE_HashLength > 2 * kuECC_MaxByteCount
#error "kECJPAKE_HashLength should be less or equal to (2 * kuECC_MaxByteCount)"
#endif

// This check is needed because of the current limitation of uECC_vli_mmod function
#if kECJPAKE_MaxPasswordLength > 2 * kuECC_MaxByteCount
#error "kECJPAKE_MaxPasswordLength should be less or equal to (2 * kuECC_MaxByteCount)"
#endif

typedef uint32_t EccPoint[2 * kuECC_MaxWordCount];

/* Points addition: result = left + right */
extern void uECC_point_add(uECC_word_t *result,
                           const uECC_word_t *left,
                           const uECC_word_t *right,
                           uECC_Curve curve);

#endif // WEAVE_CONFIG_USE_MICRO_ECC


class EllipticCurveJPAKE
{
private:
#if WEAVE_CONFIG_USE_OPENSSL_ECC
    struct ECJPAKE_CTX *ECJPAKECtx;
#endif // WEAVE_CONFIG_USE_OPENSSL_ECC

#if WEAVE_CONFIG_USE_MICRO_ECC
/* In the definition, (xa, xb, xc, xd) are Alice's (x1, x2, x3, x4)
 *                                      or   Bob's (x3, x4, x1, x2) */
    union
    {
        uECC_word_t Xb[kuECC_MaxWordCount];        /* Alice's x2 or Bob's x4 */
        uint8_t SharedSecret[kECJPAKE_HashLength]; /* Calculated Shared Secret */
    };
    uECC_word_t XbS[kuECC_MaxWordCount];           /* Alice's (x2 * secret) or Bob's (x4 * secret) */
                                                   /* XbS field should be initialized with secret in VLI format */
    EccPoint Gxd;                                  /* Alice's G*x4 or Bob's G*x2 */
    EccPoint Gxacd;                                /* Alice's G*(x1+x3+x4) or Bob's G*(x3+x1+x2) */
    EccPoint Gxabc;                                /* Alice's G*(x1+x2+x3) or Bob's G*(x3+x4+x1) */
    uint16_t LocalNameLen;
    uint16_t PeerNameLen;
    uint8_t LocalName[kECJPAKE_MaxNameLength];     /* Must be unique */
    uint8_t PeerName[kECJPAKE_MaxNameLength];
    uECC_Curve Curve;                              /* Elliptic Curve group */
#endif // WEAVE_CONFIG_USE_MICRO_ECC

public:
    void Init(void);
    void Shutdown(void);
    void Reset(void);
    WEAVE_ERROR Init(const OID curveOID, const uint8_t *pw, const uint16_t pwLen,
                                         const uint8_t *localName, const uint16_t localNameLen,
                                         const uint8_t *peerName, const uint16_t peerNameLen);
    WEAVE_ERROR GenerateStep1(const uint8_t *buf, const uint16_t bufSize, uint16_t& stepDataLen);
    WEAVE_ERROR ProcessStep1(const uint8_t *buf, const uint16_t bufSize, uint16_t& stepDataLen);
    WEAVE_ERROR GenerateStep2(const uint8_t *buf, const uint16_t bufSize, uint16_t& stepDataLen);
    WEAVE_ERROR ProcessStep2(const uint8_t *buf, const uint16_t bufSize, uint16_t& stepDataLen);
    uint8_t *GetSharedSecret(void);
    int GetCurveSize(void);

private:
#if WEAVE_CONFIG_USE_MICRO_ECC
    WEAVE_ERROR GenerateZeroKnowledgeProof(ECJPAKEStepPart *stepPart, const uECC_word_t *x, const EccPoint zkpG, const uint8_t *name, const uint16_t nameLen);
    WEAVE_ERROR VerifyZeroKnowledgeProof(const ECJPAKEStepPart *stepPart, const EccPoint zkpG, const uint8_t *name, const uint16_t nameLen);
    WEAVE_ERROR FindStepPartDataPointers(ECJPAKEStepPart *stepPart, const uint8_t *buf, const uint16_t bufSize, uint16_t& stepDataLen);
    WEAVE_ERROR GenerateStepPart(ECJPAKEStepPart *stepPart, const uECC_word_t *x, const EccPoint G, const uint8_t *name, const uint16_t nameLen);
    void ComputeSharedSecret(const EccPoint Gx);
#endif // WEAVE_CONFIG_USE_MICRO_ECC
};
#endif // WEAVE_IS_ECJPAKE_ENABLED

} // namespace Crypto
} // namespace Weave
} // namespace nl

#endif /* ELLIPTICCURVE_H_ */
