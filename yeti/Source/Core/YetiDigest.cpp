#include "YetiDigest.h"
#include "YetiUtil.h"

NAMEBEG

#define YETI_BASIC_DIGEST_BLOCK_SIZE 64

#define YETI_Digest_ROL(x, y) \
    ( (((YETI_UInt32)(x) << (y)) | (((YETI_UInt32)(x) & 0xFFFFFFFFUL) >> (32 - (y)))) & 0xFFFFFFFFUL)
#define YETI_Digest_ROR(x, y) \
    ( ((((YETI_UInt32)(x)&0xFFFFFFFFUL)>>(YETI_UInt32)((y)&31)) | ((YETI_UInt32)(x)<<(YETI_UInt32)(32-((y)&31)))) & 0xFFFFFFFFUL)

#define YETI_Sha1_F0(x,y,z)  (z ^ (x & (y ^ z)))
#define YETI_Sha1_F1(x,y,z)  (x ^ y ^ z)
#define YETI_Sha1_F2(x,y,z)  ((x & y) | (z & (x | y)))
#define YETI_Sha1_F3(x,y,z)  (x ^ y ^ z)

#define YETI_Sha1_FF0(a,b,c,d,e,i) e = (YETI_Digest_ROL(a, 5) + YETI_Sha1_F0(b,c,d) + e + W[i] + 0x5a827999UL); b = YETI_Digest_ROL(b, 30);
#define YETI_Sha1_FF1(a,b,c,d,e,i) e = (YETI_Digest_ROL(a, 5) + YETI_Sha1_F1(b,c,d) + e + W[i] + 0x6ed9eba1UL); b = YETI_Digest_ROL(b, 30);
#define YETI_Sha1_FF2(a,b,c,d,e,i) e = (YETI_Digest_ROL(a, 5) + YETI_Sha1_F2(b,c,d) + e + W[i] + 0x8f1bbcdcUL); b = YETI_Digest_ROL(b, 30);
#define YETI_Sha1_FF3(a,b,c,d,e,i) e = (YETI_Digest_ROL(a, 5) + YETI_Sha1_F3(b,c,d) + e + W[i] + 0xca62c1d6UL); b = YETI_Digest_ROL(b, 30);

#define YETI_Sha256_Ch(x,y,z)       (z ^ (x & (y ^ z)))
#define YETI_Sha256_Maj(x,y,z)      (((x | y) & z) | (x & y)) 
#define YETI_Sha256_S(x, n)         YETI_Digest_ROR((x),(n))
#define YETI_Sha256_R(x, n)         (((x)&0xFFFFFFFFUL)>>(n))
#define YETI_Sha256_Sigma0(x)       (YETI_Sha256_S(x,  2) ^ YETI_Sha256_S(x, 13) ^ YETI_Sha256_S(x, 22))
#define YETI_Sha256_Sigma1(x)       (YETI_Sha256_S(x,  6) ^ YETI_Sha256_S(x, 11) ^ YETI_Sha256_S(x, 25))
#define YETI_Sha256_Gamma0(x)       (YETI_Sha256_S(x,  7) ^ YETI_Sha256_S(x, 18) ^ YETI_Sha256_R(x,  3))
#define YETI_Sha256_Gamma1(x)       (YETI_Sha256_S(x, 17) ^ YETI_Sha256_S(x, 19) ^ YETI_Sha256_R(x, 10))


#define YETI_Md5_F(x,y,z)  (z ^ (x & (y ^ z)))
#define YETI_Md5_G(x,y,z)  (y ^ (z & (y ^ x)))
#define YETI_Md5_H(x,y,z)  (x ^ y ^ z)
#define YETI_Md5_I(x,y,z)  (y ^ (x | (~z)))

#define YETI_Md5_FF(a,b,c,d,M,s,t) \
    a = (a + YETI_Md5_F(b,c,d) + M + t); a = YETI_Digest_ROL(a, s) + b;

#define YETI_Md5_GG(a,b,c,d,M,s,t) \
    a = (a + YETI_Md5_G(b,c,d) + M + t); a = YETI_Digest_ROL(a, s) + b;

#define YETI_Md5_HH(a,b,c,d,M,s,t) \
    a = (a + YETI_Md5_H(b,c,d) + M + t); a = YETI_Digest_ROL(a, s) + b;

