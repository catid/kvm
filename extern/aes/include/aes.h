#pragma once

extern "C" {


void aesbs_convert_key(u8 out[], u32 const rk[], int rounds);

void aesbs_cbc_decrypt(u8 out[], u8 const in[], u8 const rk[], int rounds, int blocks, u8 iv[]);

void aesbs_xts_encrypt(u8 out[], u8 const in[], u8 const rk[], int rounds, int blocks, u8 iv[]);
void aesbs_xts_decrypt(u8 out[], u8 const in[], u8 const rk[], int rounds, int blocks, u8 iv[]);

void aesbs_ctr_encrypt(u8 out[], u8 const in[], u8 const rk[], int rounds, int blocks, u8 iv[], u8 final[]);


} // extern "C"
