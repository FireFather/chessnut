#include "hash.h"
#include <utility>
#include "position.h"

size_t tt_size=1<<23;
hash_entry* hashtable=nullptr;
rep_entry reptable[rt_size];

void resize_tt(const size_t mb){
  delete[] hashtable;
  tt_size=(mb*1024*1024)/sizeof(hash_entry);
  hashtable=new hash_entry[tt_size];
  hash_table::clear();
}

int hash_table::probe(const int depth,const int alpha,const int beta,const int ply_from_root){
  if(const rep_entry* r=&reptable[position::key()&(rt_size-1)];r->key==position::key()&&r->occurrences>1) return no_eval;
  const hash_entry* entry=&hashtable[position::key()&(tt_size-1)];
  int eval=entry->eval;
  eval-=ply_from_root*is_win(eval);
  eval+=ply_from_root*is_loss(eval);
  if(entry->key==position::key()&&std::cmp_greater_equal(entry->depth,depth)){
    if(entry->flag==exact) return eval;
    if(entry->flag==upper&&eval<=alpha) return alpha;
    if(entry->flag==lower&&eval>=beta) return beta;
  }
  return no_eval;
}

void hash_table::store(const u8 depth,const bound_type flag,int eval,const move best_move,const int ply_from_root){
  eval+=ply_from_root*is_win(eval);
  eval-=ply_from_root*is_loss(eval);
  hash_entry* e=&hashtable[position::key()&(tt_size-1)];
  e->key=position::key();
  e->depth=depth;
  e->flag=flag;
  e->eval=eval;
  e->best_move=best_move;
}

move hash_table::get_move(){
  const hash_entry* e=&hashtable[position::key()&(tt_size-1)];
  return e->key==position::key()?e->best_move:no_move;
}

void hash_table::clear(){
  memset(hashtable,0,tt_size*sizeof(hash_entry));
}

bool rep_table::draw(){
  const rep_entry* r=&reptable[position::key()&(rt_size-1)];
  return r->key==position::key()&&r->occurrences>=3;
}

void rep_table::push(){
  if(rep_entry* r=&reptable[position::key()&(rt_size-1)];r->key==position::key()){
    r->occurrences++;
  } else if(r->occurrences==0){
    r->key=position::key();
    r->occurrences=1;
  }
}

void rep_table::pop(){
  if(rep_entry* r=&reptable[position::key()&(rt_size-1)];r->key==position::key()) r->occurrences--;
}

void rep_table::clear(){
  memset(reptable,0,rt_size*sizeof(rep_entry));
}