#define YETI_Md5_II(a,b,c,d,M,s,t) \
    a = (a + YETI_Md5_I(b,c,d) + M + t); a = YETI_Digest_ROL(a, s) + b;


class BasicDigest : public Digest
{
public:
    BasicDigest();

    virtual YETI_Result update(const YETI_UInt8 * data, YETI_Size data_size);

protected:
    YETI_Result compute_digest(YETI_UInt32 * state, 
        YETI_Cardinal state_count, 
        bool big_endian,
        DataBuffer& digest);
    virtual void compress_block(const YETI_UInt8 * block) = 0;

    YETI_UInt64 m_length_;
    YETI_UInt32 m_pending_;
    YETI_UInt8  m_buffer_[YETI_BASIC_DIGEST_BLOCK_SIZE];
};

BasicDigest::BasicDigest() :
m_length_(0),
m_pending_(0)
{
}

YETI_Result BasicDigest::update(const YETI_UInt8 * data, YETI_Size data_size)
{
    while (data_size > 0) {
        if (m_pending_ == 0 && data_size >= YETI_BASIC_DIGEST_BLOCK_SIZE) {
            compress_block(data);
            m_length_  += YETI_BASIC_DIGEST_BLOCK_SIZE * 8;
            data      += YETI_BASIC_DIGEST_BLOCK_SIZE;
            data_size -= YETI_BASIC_DIGEST_BLOCK_SIZE;
        } else {
            unsigned int chunk = data_size;
            if (chunk > (YETI_BASIC_DIGEST_BLOCK_SIZE - m_pending_)) {
                chunk = YETI_BASIC_DIGEST_BLOCK_SIZE - m_pending_;
            }
            MemoryCopy(&m_buffer_[m_pending_], data, chunk);
            m_pending_ += chunk;
            data      += chunk;
            data_size -= chunk;
            if (m_pending_ == YETI_BASIC_DIGEST_BLOCK_SIZE) {
                compress_block(m_buffer_);
                m_length_ += 8 * YETI_BASIC_DIGEST_BLOCK_SIZE;
                m_pending_ = 0;
            }
        }
    }

    return YETI_SUCCESS;
}

YETI_Result BasicDigest::compute_digest(YETI_UInt32* state,
                                        YETI_Cardinal state_count,
                                        bool big_endian,
                                        DataBuffer & digest)
{
    // increase the length of the message
    m_length_ += m_pending_ * 8;

    // append the '1' bit
    m_buffer_[m_pending_++] = 0x80;

    // if there isn't enough space left for the size (8 bytes), then compress.
    // then we can fall back to padding zeros and length encoding as normal. 
    if (m_pending_ > YETI_BASIC_DIGEST_BLOCK_SIZE-8) {
        while (m_pending_ < YETI_BASIC_DIGEST_BLOCK_SIZE) {
            m_buffer_[m_pending_++] = 0;
        }
        compress_block(m_buffer_);
        m_pending_ = 0;
    }

    // pad with zeroes up until the length
    while (m_pending_ < YETI_BASIC_DIGEST_BLOCK_SIZE-8) {
        m_buffer_[m_pending_++] = 0;
    }

    // store length
    if (big_endian) {
        bytes_from_int64_be(&m_buffer_[YETI_BASIC_DIGEST_BLOCK_SIZE-8], m_length_);
    } else {
        bytes_from_int64_le(&m_buffer_[YETI_BASIC_DIGEST_BLOCK_SIZE-8], m_length_);
    }
    compress_block(m_buffer_);

    // copy output
    digest.set_data_size(4*state_count);
    YETI_UInt8* out = digest.use_data();
    if (big_endian) {
        for (unsigned int i = 0; i < state_count; i++) {
            bytes_from_int32_be(out, state[i]);
            out += 4;
        }
    } else {
        for (unsigned int i = 0; i < state_count; i++) {
            bytes_from_int32_le(out, state[i]);
            out += 4;
        }
    }
    return YETI_SUCCESS;
}

class Sha1Digest : public BasicDigest
{
public:
    Sha1Digest();

    // Digest methods
    virtual YETI_Result   get_digest(DataBuffer& digest);
    virtual unsigned int get_size() { return 20; }

private:
    // methods
    virtual void compress_block(const YETI_UInt8* block);

