#ifndef _CXL_YETI_CRYPTO_H_
#define _CXL_YETI_CRYPTO_H_

#include "YetiTypes.h"
#include "YetiDataBuffer.h"

NAMEBEG

class BlockCipher
{
public:
    typedef enum
    {
        AES_128,
    } Algorithm;

    typedef enum
    {
        ENCRYPT,
        DECRYPT,
    } Direction;

    static YETI_Result create(Algorithm algorithm,
        Direction                       direction,
        const YETI_UInt8 *              key,
        YETI_Size                       key_size,
        BlockCipher *&                  cipher);

    virtual             ~BlockCipher() {}
    virtual YETI_Size   get_block_size()    = 0;
    virtual Direction   get_direction()     = 0;
    virtual Algorithm   get_algorithm()     = 0;
    virtual YETI_Result process_block(const YETI_UInt8 * input, YETI_UInt8 * output) = 0;

    virtual YETI_Result process_cbc(const YETI_UInt8 * input, YETI_Size input_size, const YETI_UInt8 * iv, DataBuffer & output);

protected:
    BlockCipher() {}
};

NAMEEND

#endif // _CXL_YETI_CRYPTO_H_
