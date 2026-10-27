#include "tape.h"
#include <stdio.h>
#include <vector>
int main(){ FILE*f=fopen("/tmp/tzxtest/t.tzx","rb"); static u8 b[4096]; int n=fread(b,1,sizeof b,f); fclose(f);
 CTape t; if(!t.InsertTZX(b,n,"t")){printf("no carga\n");return 1;} printf("bloques %u, no soportados %u\n", t.GetBlockCount(), t.GetUnsupportedBlocks());
 t.SetMachine(3500000,TRUE); t.Play(0); std::vector<unsigned> seq; unsigned last=999; u64 now=0;
 for(int i=0;i<200000;i++){ now+=35000; t.GetEarBit(now);
   if(t.IsSelectPending()){ char tx[40]; printf("seleccion: %u opciones:", t.GetSelectCount()); for(unsigned k=0;k<t.GetSelectCount();k++){ t.GetSelectItem(k,tx,sizeof tx); printf(" [%s]",tx);} printf(" -> elijo la 2\n"); t.Select(1); t.Play(now); }
   unsigned bl=t.GetBlock(); if(bl!=last){ seq.push_back(bl); last=bl; }
   if(!t.IsPlaying() && !t.IsSelectPending()) break; }
 printf("bloques recorridos:"); for(unsigned x:seq) printf(" %u",x); printf("\n"); }