    // members
    YETI_UInt32 m_state_[5];
};

Sha1Digest::Sha1Digest()
{
    m_state_[0] = 0x67452301UL;
    m_state_[1] = 0xefcdab89UL;
    m_state_[2] = 0x98badcfeUL;
    m_state_[3] = 0x10325476UL;
    m_state_[4] = 0xc3d2e1f0UL;
}

void Sha1Digest::compress_block(const YETI_UInt8* block)
{
    YETI_UInt32 a,b,c,d,e,t,W[80];

    // copy the 512-bit block into W[0..15]
    for (unsigned int i = 0; i < 16; i++) {
        W[i] = bytes_to_int32_be(&block[4*i]);
    }

    // copy the state to local variables
    a = m_state_[0];
    b = m_state_[1];
    c = m_state_[2];
    d = m_state_[3];
    e = m_state_[4];

    // expand it
    unsigned int i;
    for (i = 16; i < 80; i++) {
        W[i] = YETI_Digest_ROL(W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16], 1); 
    }

    // compress
    for (i = 0; i < 20; ) {
        YETI_Sha1_FF0(a,b,c,d,e,i++); t = e; e = d; d = c; c = b; b = a; a = t;
    }

    for (; i < 40; ) {
        YETI_Sha1_FF1(a,b,c,d,e,i++); t = e; e = d; d = c; c = b; b = a; a = t;
    }

    for (; i < 60; ) {
        YETI_Sha1_FF2(a,b,c,d,e,i++); t = e; e = d; d = c; c = b; b = a; a = t;
    }

    for (; i < 80; ) {
        YETI_Sha1_FF3(a,b,c,d,e,i++); t = e; e = d; d = c; c = b; b = a; a = t;
    }

    // store the variables back into the state
    m_state_[0] += a;
    m_state_[1] += b;
    m_state_[2] += c;
    m_state_[3] += d;
    m_state_[4] += e;
}

YETI_Result Sha1Digest::get_digest(DataBuffer& digest)
{
    return compute_digest(m_state_, 5, true, digest);
}

static const YETI_UInt32 YETI_Sha256_K[64] = {
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL,
    0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL, 0xd807aa98UL, 0x12835b01UL,
    0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL,
    0xc19bf174UL, 0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
    0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL, 0x983e5152UL,
    0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL,
    0x06ca6351UL, 0x14292967UL, 0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL,
    0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL,
    0xd6990624UL, 0xf40e3585UL, 0x106aa070UL, 0x19a4c116UL, 0x1e376c08UL,
    0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL,
    0x682e6ff3UL, 0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
    0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};

class Sha256Digest : public BasicDigest
{
public:
    Sha256Digest();

    // Digest methods
    virtual YETI_Result   get_digest(DataBuffer& digest);
    virtual unsigned int get_size() { return 32; }

private:
    // methods
    virtual void compress_block(const YETI_UInt8* block);

    // members
    YETI_UInt32 m_state_[8];
};

Sha256Digest::Sha256Digest()
{
    m_state_[0] = 0x6A09E667UL;
    m_state_[1] = 0xBB67AE85UL;
    m_state_[2] = 0x3C6EF372UL;
    m_state_[3] = 0xA54FF53AUL;
    m_state_[4] = 0x510E527FUL;
    m_state_[5] = 0x9B05688CUL;
    m_state_[6] = 0x1F83D9ABUL;
    m_state_[7] = 0x5BE0CD19UL;
}

