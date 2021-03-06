#include "encrypt.h"
#include <stdio.h>

//8-bit unsigned int
typedef uint8_t aes_gf28_t;


//Macro for efficiently computing round key step
#define AES_ENC_RND_KEY_STEP(a,b,c,d) { \
  s[ a ] = s[ a ] ^ rk[ a ];            \
  s[ b ] = s[ b ] ^ rk[ b ];            \
  s[ c ] = s[ c ] ^ rk[ c ];            \
  s[ d ] = s[ d ] ^ rk[ d ];            \
}

//Macro for efficiently computing sub-byte step
#define AES_ENC_RND_SUB_STEP(a,b,c,d) { \
  s[ a ] = sbox( s[ a ] );      \
  s[ b ] = sbox( s[ b ] );      \
  s[ c ] = sbox( s[ c ] );      \
  s[ d ] = sbox( s[ d ] );      \
}

//Macro for efficiently computing row shift step
#define AES_ENC_RND_ROW_STEP(a,b,c,d,e,f,g,h) { \
  aes_gf28_t __a1 = s[ a ];                     \
  aes_gf28_t __b1 = s[ b ];                     \
  aes_gf28_t __c1 = s[ c ];                     \
  aes_gf28_t __d1 = s[ d ];                     \
  s[ e ] = __a1;                                \
  s[ f ] = __b1;                                \
  s[ g ] = __c1;                                \
  s[ h ] = __d1;                                \
}

//Macro for efficiently computing mix-column step
#define AES_ENC_RND_MIX_STEP(a,b,c,d) {         \
  aes_gf28_t __a1 = s[ a ];                     \
  aes_gf28_t __b1 = s[ b ];                     \
  aes_gf28_t __c1 = s[ c ];                     \
  aes_gf28_t __d1 = s[ d ];                     \
  aes_gf28_t __a2 = aes_gf28_xtime( __a1 );      \
  aes_gf28_t __b2 = aes_gf28_xtime( __b1 );      \
  aes_gf28_t __c2 = aes_gf28_xtime( __c1 );      \
  aes_gf28_t __d2 = aes_gf28_xtime( __d1 );      \
  aes_gf28_t __a3 = __a1 ^ __a2;                \
  aes_gf28_t __b3 = __b1 ^ __b2;                \
  aes_gf28_t __c3 = __c1 ^ __c2;                \
  aes_gf28_t __d3 = __d1 ^ __d2;                \
  s[ a ] = __a2 ^ __b3 ^ __c1 ^ __d1;           \
  s[ b ] = __a1 ^ __b2 ^ __c3 ^ __d1;           \
  s[ c ] = __a1 ^ __b1 ^ __c2 ^ __d3;           \
  s[ d ] = __a3 ^ __b1 ^ __c1 ^ __d2;           \
}

//function to calculate r(x) = t(x) (i.e. prod(ai.x^(i+1))) (mod p(x))
aes_gf28_t aes_gf28_xtime( aes_gf28_t a ){
  //if t8 == 1 then r(x) = t(x) - p(x)
  if((a & 0x80) == 0x80){
    //0x1B IS p(x)
    return 0x1B ^ (a << 1);
  }
  //otherwise r(x) = t(x)
  else{
    return (a << 1);
  }
}

aes_gf28_t aes_gf28_mul( aes_gf28_t a, aes_gf28_t b ) {
  aes_gf28_t t = 0;
  for(int i = 7; i >= 0; i--){
    t = aes_gf28_xtime( t );
    if((b >> i) & 1){
      t ^= a;
    }
  }
  return t;
}

//Function to calculate finite field inverse, for sub-bytes
aes_gf28_t aes_gf28_inv( aes_gf28_t a ) {
  aes_gf28_t t_0 = aes_gf28_mul(    a,  a ); // a^2
  aes_gf28_t t_1 = aes_gf28_mul( t_0,   a ); // a^3
             t_0 = aes_gf28_mul( t_0, t_0 ); // a^4
             t_1 = aes_gf28_mul( t_1, t_0 ); // a^7
             t_0 = aes_gf28_mul( t_0, t_0 ); // a^8
             t_0 = aes_gf28_mul( t_1, t_0 ); // a^15
             t_0 = aes_gf28_mul( t_0, t_0 ); // a^30
             t_0 = aes_gf28_mul( t_0, t_0 ); // a^60
             t_1 = aes_gf28_mul( t_1, t_0 ); // a^67
             t_0 = aes_gf28_mul( t_0, t_1 ); // a^127
             t_0 = aes_gf28_mul( t_0, t_0 ); // a^254
  return t_0;
}


//Function which computes S-Box(a), for sub-bytes
aes_gf28_t sbox( aes_gf28_t a ){
  a = aes_gf28_inv(a);

  a = (0x63) ^ //  0   1   1   0   0   0   1   1
      ( a  ) ^ // a_7 a_6 a_5 a_4 a_3 a_2 a_1 a_0
      (a<<1) ^ // a_6 a_5 a_4 a_3 a_2 a_1 a_0  0
      (a<<2) ^ // a_5 a_4 a_3 a_2 a_1 a_0  0   0
      (a<<3) ^ // a_4 a_3 a_2 a_1 a_0  0   0   0
      (a<<4) ^ // a_3 a_2 a_1 a_0  0   0   0   0
      (a>>7) ^ //  0   0   0   0   0   0   0   a_7
      (a>>6) ^ //  0   0   0   0   0   0  a_7  a_6
      (a>>5) ^ //  0   0   0   0   0  a_7  a_6 a_5
      (a>>4);   //  0   0   0   0  a_7  a_6 a_5 a_4
  return a;
}


