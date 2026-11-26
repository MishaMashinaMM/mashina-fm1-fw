/* Losslessly packed exact archive objects from factory/final-performance.json. */
#pragma once
#define MASHINA_FACTORY_SIZE_0 1824u
static const uint8_t mashina_factory_object_0[158] = {
14,77,80,82,49,1,0,102,4,94,116,38,118,255,255,0,138,0,8,129,167,205,57,1,
71,37,3,120,132,22,18,1,0,60,0,64,0,90,0,90,0,60,0,40,0,60,0,0,
0,30,132,23,151,0,131,33,135,87,4,1,0,1,0,127,130,5,135,3,0,16,154,65,
137,147,11,64,0,1,0,168,64,66,65,128,64,191,64,130,0,11,80,66,64,128,64,64,
64,66,65,65,64,191,129,19,130,4,2,64,64,64,255,36,180,147,5,1,2,3,4,5,
6,144,255,2,127,1,127,146,3,3,0,0,0,1,144,46,255,18,255,148,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,177,255
};
#define MASHINA_FACTORY_SIZE_1 648u
static const uint8_t mashina_factory_object_1[68] = {
5,54,82,69,80,74,0,135,0,20,53,78,65,80,2,3,4,5,6,7,8,9,10,11,
12,13,0,1,0,1,6,129,16,0,1,131,0,136,42,255,10,255,140,255,255,228,255,0,
3,129,255,0,2,129,255,0,34,134,255,5,1,2,3,4,5,6,195,255
};
#define MASHINA_FACTORY_SIZE_10 1320u
static const uint8_t mashina_factory_object_10[33] = {
5,83,79,78,71,2,0,132,0,0,1,156,1,255,35,251,143,255,4,255,134,255,255,255,
255,255,255,255,255,255,255,234,255
};
/* Literal runs (1..128 bytes) and overlapping backward copies (3..130
 * bytes, distance 1..256). Decode into existing object scratch, never allocate. */
static int mashina_factory_unpack(const uint8_t *src,uint32_t size,void *dst,uint32_t cap)
{
    uint8_t *out=(uint8_t *)dst;uint32_t i=0,n=0;
    while(i<size){
        uint32_t cmd=src[i++],len=(cmd&127u)+((cmd&128u)?3u:1u);
        if(len>cap-n)return 1;
        if(cmd&128u){
            if(i>=size)return 1;
            uint32_t d=(uint32_t)src[i++]+1u;
            if(d>n)return 1;
            while(len--){out[n]=out[n-d];n++;}
        }else{
            if(len>size-i)return 1;
            memcpy(out+n,src+i,len);i+=len;n+=len;
        }
    }
    return n!=cap;
}
#define MASHINA_FACTORY_UNPACK(id,dst) mashina_factory_unpack(mashina_factory_object_##id,sizeof mashina_factory_object_##id,(dst),MASHINA_FACTORY_SIZE_##id)