void Sha256Digest::compress_block(const YETI_UInt8* block)
{
    YETI_UInt32 S[8], W[64];

    // copy the state into the local workspace
    for (unsigned int i = 0; i < 8; i++) {
        S[i] = m_state_[i];
    }

    // copy the 512-bit block into W[0..15]
    for (unsigned int i = 0; i < 16; i++) {
        W[i] = bytes_to_int32_be(&block[4*i]);
    }

    // fill W[16..63]
    for (unsigned int i = 16; i < 64; i++) {
        W[i] = YETI_Sha256_Gamma1(W[i - 2]) + W[i - 7] + YETI_Sha256_Gamma0(W[i - 15]) + W[i - 16];
    }        

    // compress
    for (unsigned int i = 0; i < 64; ++i) {
        YETI_UInt32 t0 = 
            S[7] + 
            YETI_Sha256_Sigma1(S[4]) + 
            YETI_Sha256_Ch(S[4], S[5], S[6]) + 
            YETI_Sha256_K[i] + 
            W[i];
        YETI_UInt32 t1 = YETI_Sha256_Sigma0(S[0]) + YETI_Sha256_Maj(S[0], S[1], S[2]);
        S[3] += t0;
        S[7]  = t0 + t1;

        YETI_UInt32 t = S[7]; S[7] = S[6]; S[6] = S[5]; S[5] = S[4]; 
        S[4] = S[3]; S[3] = S[2]; S[2] = S[1]; S[1] = S[0]; S[0] = t;
    }  

    // store the local variables back into the state
    for (unsigned i = 0; i < 8; i++) {
        m_state_[i] += S[i];
    }    
}

YETI_Result Sha256Digest::get_digest(DataBuffer& digest)
{
    return compute_digest(m_state_, 8, true, digest);
}

class Md5Digest : public BasicDigest
{
public:
    Md5Digest();

    // Digest methods
    virtual YETI_Result   get_digest(DataBuffer& digest);
    virtual unsigned int get_size() { return 16; }

protected:
    // methods
    virtual void compress_block(const YETI_UInt8* block);

    // members
    YETI_UInt32 m_state_[4];
};

Md5Digest::Md5Digest()
{
    m_state_[0] = 0x67452301UL;
    m_state_[1] = 0xefcdab89UL;
    m_state_[2] = 0x98badcfeUL;
    m_state_[3] = 0x10325476UL;
}