//Round key function
void aes_enc_rnd_key( aes_gf28_t* s, aes_gf28_t* rk ){
  AES_ENC_RND_KEY_STEP( 0, 1, 2, 3 );
  AES_ENC_RND_KEY_STEP( 4, 5, 6, 7 );
  AES_ENC_RND_KEY_STEP( 8, 9, 10, 11 );
  AES_ENC_RND_KEY_STEP( 12, 13, 14, 15 );
}

//Sub-bytes function, for S-Layer
void aes_enc_rnd_sub( aes_gf28_t* s ){
  AES_ENC_RND_SUB_STEP( 0, 1, 2, 3 );
  AES_ENC_RND_SUB_STEP( 4, 5, 6, 7 );
  AES_ENC_RND_SUB_STEP( 8, 9, 10, 11 );
  AES_ENC_RND_SUB_STEP( 12, 13, 14, 15 );
}

//Shift-rows function, for P-Layer
void aes_enc_rnd_row( aes_gf28_t* s ){
  AES_ENC_RND_ROW_STEP( 1, 5, 9, 13,
                      13,  1, 5,  9);
  AES_ENC_RND_ROW_STEP( 2, 6,10, 14,
                       10, 14, 2, 6);
  AES_ENC_RND_ROW_STEP( 3, 7, 11, 15,
                      7,  11, 15,  3);
}

//Mix-cols function, for P-Layer
void aes_enc_rnd_mix( aes_gf28_t* s ){
  AES_ENC_RND_MIX_STEP( 0, 1, 2, 3 );
  AES_ENC_RND_MIX_STEP( 4, 5, 6, 7 );
  AES_ENC_RND_MIX_STEP( 8, 9, 10, 11 );
  AES_ENC_RND_MIX_STEP( 12, 13, 14, 15 );
}

//Round key generation function
void aes_enc_keyexp_step(uint8_t* r, const uint8_t* rk, uint8_t rc){
  r[ 0 ] = rc ^ sbox( rk[ 13 ] ) ^ rk[ 0 ];
  r[ 1 ] =      sbox( rk[ 14 ] ) ^ rk[ 1 ];
  r[ 2 ] =      sbox( rk[ 15 ] ) ^ rk[ 2 ];
  r[ 3 ] =      sbox( rk[ 12 ] ) ^ rk[ 3 ];

  r[ 4 ] =                     r[ 0 ]    ^ rk[ 4 ];
  r[ 5 ] =                     r[ 1 ]    ^ rk[ 5 ];
  r[ 6 ] =                     r[ 2 ]    ^ rk[ 6 ];
  r[ 7 ] =                     r[ 3 ]    ^ rk[ 7 ];

  r[ 8 ] =                     r[ 4 ]    ^ rk[ 8 ];
  r[ 9 ] =                     r[ 5 ]    ^ rk[ 9 ];
  r[ 10 ] =                    r[ 6 ]    ^ rk[ 10];
  r[ 11 ] =                    r[ 7 ]    ^ rk[ 11];

  r[ 12 ] =                    r[ 8 ]    ^ rk[ 12];
  r[ 13 ] =                    r[ 9 ]    ^ rk[ 13];
  r[ 14 ] =                    r[ 10]    ^ rk[ 14];
  r[ 15 ] =                    r[ 11]    ^ rk[ 15];
}

void aes_enc( uint8_t* c, uint8_t* m, uint8_t* k ){
  uint8_t rk[ 16 ], s[ 16 ];

  aes_gf28_t rcp[10] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1B,0x36};
  uint8_t* rkp = rk;

  memcpy(s,m,16);
  memcpy(rkp,k,16);
  //1 initial   round
  aes_enc_rnd_key( s, rkp );
  //Nr - 1 iterated rounds
  for(int i = 1; i < 10; i++){
    aes_enc_rnd_sub( s       );
    aes_enc_rnd_row( s       );
    aes_enc_rnd_mix( s       );
    aes_enc_keyexp_step(rkp,rkp,rcp[i-1]);
    aes_enc_rnd_key( s, rkp );


  }
  //1 final     round
  aes_enc_rnd_sub( s       );
  aes_enc_rnd_row( s       );
  aes_enc_keyexp_step( rkp, rkp, rcp[9]);
  aes_enc_rnd_key( s, rkp );

  memcpy(c,s,16);

}


int main(int argc, char *argv[]){
  //Test vector
  uint8_t k[ 16 ] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
                      0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };
  uint8_t m[ 16 ] = { 0x32, 0x43, 0xF6, 0xA8, 0x88, 0x5A, 0x30, 0x8D,
                      0x31, 0x31, 0x98, 0xA2, 0xE0, 0x37, 0x07, 0x34 };
  uint8_t c[ 16 ] = { 0x39, 0x25, 0x84, 0x1D, 0x02, 0xDC, 0x09, 0xFB,
                      0xDC, 0x11, 0x85, 0x97, 0x19, 0x6A, 0x0B, 0x32 };
  uint8_t t[ 16 ];

  aes_enc(t,m,k);
  bool match = true;
  for(int i = 0; i < 16; i++){
    if(c[i] != t[i]){
      match = false;
    }
  }
  if(match){
    printf("AES(m,k) == c\n");
  }

  return 0;

}
