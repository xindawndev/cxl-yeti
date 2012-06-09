#ifndef _CXL_YETI_DIGEST_H_
#define _CXL_YETI_DIGEST_H_

#include "YetiTypes.h"
#include "YetiDataBuffer.h"

NAMEBEG

class Digest
{
public:
    typedef enum {
        ALGORITHM_SHA1,
        ALGORITHM_SHA256,
        ALGORITHM_MD5
    } Algorithm;

    static YETI_Result create(Algorithm algorithm, Digest *& digest);

    virtual ~Digest() {}
    virtual unsigned int get_size() = 0;
    virtual YETI_Result update(const YETI_UInt8 * data, YETI_Size data_size) = 0;
    virtual YETI_Result get_digest(DataBuffer & digest) = 0;

protected:
    Digest() {}
};

class Hmac
{
public:
    static YETI_Result create(Digest::Algorithm algorithm,
        const YETI_UInt8 * key,
        YETI_Size key_size,
        Digest *& digest);

private:
    Hmac() {}
};

NAMEEND

#endif // _CXL_YETI_DIGEST_H_