void Md5Digest::compress_block(const YETI_UInt8* block)
{
    YETI_UInt32 a,b,c,d,W[16];

    // copy the 512-bit block into W[0..15]
    unsigned int i;
    for (i = 0; i < 16; i++) {
        W[i] = bytes_to_int32_le(&block[4*i]);
    }

    // copy the state to local variables
    a = m_state_[0];
    b = m_state_[1];
    c = m_state_[2];
    d = m_state_[3];

    // round 1
    YETI_Md5_FF(a,b,c,d,W[ 0], 7,0xd76aa478UL)
        YETI_Md5_FF(d,a,b,c,W[ 1],12,0xe8c7b756UL)
        YETI_Md5_FF(c,d,a,b,W[ 2],17,0x242070dbUL)
        YETI_Md5_FF(b,c,d,a,W[ 3],22,0xc1bdceeeUL)
        YETI_Md5_FF(a,b,c,d,W[ 4], 7,0xf57c0fafUL)
        YETI_Md5_FF(d,a,b,c,W[ 5],12,0x4787c62aUL)
        YETI_Md5_FF(c,d,a,b,W[ 6],17,0xa8304613UL)
        YETI_Md5_FF(b,c,d,a,W[ 7],22,0xfd469501UL)
        YETI_Md5_FF(a,b,c,d,W[ 8], 7,0x698098d8UL)
        YETI_Md5_FF(d,a,b,c,W[ 9],12,0x8b44f7afUL)
        YETI_Md5_FF(c,d,a,b,W[10],17,0xffff5bb1UL)
        YETI_Md5_FF(b,c,d,a,W[11],22,0x895cd7beUL)
        YETI_Md5_FF(a,b,c,d,W[12], 7,0x6b901122UL)
        YETI_Md5_FF(d,a,b,c,W[13],12,0xfd987193UL)
        YETI_Md5_FF(c,d,a,b,W[14],17,0xa679438eUL)
        YETI_Md5_FF(b,c,d,a,W[15],22,0x49b40821UL)

        // round 2
        YETI_Md5_GG(a,b,c,d,W[ 1], 5,0xf61e2562UL)
        YETI_Md5_GG(d,a,b,c,W[ 6], 9,0xc040b340UL)
        YETI_Md5_GG(c,d,a,b,W[11],14,0x265e5a51UL)
        YETI_Md5_GG(b,c,d,a,W[ 0],20,0xe9b6c7aaUL)
        YETI_Md5_GG(a,b,c,d,W[ 5], 5,0xd62f105dUL)
        YETI_Md5_GG(d,a,b,c,W[10], 9,0x02441453UL)
        YETI_Md5_GG(c,d,a,b,W[15],14,0xd8a1e681UL)
        YETI_Md5_GG(b,c,d,a,W[ 4],20,0xe7d3fbc8UL)
        YETI_Md5_GG(a,b,c,d,W[ 9], 5,0x21e1cde6UL)
        YETI_Md5_GG(d,a,b,c,W[14], 9,0xc33707d6UL)
        YETI_Md5_GG(c,d,a,b,W[ 3],14,0xf4d50d87UL)
        YETI_Md5_GG(b,c,d,a,W[ 8],20,0x455a14edUL)
        YETI_Md5_GG(a,b,c,d,W[13], 5,0xa9e3e905UL)
        YETI_Md5_GG(d,a,b,c,W[ 2], 9,0xfcefa3f8UL)
        YETI_Md5_GG(c,d,a,b,W[ 7],14,0x676f02d9UL)
        YETI_Md5_GG(b,c,d,a,W[12],20,0x8d2a4c8aUL)

        // round 3
        YETI_Md5_HH(a,b,c,d,W[ 5], 4,0xfffa3942UL)
        YETI_Md5_HH(d,a,b,c,W[ 8],11,0x8771f681UL)
        YETI_Md5_HH(c,d,a,b,W[11],16,0x6d9d6122UL)
        YETI_Md5_HH(b,c,d,a,W[14],23,0xfde5380cUL)
        YETI_Md5_HH(a,b,c,d,W[ 1], 4,0xa4beea44UL)
        YETI_Md5_HH(d,a,b,c,W[ 4],11,0x4bdecfa9UL)
        YETI_Md5_HH(c,d,a,b,W[ 7],16,0xf6bb4b60UL)
        YETI_Md5_HH(b,c,d,a,W[10],23,0xbebfbc70UL)
        YETI_Md5_HH(a,b,c,d,W[13], 4,0x289b7ec6UL)
        YETI_Md5_HH(d,a,b,c,W[ 0],11,0xeaa127faUL)
        YETI_Md5_HH(c,d,a,b,W[ 3],16,0xd4ef3085UL)
        YETI_Md5_HH(b,c,d,a,W[ 6],23,0x04881d05UL)
        YETI_Md5_HH(a,b,c,d,W[ 9], 4,0xd9d4d039UL)
        YETI_Md5_HH(d,a,b,c,W[12],11,0xe6db99e5UL)
        YETI_Md5_HH(c,d,a,b,W[15],16,0x1fa27cf8UL)
        YETI_Md5_HH(b,c,d,a,W[ 2],23,0xc4ac5665UL)

        // round 4
        YETI_Md5_II(a,b,c,d,W[ 0], 6,0xf4292244UL)
        YETI_Md5_II(d,a,b,c,W[ 7],10,0x432aff97UL)
        YETI_Md5_II(c,d,a,b,W[14],15,0xab9423a7UL)
        YETI_Md5_II(b,c,d,a,W[ 5],21,0xfc93a039UL)
        YETI_Md5_II(a,b,c,d,W[12], 6,0x655b59c3UL)
        YETI_Md5_II(d,a,b,c,W[ 3],10,0x8f0ccc92UL)
        YETI_Md5_II(c,d,a,b,W[10],15,0xffeff47dUL)
        YETI_Md5_II(b,c,d,a,W[ 1],21,0x85845dd1UL)
        YETI_Md5_II(a,b,c,d,W[ 8], 6,0x6fa87e4fUL)
        YETI_Md5_II(d,a,b,c,W[15],10,0xfe2ce6e0UL)
        YETI_Md5_II(c,d,a,b,W[ 6],15,0xa3014314UL)
        YETI_Md5_II(b,c,d,a,W[13],21,0x4e0811a1UL)
        YETI_Md5_II(a,b,c,d,W[ 4], 6,0xf7537e82UL)
        YETI_Md5_II(d,a,b,c,W[11],10,0xbd3af235UL)
        YETI_Md5_II(c,d,a,b,W[ 2],15,0x2ad7d2bbUL)
        YETI_Md5_II(b,c,d,a,W[ 9],21,0xeb86d391UL)

        // store the variables back into the state
        m_state_[0] += a;
    m_state_[1] += b;
    m_state_[2] += c;
    m_state_[3] += d;
}

YETI_Result Md5Digest::get_digest(DataBuffer& digest)
{
    return compute_digest(m_state_, 4, false, digest);
}

/*----------------------------------------------------------------------
|
|   compute Digest(key XOR opad, Digest(key XOR ipad, data))
|   key is the MAC key
|   ipad is the byte 0x36 repeated 64 times
|   opad is the byte 0x5c repeated 64 times
|   and data is the data to authenticate
|
+---------------------------------------------------------------------*/
class HmacDigest : public Digest
{
public:
    HmacDigest(Digest::Algorithm algorithm,
        const YETI_UInt8 * key, 
        YETI_Size key_size);
    ~HmacDigest();

    // Digest methods
    virtual YETI_Result update(const YETI_UInt8* data, YETI_Size data_size) {
        return m_inner_digest_->update(data, data_size);
    }
    virtual YETI_Result get_digest(DataBuffer& buffer);
    virtual unsigned int get_size() { return m_inner_digest_->get_size(); }

private:
    Digest* m_inner_digest_;
    Digest* m_outer_digest_;
};

HmacDigest::HmacDigest(Digest::Algorithm algorithm,
                       const YETI_UInt8 * key, 
                       YETI_Size key_size)
{
    Digest::create(algorithm, m_inner_digest_);
    Digest::create(algorithm, m_outer_digest_);

    YETI_UInt8 workspace[YETI_BASIC_DIGEST_BLOCK_SIZE];

    // if the key is larger than the block size, use a digest of the key
    if (key_size > YETI_BASIC_DIGEST_BLOCK_SIZE) {
        Digest* key_digest = NULL;
        Digest::create(algorithm, key_digest);
        key_digest->update(key, key_size);
        DataBuffer hk;
        key_digest->get_digest(hk);
        key = hk.get_data();
        key_size = hk.get_data_size();
        delete key_digest;
    }

    // compute key XOR ipad
    for (unsigned int i = 0; i < key_size; i++) {
        workspace[i] = key[i] ^ 0x36;
    }
    for (unsigned int i = key_size; i < YETI_BASIC_DIGEST_BLOCK_SIZE; i++) {
        workspace[i] = 0x36;
    }

    // start the inner digest with (key XOR ipad)
    m_inner_digest_->update(workspace, YETI_BASIC_DIGEST_BLOCK_SIZE);

    // compute key XOR opad
    for (unsigned int i = 0; i < key_size; i++) {
        workspace[i] = key[i] ^ 0x5c;
    }
    for (unsigned int i = key_size; i < YETI_BASIC_DIGEST_BLOCK_SIZE; i++) {
        workspace[i] = 0x5c;
    }

    // start the outer digest with (key XOR opad)
    m_outer_digest_->update(workspace, YETI_BASIC_DIGEST_BLOCK_SIZE);
}

HmacDigest::~HmacDigest()
{
    delete m_inner_digest_;
    delete m_outer_digest_;
}

YETI_Result HmacDigest::get_digest(DataBuffer& mac)
{
    // finish the outer digest with the value of the inner digest
    DataBuffer inner;
    m_inner_digest_->get_digest(inner);
    m_outer_digest_->update(inner.get_data(), inner.get_data_size());

    // return the value of the outer digest
    return m_outer_digest_->get_digest(mac);
}

YETI_Result Digest::create(Algorithm algorithm, Digest *& digest)
{
    switch (algorithm) {
case ALGORITHM_SHA1:   digest = new Sha1Digest();   return YETI_SUCCESS;
case ALGORITHM_SHA256: digest = new Sha256Digest(); return YETI_SUCCESS;
case ALGORITHM_MD5:    digest = new Md5Digest();    return YETI_SUCCESS;
default: return YETI_ERROR_NOT_SUPPORTED;
    }
}

YETI_Result Hmac::create(Digest::Algorithm algorithm, 
                         const YETI_UInt8*      key,
                         YETI_Size              key_size,
                         Digest*&          digest)
{
    switch (algorithm) {
case Digest::ALGORITHM_SHA1: 
case Digest::ALGORITHM_MD5:
    digest = new HmacDigest(algorithm, key, key_size); 
    return YETI_SUCCESS;
default: return YETI_ERROR_NOT_SUPPORTED;
    }
}

NAMEEND
